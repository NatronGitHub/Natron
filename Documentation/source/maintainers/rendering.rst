.. _maint-rendering:

Rendering, Threading and Caching
================================

This is the most intricate part of Natron. It is worth reading slowly, because
almost every performance-sensitive bug lives here.

Demand-driven, region-based rendering
-------------------------------------

Natron renders **lazily and locally**: it computes only the pixels that are
actually needed to satisfy a request, not whole frames of every node. A request
originates from an *output* effect — the Viewer showing the current frame, or a
Writer rendering a range — and flows *up* the graph toward the readers.

The central call is::

    RenderRoIRetCode EffectInstance::renderRoI(const RenderRoIArgs& args,
                                               std::map<ImagePlaneDesc, ImagePtr>* outputPlanes)

``RenderRoIArgs`` says *what* to produce: the time, the view, the render scale
(mip-map level — a power-of-two downscale used for proxy and for zoomed-out
viewing), the region of interest (a ``RectI``), and the desired planes
(components) and bit depth. ``renderRoI`` finds the region of definition,
handles identity and cached results, recursively renders the inputs it needs,
and then calls the effect's render action on what is left. The
:ref:`worked example <maint-renderroi-example>` below follows one call step by
step.

The *actions* an effect implements mirror the OpenFX action set:
``getRegionOfDefinition``, ``getRegionsOfInterest``, ``getFramesNeeded``,
``isIdentity``, ``getPreferredMetadata`` (components, bit depth,
premultiplication state, pixel aspect, frame rate),
``beginSequenceRender``/``endSequenceRender``, and ``render``. The engine
calls them through ``…_public`` wrappers (``getRegionOfDefinition_public``,
``isIdentity_public``, ``render_public``, …), which add caching of the results
and recursion checks. ``renderRoIInternal`` (in ``EffectInstanceRenderRoI.cpp``)
is the private core that runs ``beginSequenceRender``, the ``render`` action on
each rectangle, and ``endSequenceRender``; the public ``renderRoI``
orchestrates recursion, identity and cache decisions.

Per-render context: ParallelRenderArgs and TLS
----------------------------------------------

A single frame render is a deep recursive tree walk executed by many threads. It
needs a lot of shared context: which frame/view is being rendered, the node
hashes captured at the start (so the tree is consistent even if the user changes
something mid-render), the abort flag, timing/statistics, and safety flags.

Threading that through every function signature would be intractable, so Natron
stores it in **thread-local storage**. ``ParallelRenderArgs`` is described in
its header as *"thread-local arguments given to render a frame by the tree …
not to a single renderRoI call but to the rendering of a whole frame."* It is
installed for the duration of a render by ``ParallelRenderArgsSetter`` (and
``ViewerParallelRenderArgsSetter``, in ``ViewerInstance.cpp``) and reached
through the ``TLSHolder`` / ``AppTLS`` machinery.

The Viewer and writer frame renders and the node previews do three things
before they call ``renderRoI`` (see ``DefaultRenderFrameRunnable::renderFrame``
in ``OutputSchedulerThread.cpp`` for writers,
``ViewerInstance::renderViewer_internal`` for the Viewer, and
``Node::makePreviewImage`` for previews). Other entry points (the tracker,
analysis actions, ``DefaultScheduler::processFrame``) only do the first one,
without the request pass:

1. Construct a ``ParallelRenderArgsSetter`` for the tree
   (``ParallelRenderArgs.cpp``). Its constructor visits every node upstream of
   the output and stores a ``ParallelRenderArgs`` in the TLS of each of their
   effects (``setParallelRenderArgsTLS``); its destructor removes them.
2. Run the **request pass**, ``EffectInstance::computeRequestPass()``. It walks
   the tree once without rendering (``getInputsRoIsFunctor`` and
   ``treeRecurseFunctor`` with ``isRenderFunctor`` false) and records, for each
   node, the region of definition, the regions of interest of its inputs and
   the frames it needs.
3. Store the result in the TLS with
   ``ParallelRenderArgsSetter::updateNodesRequest()``.
   ``renderRoI`` then reuses these results (for example the frames needed)
   instead of calling the actions again.

If ``renderRoI`` finds no ``ParallelRenderArgs`` in the TLS, it prints a
``[BUG]`` message and creates a minimal one on the fly
(``EffectInstanceRenderRoI.cpp``, at the start of ``renderRoI``), so a missing
setter shows up in the log rather than as a crash.

.. note::

   Because so much render state is captured up front and stored in TLS, the two
   classic ways to break rendering are (a) reading live node state during a
   render instead of the captured hash/args, and (b) forgetting to install or
   restore the ``ParallelRenderArgsSetter`` on a new code path. When adding a
   render entry point, mirror an existing one exactly.

Scheduler threads and playback
------------------------------

