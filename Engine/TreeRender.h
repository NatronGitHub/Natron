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

#ifndef NATRON_ENGINE_TREERENDER_H
#define NATRON_ENGINE_TREERENDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QRunnable>


#include "Engine/ImagePlaneDesc.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectD.h"
#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER


/**
 * @brief A sub-execution of a render in a TreeRender. Typically this is created when calling getImagePlane()
 **/
struct TreeRenderExecutionDataPrivate;
class TreeRenderExecutionData : public boost::enable_shared_from_this<TreeRenderExecutionData>
{
public:

    /**
     * @brief @param createTreeRenderIfUnrenderedImage If true, anything that request new images
     * to be rendered from within the execution should create a new TreeRender instead. 
     * See discussion in @FrameViewRequest: this is to overcome thread-safety for host frame-threading effects.
     **/
    TreeRenderExecutionData(bool createTreeRenderIfUnrenderedImage);

    virtual ~TreeRenderExecutionData();

    /**
     * @brief A TreeRender may have multiple executions: One is the main execution that returns the image
     * of the requested arguments passed to the ctor, but sub-executions may be created for example
     * in getImagePlane or to retrieve extraneous images for the color-picker
     **/
    bool isTreeMainExecution() const;

    /**
     * @brief Returns a pointer to the TreeRender that created this object.
     **/
    TreeRenderPtr getTreeRender() const;

    /**
     * @brief Returns the status of this object. If returning eActionStatusFailed, this object should not be used anymore.
     **/
    ActionRetCodeEnum getStatus() const;

    /**
     * @brief Returns the request object of the output node of the tree render execution
     **/
    FrameViewRequestPtr getOutputRequest() const;

    bool isNewTreeRenderUponUnrenderedImageEnabled() const;

    /**
     * @brief Starts tasks that are available for rendering and queue them in the thread pool
     * @param launchAllTasksPossible A boolean indicating how many tasks to start. If -1 is passed, all available tasks
     * should be started.
     * @returns The number of parallel tasks that were queued.
     **/
    int executeAvailableTasks(int nTasksToLaunch);

private:

    void addTaskToRender(const FrameViewRequestPtr& render);


    friend class EffectInstance;
    friend class FrameViewRenderRunnable;
    friend struct TreeRenderPrivate;
    boost::scoped_ptr<TreeRenderExecutionDataPrivate> _imp;
};

class FrameViewRenderRunnable;
typedef boost::shared_ptr<FrameViewRenderRunnable> FrameViewRenderRunnablePtr;
/**
 * @brief A runnable that executes the render of 1 node (FrameViewRequest) within a TreeRenderExecutionData. 
 * Once rendered, this task will make tasks that depend on this task's results available for render.
 **/
class FrameViewRenderRunnable
    : public QRunnable
    , public boost::enable_shared_from_this<FrameViewRenderRunnable>
{
    struct Implementation;

    struct MakeSharedEnabler;

    // used by boost::make_shared<>
    FrameViewRenderRunnable(const TreeRenderExecutionDataPtr& sharedData, const FrameViewRequestPtr& request);

public:
    static FrameViewRenderRunnablePtr create(const TreeRenderExecutionDataPtr& sharedData, const FrameViewRequestPtr& request);

    virtual ~FrameViewRenderRunnable();

    virtual void run() OVERRIDE FINAL;

private:

    boost::scoped_ptr<Implementation> _imp;
};

/**
 * @brief This object represents a single render request for a compositing tree. 
 * The arguments to be passed to render are passed in the CtorArgs struct.
 * A TreeRender may also report extrenous results of nodes along the tree:
 * this is most useful for example for the color picker which may need an image which 
 * is not the root of the tree.
 * Each render execution is represented by a TreeRenderExecutionData which is a topologically sorted
 * execution list of the nodes to render. The rendering of TreeRenderExecutionData themselves is managed by the
 * TreeRenderQueueManager which is able to distribute threads amongst multiple concurrent renders.
 **/
