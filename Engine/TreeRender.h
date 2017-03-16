/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Engine/ImagePlaneDesc.h"
#include "Engine/TimeValue.h"
#include "Engine/ViewIdx.h"
#include "Engine/RectD.h"
#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief Used in the implementation of the TreeRender to determine FrameViewRequest that are available to be rendered
 * and holds the global render status of the tree.
 **/
struct RequestPassSharedDataPrivate;
class RequestPassSharedData : public boost::enable_shared_from_this<RequestPassSharedData>
{
public:
    RequestPassSharedData();

    ~RequestPassSharedData();

    void addTaskToRender(const FrameViewRequestPtr& render);

    TreeRenderPtr getTreeRender() const;

private:

    friend class FrameViewRenderRunnable;
    friend struct TreeRenderPrivate;
    boost::scoped_ptr<RequestPassSharedDataPrivate> _imp;
};

/**
 * @brief Setup thread local storage through a render tree starting from the tree root.
 * This is mandatory to create an instance of this class before calling renderRoI on the treeRoot.
 * Without this a lot of the compositing engine intelligence cannot work properly.
 * Dependencies are computed recursively. The constructor may throw an exception upon failure.
 **/
struct TreeRenderPrivate;
class TreeRender
: public boost::enable_shared_from_this<TreeRender>
{

public:

    struct CtorArgs
    {
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
        
        // If non null, this is the portion of the input image to render, otherwise the
        // full region of definition wil be rendered.
        const RectD* canonicalRoI;

        // The plane to render. If NULL this will be set to the first
        // plane the node produces (usually the color plane)
        const ImagePlaneDesc* plane;

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
     * For OpenFX this also set thread-local storage on all nodes that will be visited to ensure OpenFX
     * does not loose context.
     * This function may throw an exception in case of failure
     **/
    static TreeRenderPtr create(const CtorArgsPtr& inArgs);

    /**
     * @brief Launch the actual render on the tree that was constructed in create().
     * @param outputResults[out] The output results if the render succeeded. The image
     * can be retrieved on the request with getImagePlane().
     * @returns The status, eActionStatusOK if everything went fine.
     **/
    ActionRetCodeEnum launchRender(FrameViewRequestPtr* outputRequest);

    /**
     * @brief Same as launchRender() except that it launches the render on a different node than the root
     * of the tree with different parameters. 
     **/
    ActionRetCodeEnum launchRenderWithArgs(const EffectInstancePtr& root,
                                           TimeValue time,
                                           ViewIdx view,
                                           const RenderScale& proxyScale,
                                           unsigned int mipMapLevel,
                                           const ImagePlaneDesc* plane,
                                           const RectD* canonicalRoI,
                                           FrameViewRequestPtr* outputRequest);
public:

    virtual ~TreeRender();

    /**
    * @brief Get the frame of the render
    **/
    TimeValue getTime() const;

    /**
     * @brief Get the view of the render
     **/
    ViewIdx getView() const;

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
     * @brief Returns the object used to gather stats for this rende
     **/
    RenderStatsPtr getStatsObject() const;

    /**
     * @brief Get the OpenGL context associated to this render
     **/
    OSGLContextPtr getGPUOpenGLContext() const;
    OSGLContextPtr getCPUOpenGLContext() const;


private:



    boost::scoped_ptr<TreeRenderPrivate> _imp;
};


NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_TREERENDER_H