Interactive playback and batch rendering are driven by scheduler threads built
on a common base:

- ``GenericSchedulerThread`` (``GenericSchedulerThread.cpp``): a reusable
  start/abort/quit worker-thread abstraction, with a watcher
  (``GenericSchedulerThreadWatcher``) that runs a callback when the thread
  finishes.
- ``RenderEngine`` (``OutputSchedulerThread.h``), owned by each
  ``OutputEffectInstance``, creates the scheduler on first use:
  a ``DefaultScheduler`` for writers, a ``ViewerDisplayScheduler`` for the
  Viewer (``ViewerRenderEngine``). Both derive from ``OutputSchedulerThread``,
  which derives from ``GenericSchedulerThread``.
- ``OutputSchedulerThread`` schedules the rendering of a *range* of frames:
  ``startRender()`` and ``startTasks()`` start ``RenderThreadTask`` workers,
  each of which picks the next frame (``pickFrameToRender()``) and renders it.
  For writers, the worker (``DefaultRenderFrameRunnable``) calls ``renderRoI``
  on the writer itself and reports the frame with ``notifyFrameRendered()``.
  For the Viewer, the worker (``ViewerRenderFrameRunnable``) calls
  ``ViewerInstance::renderViewer()`` and appends the result to the scheduler's
  buffer (``appendToBuffer()``); the scheduler thread then shows the frames in
  order and at the right rate (``ViewerDisplayScheduler::processFrame()`` →
  ``ViewerInstance::updateViewer()``).
- The Viewer has its own "current frame" scheduler
  (``ViewerCurrentFrameRequestScheduler``, used by
  ``RenderEngine::renderCurrentFrame()``) so that scrubbing the timeline or
  editing a parameter coalesces and cancels stale requests.
- On the command line, ``AppInstance::startWritersRendering()``
  (``AppInstance.cpp``) calls ``OutputEffectInstance::renderFullSequence()``
  on each writer, which goes through the same ``RenderEngine``.
- ``ThreadPool`` (``ThreadPool.cpp``) and Qt's ``QtConcurrent`` provide the
  worker threads that render rectangles in parallel; ``AbortableThread`` /
  ``AbortableRenderInfo`` let a render be cancelled promptly when the user acts
  again.

Aborting renders
----------------

Because feedback must be immediate, any in-flight render can be **aborted**.
``AbortableRenderInfo`` tracks whether a given render has been cancelled;
render code polls it and unwinds cleanly. When you write a long-running loop in
render code, check the abort flag periodically — a render that ignores aborts
makes the whole UI feel stuck.

The cache
---------

Rendered images are kept in the caches owned by ``AppManager`` (node images,
``DiskCache`` node images and Viewer textures), under a hash that changes
whenever a parameter or an input changes. See :ref:`maint-cache`.

GPU rendering contexts
----------------------

Some rendering (and all viewer display) uses OpenGL. Off-screen GPU rendering
needs a GL context on a worker thread, per platform: ``OSGLContext`` with
backends ``OSGLContext_mac`` / ``_win`` / ``_x11`` / ``_wayland`` (the Linux
backends use the internal ``OSGLContext_glx_data`` / ``OSGLContext_egl_data``
helpers, and ``OSGLContext_xdg.h`` for XDG desktop integration).
``GPUContextPool`` pools these contexts so
threads can borrow one. GL entry points are loaded with **glad**
(``Global/GLIncludes.h``). The viewer widgets themselves are ``QOpenGLWidget``
subclasses (see :ref:`maint-gui`).

.. _maint-renderroi-example:

Worked example: the life of a ``renderRoI`` call
------------------------------------------------

To make the pipeline concrete, here is the actual path a single request takes
through ``EffectInstance::renderRoI`` (in ``EffectInstance.h`` and
``EffectInstanceRenderRoI.cpp``), in the order of the code. Method and enum
names below are the real ones, so you can follow along in the source.

Suppose the Viewer needs the visible region of a Blur node at frame 10.

1. **The call.** ``ViewerInstance::renderViewer_internal`` has installed the
   ``ParallelRenderArgs`` and run the request pass (see above), then calls
   ``blur->renderRoI(args, &planes)`` with a ``RenderRoIArgs`` describing time
   10, the view, the mip-map level, the region of interest and the desired
   planes and bit depth. The function returns a ``RenderRoIRetCode`` —
   ``eRenderRoIRetCodeOk``, ``eRenderRoIRetCodeAborted`` or
   ``eRenderRoIRetCodeFailed``. If the Blur is a render clone, the call is
   forwarded to the main instance. ``renderRoI`` then fetches its
   ``ParallelRenderArgs`` from the TLS.

2. **Region of definition.** ``getRegionOfDefinition_public`` gives the Blur's
   RoD for these args (unless the caller passed a precomputed one).

