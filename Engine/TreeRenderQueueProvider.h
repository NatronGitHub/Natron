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

#ifndef NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H
#define NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/EngineFwd.h"
#include "Global/GlobalDefines.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"


NATRON_NAMESPACE_ENTER;

/**
 * @brief Common interface shared between TreeRenderQueueProvider and TreeRenderQueueManager
 * for launching renders. This exists so that the user is only tempted to call functions on the 
 * TreeRenderQueueProvider which forwards them to the TreeRenderQueueManager
 **/
class TreeRenderLauncherI
{
public:
    TreeRenderLauncherI() {}

    virtual ~TreeRenderLauncherI() {}

    /**
     * @brief Launch a new render to be processed and returns immediately. To retrieve results for that render you
     * must call waitForRenderFinished()
     **/
    virtual void launchRender(const TreeRenderPtr& render) = 0;

    /**
     * @brief Launch a sub-render execution for the given tree render. This assumes that launchRender() was called
     * already for this TreeRender. this is used in the implementation of getImagePlane to re-use the same render clones
     * for the same TreeRender.
     **/
    virtual TreeRenderExecutionDataPtr launchSubRender(const EffectInstancePtr& treeRoot,
                                                       TimeValue time,
                                                       ViewIdx view,
                                                       const RenderScale& proxyScale,
                                                       unsigned int mipMapLevel,
                                                       const ImagePlaneDesc* planeParam,
                                                       const RectD* canonicalRoIParam,
                                                       const TreeRenderPtr& render) = 0;
    
    
    /**
     * @brief Waits for a specific render to be finished.
     **/
    virtual void waitForRenderFinished(const TreeRenderPtr& render) = 0;

    /**
     * @brief Used to wait for the execution of a render to be finished. Do not call directly
     **/
    virtual void waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData) = 0;

};

/**
 * @brief Abstract class for objects that want to launch renders.
 **/
class TreeRenderQueueProvider : public TreeRenderLauncherI
{
public:

    TreeRenderQueueProvider() {}

    virtual ~TreeRenderQueueProvider() {}

    /// Overiden from TreeRenderLauncherI

    // Every method is fowarded to the TreeRenderQueueManager
    virtual void launchRender(const TreeRenderPtr& render) OVERRIDE FINAL;
    virtual TreeRenderExecutionDataPtr launchSubRender(const EffectInstancePtr& treeRoot,
                                 TimeValue time,
                                 ViewIdx view,
                                 const RenderScale& proxyScale,
                                 unsigned int mipMapLevel,
                                 const ImagePlaneDesc* planeParam,
                                 const RectD* canonicalRoIParam,
                                 const TreeRenderPtr& render) OVERRIDE FINAL;
    virtual void waitForRenderFinished(const TreeRenderPtr& render) OVERRIDE FINAL;
    virtual void waitForTreeRenderExecutionFinished(const TreeRenderExecutionDataPtr& execData) OVERRIDE FINAL;
    bool hasTreeRendersFinished() const;
    TreeRenderPtr waitForAnyTreeRenderFinished();
    ///

    /**
     * @brief This is called by the TreeRenderQueueManager when under-used to let the provider
     * launch more renders. This is called on the TreeRenderQueueManager thread.
     * Note that this function is called only if you passed the "playback" flag to the CtorArgs of a previous tree render
     * passed to launchRender(). 
     * It should return a TreeRender object that the implementation knows will be needed in the future, e.g: the TreeRender for
     * the next frame in the sequence. 
     * The implementation does not have to call launchRender(), the manager takes care of it
     **/
    virtual TreeRenderPtr fetchTreeRenderToLaunch() { return TreeRenderPtr(); };

protected:

    virtual TreeRenderQueueProviderConstPtr getThisTreeRenderQueueProviderShared() const = 0;

};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H
