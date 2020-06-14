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

#ifndef NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H
#define NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****



#include "Engine/EngineFwd.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif


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
     * Note that this function is blocking and will return only once the execution is finished.
     *
     * @param createTreeRenderIfUnrenderedImage If true, anything that request new images
     * to be rendered from within the execution should create a new TreeRender instead.
     * See discussion in @FrameViewRequest: this is to overcome thread-safety for host frame-threading effects.
     **/
    virtual TreeRenderExecutionDataPtr launchSubRender(const EffectInstancePtr& treeRoot,
                                                       TimeValue time,
                                                       ViewIdx view,
                                                       const RenderScale& proxyScale,
                                                       unsigned int mipMapLevel,
                                                       const ImagePlaneDesc* planeParam,
                                                       const RectD* canonicalRoIParam,
                                                       const TreeRenderPtr& render,
                                                       int concatenationFlags,
                                                       bool createTreeRenderIfUnrenderedImage) = 0;
    
    
    /**
     * @brief Waits for a specific render to be finished.
     **/
    virtual ActionRetCodeEnum waitForRenderFinished(const TreeRenderPtr& render) = 0;

};

/**
 * @brief Abstract class for objects that want to launch renders.
 **/
class TreeRenderQueueProvider : public TreeRenderLauncherI
{

    struct Implementation;

public:

    TreeRenderQueueProvider();

    virtual ~TreeRenderQueueProvider();

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
                                                       const TreeRenderPtr& render,
                                                       int concatenationFlags,
                                                       bool createTreeRenderIfUnrenderedImage) OVERRIDE FINAL;
    virtual ActionRetCodeEnum waitForRenderFinished(const TreeRenderPtr& render) OVERRIDE FINAL WARN_UNUSED_RETURN;
    bool hasTreeRendersFinished() const;
    bool hasTreeRendersLaunched() const;
    TreeRenderPtr waitForAnyTreeRenderFinished();
    void waitForAllTreeRenders();

    bool isWaitingForAllTreeRenders() const;
    ///

protected:

    /**
     * @brief Implement to return a shared ptr to this
     **/
    virtual TreeRenderQueueProviderConstPtr getThisTreeRenderQueueProviderShared() const = 0;

    /**
     * @brief This is called by the TreeRenderQueueManager thread when under-used to let the provider
     * launch more renders. This is called on the TreeRenderQueueManager thread.
     * Note that this function is called only if you passed the "playback" flag to the CtorArgs of a previous tree render
     * passed to launchRender().
     * This should call launchRender on a TreeRender that the implementation knows will be needed in the future, e.g: the TreeRender for
     * the next frame in the sequence.
     **/
    virtual void requestMoreRenders() { };

    /**
     * @brief Callback called on a thread-pool thread once a render is finished. Even if implementing this callback, you must call
     * waitForRenderFinished(render) to remove data associated to that render. If called within this function, this would return
     * immediately since the render is finished.
     **/
    virtual void onTreeRenderFinished(const TreeRenderPtr& /*render*/) {}

private:

    // Called by TreeRenderQueueManager
    friend class TreeRenderQueueManager;

    void notifyNeedMoreRenders();
    void notifyTreeRenderFinished(const TreeRenderPtr& render);

    boost::scoped_ptr<Implementation> _imp;

};

NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_TREERENDERQUEUEPROVIDER_H