3. **Pass-through planes.** Planes that the Blur does not produce itself are
   fetched directly from its pass-through input with ``renderRoI``.

4. **Identity check.** ``isIdentity_public``: if the Blur is a pass-through for
   these args (e.g. size 0, or disabled), it does not render at all — it calls
   ``renderRoI`` on the identity input and returns that (the "Check if effect
   is identity" block).

5. **Transform concatenation.** If the preference is enabled, consecutive
   transform nodes are merged (``tryConcatenateTransforms``), so the image is
   resampled only once. Then the RoI is computed: if the effect supports
   tiles, the requested RoI is clipped to the image bounds; otherwise the RoI
   becomes the full image bounds.

6. **Cache lookup.** ``shouldCacheOutput`` decides whether the result goes to
   the cache (see :ref:`maint-cache`). The cache key is built from the node
   hash and the args, and ``getImageFromCacheAndConvertIfNeeded`` looks for an
   existing image (in draft mode, the full-quality image is looked up first);
   an image of a higher resolution can be downscaled. The image's **bitmap**
   records which pixels are already valid, so only the missing rectangles are
   rendered. A full hit does not return here: it still goes through steps 7
   and 8, finds nothing left to render, skips the inputs and the render, ends
   with ``eRenderRoIStatusImageAlreadyRendered`` and still waits for the pixels
   that other threads are rendering (step 11).

7. **Frames needed.** ``getFramesNeeded_public`` (or the result of the request
   pass) says which frames of which inputs the Blur needs.

8. **What is left to render.** For a cached image, an
   ``ImageBitMapMarker_RAII`` calls ``markImageAsBeingRendered``: it returns
   the rectangles that are neither rendered nor being rendered by another
   thread, and marks them as being rendered by this one. Only during a paint
   stroke, when the effect supports tiles and its inputs include a mask or
   have different regions of definition (``tryIdentityOptim``),
   ``optimizeRectsToRender`` splits these rectangles and marks the parts that
   are identity.

9. **Input images.** ``renderInputImagesForRoI`` asks for the regions of
   interest of the inputs (``getRegionsOfInterest_public``, unless the request
   pass already has them) and calls ``treeRecurseFunctor``, which calls
   ``renderRoI`` on each input for each frame needed. This recursion is what
   walks the graph toward the readers. The input images are kept in the TLS,
   where the render action finds them (``EffectInstance::getImage``).

10. **Render.** The output planes are allocated (in the cache, or not), and
    ``renderRoIInternal`` calls ``beginSequenceRender_public``, then
    ``tiledRenderingFunctor`` on each rectangle, then
    ``endSequenceRender_public``. The rectangles are rendered in parallel with
    ``QtConcurrent::mapped`` only if the effect's render thread safety is
    ``eRenderSafetyFullySafeFrame``, there is more than one rectangle and the
    render is not on the GPU; otherwise they are rendered one after the other.
    ``tiledRenderingFunctor`` calls ``renderHandler`` (``EffectInstance.cpp``),
    which calls ``render_public`` and so the effect's ``render`` action. For
    OpenFX nodes, ``OfxEffectInstance::render`` calls the plug-in's render
    action (see :ref:`maint-openfx`); for built-ins it runs the native C++ code.
    ``renderRoIInternal`` returns a ``RenderRoIStatusEnum``
    (``eRenderRoIStatusImageAlreadyRendered``, ``…ImageRendered``,
    ``…RenderFailed``, ``…RenderOutOfGPUMemory``).

11. **Concurrency: "being rendered elsewhere".** Before returning, the marker
    waits for the rectangles that another thread was rendering
    (``waitForPendingRegions`` → ``waitForImageBeingRenderedElsewhere``), and
    its destructor calls ``unmarkImageAsBeingRendered``. This is the mechanism
    that keeps concurrent renders from duplicating work. It also makes this wait
    a prime suspect for the CPU-idle stall in issue
    `#248 <https://github.com/NatronGitHub/Natron/issues/248>`_: a thread that
    blocks here on another thread's rectangle that never completes would sit
    idle forever (see :ref:`maint-issue-triage`).

12. **Return.** An aborted render returns ``eRenderRoIRetCodeAborted``; a failed
    one throws; a render that ran out of GPU memory is retried on the CPU.
    Otherwise the rendered pixels are already in the (cached) image, with its
    bitmap updated, and ``renderRoI`` returns ``eRenderRoIRetCodeOk``. The
    Viewer then converts the result into a ``FrameEntry`` texture, which has
    its own cache.

Throughout, the per-frame context (hashes captured up front, the abort flag,
statistics) is read from the thread-local ``ParallelRenderArgs``, not from live
node state. That is what keeps these steps consistent even if the user changes
a parameter mid-render — and why bypassing that TLS is a classic source of
render bugs.

.. toctree::
   :maxdepth: 1

   cache
