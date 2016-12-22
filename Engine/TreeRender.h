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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Engine/EngineFwd.h"
#include "Engine/ImageComponents.h"
#include "Engine/ViewIdx.h"

#include "Global/GlobalDefines.h"


NATRON_NAMESPACE_ENTER;


/**
 * @brief Setup thread local storage through a render tree starting from the tree root.
 * This is mandatory to create an instance of this class before calling renderRoI on the treeRoot.
 * Without this a lot of the compositing engine intelligence cannot work properly.
 * Dependencies are computed recursively. The constructor may throw an exception upon failure.
 **/
struct TreeRenderPrivate;
class TreeRender
: public QObject
, public boost::enable_shared_from_this<TreeRender>
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    struct CtorArgs
    {
        // The time at which to render
        double time;

        // The view at which to render
        ViewIdx view;

        // The node at the bottom of the tree (from which we want to pull an image from)
        NodePtr treeRoot;

        // When painting with a roto item, this points to the item used to draw
        RotoDrawableItemPtr activeRotoDrawableItem;

        // Pointer to stats object if any.
        RenderStatsPtr stats;

        // If non null, this is the portion of the input image to render, otherwise the
        // full region of definition wil be rendered.
        const RectD* canonicalRoI;

        // This is the layer to render
        ImageComponents layer;

        // mip-map level at which to Render
        unsigned int mipMapLevel;
        
        // True if the render should be draft (i.e: low res) because user is anyway
        // scrubbing timeline or a slider
        bool draftMode;

        // Is this render triggered by a playback or render on disk ?
        bool playback;

        // Make sure each node in the tree gets rendered at least once
        bool byPassCache;
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
     * Once initialized this will render the image and return it in renderedImage.
     * The return code of render can be obtained in renderStatus
     **/
    static ImagePtr renderImage(const CtorArgsPtr& inArgs, ImagePtr* renderedImage, RenderRoIRetCode* renderStatus);

    virtual ~TreeRender();

    /**
     * @brief Is this render aborted ? This is extremely fast as it just dereferences an atomic integer
     **/
    bool isAborted() const;

    /**
     * @brief Set this render as aborted, cannot be reversed. This is called when the function GenericSchedulerThread::abortThreadedTask() is called
     **/
    void setAborted();

    /**
     * @brief Returns whether this render is part of a playback render or just a single render
     **/
    bool isPlayback() const;

    /**
     * @brief Returns whether this render is a bad quality render (typically used when scrubbing a slider or the timeline) or normal quality render
     **/
    bool isDraftRender() const;

    /**
     * @brief Returns arguments that are specific to the given node by that remain the same throughout the render of the frame, even if multiple time/view
     * are rendered.
     **/
    TreeRenderNodeArgsPtr getNodeRenderArgs(const NodePtr& node) const;

private Q_SLOTS:

    /**
     * @brief Triggered by a timer after some time that a thread called setAborted(). This is used to detect stalled threads that do not seem
     * to want to abort.
     **/
    void onAbortTimerTimeout();

    void onStartTimerInOriginalThreadTriggered();

Q_SIGNALS:

    void startTimerInOriginalThread();

private:



    /**
     * @brief Registers the thread as part of this render request. Whenever AbortableThread::setAbortInfo is called, the thread is automatically registered
     * in this class as to be part of this render. This is used to monitor running threads for a specific render and to know if a thread has stalled when
     * the user called setAborted()
     **/
    void registerThreadForRender(AbortableThread* thread);

    /**
     * @brief Unregister the thread as part of this render, returns true
     * if it was registered and found.
     * This is called whenever a thread calls cleanupTLSForThread(), i.e:
     * when its processing is finished.
     **/
    bool unregisterThreadForRender(AbortableThread* thread);

    // To call registerThreadForRender and unregister
    friend class AbortableThread;

    boost::scoped_ptr<TreeRenderPrivate> _imp;
};


NATRON_NAMESPACE_EXIT;

#endif // NATRON_ENGINE_TREERENDER_H