struct TreeRenderPrivate;
class TreeRender
: public boost::enable_shared_from_this<TreeRender>
{

public:

    struct CtorArgs
    {
        // Pointer to the object that issued the render.
        // This is used by the TreeRenderQueueManager and must be set
        TreeRenderQueueProviderConstPtr provider;

        // The time at which to render
        TimeValue time;

        // The view at which to render
        ViewIdx view;

        // The node at the bottom of the tree (from which we want to pull an image from).
        // If calling launchRenderWithArgs() the treeRoot is expected to be a render clone of the
        // effect that was created in TreeRender::create, otherwise this can be the main instance.
        EffectInstancePtr treeRootEffect;

        // List of nodes that belong to the tree upstream of treeRootEffect for which we desire
        // a pointer of the resulting image. This is useful for the Viewer to enable color-picking:
        // the output image is the image out of the ViewerProcess node, but what the user really
        // wants is the color-picker of the image in input of the Viewer (group) node.
        // These images can then be retrieved using the getExtraRequestedResultsForNode() function.
        std::list<NodePtr> extraNodesToSample;

        // When painting with a roto item, this points to the item used to draw
        RotoDrawableItemPtr activeRotoDrawableItem;

        // Pointer to stats object if any.
        RenderStatsPtr stats;
        
        // If non empty, this is the portion of the input image to render, otherwise the
        // full region of definition wil be rendered.
        RectD canonicalRoI;

        // The plane to render. If none, this will be set to the first
        // plane the node produces (usually the color plane)
        ImagePlaneDesc plane;

        // Proxy scale is the scale to apply to the parameters (that are expressed in the full format)
        // to obtain their value in the proxy format.
        // A value of 1 indicate that parameters should not be scaled.
        RenderScale proxyScale;

        // The mipMapScale is a scale factor applied after the proxy scale.
        // Level 0 = 1, level 1 = 0.5, level 2 = 0.25 etc..
        unsigned int mipMapLevel;
        
        // True if the render should be draft (i.e: low res) because user is anyway
        // scrubbing timeline or a slider
        bool draftMode;

        // Is this render triggered by a playback or render on disk ?
        bool playback;

        // Make sure each node in the tree gets rendered at least once
        bool byPassCache;

        // If true, this TreeRender will not have any other concurrent TreeRender.
        // This is used for example when RotoPainting to ensure
        // mouse move event renders are processed in order.
        bool preventConcurrentTreeRenders;

        CtorArgs();
    };

    typedef boost::shared_ptr<CtorArgs> CtorArgsPtr;

private:

    /**
     * @brief This object identifies a unique render of a tree at a given time and view.
     * Each render is uniquely identified by this object.
     **/
    TreeRender();


public:
    
    /**
     * @brief Create a new render and initialize some data such as OpenGL context.
     **/
    static TreeRenderPtr create(const CtorArgsPtr& inArgs);


public:

    virtual ~TreeRender();


    /**
     * @brief Returns a pointer to the original issuer of the tree render
     **/
    TreeRenderQueueProviderConstPtr getProvider() const;

    /**
    * @brief Get the frame of the render
    **/
    TimeValue getTime() const;

    /**
     * @brief Get the view of the render
     **/
    ViewIdx getView() const;

    /**
     * @brief Get the RoI passed in the CtorArgs
     **/
    RectD getCtorRoI() const;

    /**
     * @brief Returns preventConcurrentTreeRenders from the CtorArgs
     **/
    bool isConcurrentRendersAllowed() const;

    /**
     * @brief The proxy scale requested
     **/
    const RenderScale& getProxyScale() const;

    /**
     * @brief Returns the tre root passed to create()
     **/
    EffectInstancePtr getOriginalTreeRoot() const;

    /**
     * @brief Is this render aborted ? This is extremely fast as it just dereferences an atomic integer
     **/
    bool isRenderAborted() const;

    /**
     * @brief Set this render as aborted, cannot be reversed. This is called when the function GenericSchedulerThread::abortThreadedTask() is called
     **/
    void setRenderAborted();

    /**
     * @brief Returns whether this render is part of a playback render or just a single render
     **/
    bool isPlayback() const;

    /**
     * @brief Returns whether this render is a bad quality render (typically used when scrubbing a slider or the timeline) or normal quality render
     **/
    bool isDraftRender() const;

    /**
     * @brief If true, effects should always render at least once during the render of the tree
     **/
    bool isByPassCacheEnabled() const;

    /**
     * @brief Should nodes check for NaN pixels ?
     **/
    bool isNaNHandlingEnabled() const;

    /**
     * @brief Should nodes concatenate if possible
     **/
    bool isConcatenationEnabled() const;


    /**
     * @brief Returns the request of the given node if it was requested in the
     * extraNodesToSample list in the ctor arguments.
     **/
    FrameViewRequestPtr getExtraRequestedResultsForNode(const NodePtr& node) const;

    /**
     * @brief Returns the request object of the output node of the tree render execution
     **/
    FrameViewRequestPtr getOutputRequest() const;

    /**
     * @brief Get the current of the render
     **/
    ActionRetCodeEnum getStatus() const;

    /**
     * @brief Return true if an image was requested in output for the given node
     **/
    bool isExtraResultsRequestedForNode(const NodePtr& node) const;

    /**
     * @brief While drawing a preview with the RotoPaint node, this is the bounding box of the area
     * to update on the viewer.
     * This is in pixel coordinates at the mipmap level given to render
     **/
    bool getRotoPaintActiveStrokeUpdateArea(RectI* area) const;
    void setOrUnionActiveStrokeUpdateArea(const RectI& area);

    /**
     * @brief Returns the object used to gather stats for this rende
     **/
    RenderStatsPtr getStatsObject() const;

    /**
     * @brief Get the OpenGL context associated to this render
     **/
    OSGLContextPtr getGPUOpenGLContext() const;
    OSGLContextPtr getCPUOpenGLContext() const;

    /**
     * @brief If this render is triggered while drawing a roto item, this is the main instance of the item
     **/
    RotoDrawableItemPtr getCurrentlyDrawingItem() const;

    /**
     * @brief Internal function to register a render-clone associated to this TreeRender. This is used to track clones and properly
     * destroy them once the render is finished.
     **/
    void registerRenderClone(const KnobHolderPtr& holder);


private:

    // These functions below are accessed by TreeRenderQueueManager
    /**
     * @brief Create execution data for the main render of the TreeRender. Note that this object must be passed to the TreeRenderQueueManager which will 
     * schedule the execution of the render
     **/
    TreeRenderExecutionDataPtr createMainExecutionData();

    /**
     * @brief Create execution data for a sub-execution of the tree render: this is used in the implementation of getImagePlane to re-use the same render clones
     * for the same TreeRender.
     * This is also used to retrieve extraneous images (such as color-picker) for the TreeRender
     **/
    TreeRenderExecutionDataPtr createSubExecutionData(const EffectInstancePtr& treeRoot,
                                                      TimeValue time,
                                                      ViewIdx view,
                                                      const RenderScale& proxyScale,
                                                      unsigned int mipMapLevel,
                                                      const ImagePlaneDesc* planeParam,
                                                      const RectD* canonicalRoIParam,
                                                      int concatenationFlags,
                                                      bool createTreeRenderIfUnrenderedImage);

    /**
     * @brief Calls createSubExecutionData for each extra requested node result
     **/
    std::list<TreeRenderExecutionDataPtr> getExtraRequestedResultsExecutionData();

    void setResults(const FrameViewRequestPtr& request, const TreeRenderExecutionDataPtr& execData);


    void cleanupRenderClones();
    
private:

    friend class TreeRenderQueueManager;
    friend class LaunchRenderRunnable;
    friend struct TreeRenderExecutionDataPrivate;
    boost::scoped_ptr<TreeRenderPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TREERENDER_H
