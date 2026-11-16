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

    RenderRoIRetCode EffectInstance::renderRoI(const RenderRoIArgs& args, ...)

``RenderRoIArgs`` says *what* to produce: the time, the view, the render scale
(mip-map level — a power-of-two downscale used for proxy and for zoomed-out
viewing), the region of
interest (a ``RectI``), and the desired components/bit-depth. ``renderRoI``:

1. Computes the effect's **region of definition** (RoD) for these args
   (``getRegionOfDefinition``) — the maximal area the effect can produce.
2. Looks the result up in the :ref:`cache <maint-cache>`. On a hit it returns
   immediately.
3. On a miss, asks the effect which parts of its **inputs** it needs
   (``getRegionsOfInterest``) and at which **frames** (``getFramesNeeded`` —
   temporal effects like FrameBlend or retime pull several frames), and calls
   ``renderRoI`` recursively on those inputs.
4. Checks ``isIdentity`` — if the effect is a pass-through for these args (e.g. a
   disabled node, or a transform with identity matrix) it just forwards the
   input image instead of rendering.
5. Splits its output region into tiles and calls the effect's ``render()``
   action on them, potentially on many threads, then stores the result in the
   cache.

The relevant *actions* an effect implements mirror the OpenFX action set:
``getRegionOfDefinition``, ``getRegionsOfInterest``, ``getFramesNeeded``,
``isIdentity``, ``getPreferredMetadata`` (components, bit depth, premultiplication state, pixel
aspect, frame rate), ``beginSequenceRender``/``endSequenceRender``, and
``render``. ``renderRoIInternal`` (in ``EffectInstanceRenderRoI.cpp``) is the
private core that wraps ``render`` with thread-safety and cache handling; the
public ``renderRoI`` orchestrates recursion and identity/cache decisions.

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
``ViewerParallelRenderArgsSetter``) and reached through the ``TLSHolder`` /
``AppTLS`` machinery.

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
- ``OutputSchedulerThread`` (``OutputSchedulerThread.cpp``): schedules the
  rendering of a *range* of frames for an output effect, spawning
  ``RenderThreadTask`` workers, ordering finished frames, and (for the Viewer)
  feeding them to the display. ``RenderEngine`` owns the scheduler for an output
  node.
- The Viewer has its own "current frame" scheduler
  (``ViewerCurrentFrameRequestScheduler``) so that scrubbing the timeline
  coalesces and cancels stale requests.
- ``ThreadPool`` (``ThreadPool.cpp``) and Qt's ``QtConcurrent`` provide the
  worker threads that render tiles in parallel; ``AbortableThread`` /
  ``AbortableRenderInfo`` let a render be cancelled promptly when the user acts
  again.

Aborting renders
----------------

Because feedback must be immediate, any in-flight render can be **aborted**.
``AbortableRenderInfo`` tracks whether a given render has been cancelled;
render code polls it and unwinds cleanly. When you write a long-running loop in
render code, check the abort flag periodically — a render that ignores aborts
makes the whole UI feel stuck.

.. _maint-cache:

The cache
---------

The cache is what makes real-time playback possible: once a frame (or a tile of
an intermediate result) is computed, it is kept so the next request is instant.

- ``Cache`` (``Cache.h``): a templated, thread-safe, least-recently-used cache
  with a two-tier backing store — an in-RAM portion and an on-disk portion.
  ``LRUHashTable.h`` is the eviction structure.
- Disk entries are memory-mapped files (``MemoryFile.*``, ``TileCacheFile``),
  so large cached images live on disk but are accessed as memory.
- Entries are keyed by a 64-bit **hash** (``Hash64``) computed from everything
  that can affect the result: the node's parameters, its inputs' hashes, the
  time, view, scale and region. If any input changes, the hash changes and the
  old entry is simply not found — this is how invalidation stays correct without
  explicit dirty-tracking.
- ``CacheEntry`` / ``CacheEntryHolder`` are the stored objects. There are
  distinct caches/entries for full images (``Image`` + ``ImageParams``) and for
  the Viewer's display textures (``FrameEntry`` + ``FrameKey`` +
  ``FrameParams``, which hold the 8-bit or 32-bit texture actually uploaded to
  OpenGL).

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

Worked example: the life of a ``renderRoI`` call
------------------------------------------------

To make the pipeline concrete, here is the actual path a single request takes
through ``EffectInstance::renderRoI`` (in ``EffectInstance.h`` and
``EffectInstanceRenderRoI.cpp``). Method and enum names below are the real ones,
so you can follow along in the source.

Suppose the Viewer needs the visible region of a Blur node at frame 10.

1. **The call.** The Viewer's ``ViewerInstance`` calls
   ``blur->renderRoI(args, …)`` with a ``RenderRoIArgs`` describing time 10, the
   view, the mip-map level, the region of interest and the desired
   components/bit-depth. The function returns a ``RenderRoIRetCode`` —
   ``eRenderRoIRetCodeOk``, ``eRenderRoIRetCodeAborted`` or
   ``eRenderRoIRetCodeFailed``.

2. **Region of definition.** ``renderRoI`` computes the Blur's RoD for these
   args and clips the requested RoI to it, so nothing outside the image is
   requested.

3. **Identity check.** It calls ``isIdentity_public``. If the Blur is a
   pass-through for these args (e.g. size 0, or disabled), it does not render at
   all — it recurses into the identity input's ``renderRoI`` and returns that.
   (See the "Check if effect is identity" block in
   ``EffectInstanceRenderRoI.cpp``.)

4. **Cache lookup.** It builds the cache key from the node hash and args and
   looks for an existing image (``downscaleImage`` / ``fullscaleImage``). On a
   full hit it returns immediately. On a partial hit, the image's **bitmap**
   records which tiles are already valid, so only the missing rectangles are
   rendered.

5. **Concurrency: "being rendered elsewhere".** Before rendering the missing
   rectangles, the effect marks them via ``markImageAsBeingRendered``. If another
   thread is already producing an overlapping region, this thread calls
   ``waitForImageBeingRenderedElsewhere`` instead of rendering it twice, and
   ``unmarkImageAsBeingRendered`` when done. This is the mechanism that keeps
   concurrent renders from duplicating work. It also makes this wait a prime
   suspect for the CPU-idle stall in issue
   `#248 <https://github.com/NatronGitHub/Natron/issues/248>`_: a thread that
   blocks here on another thread's tile that never completes would sit idle
   forever (see :ref:`maint-issue-triage`).

6. **Input images.** For the rectangles it must render, the effect asks which
   inputs and frames it needs (``getRegionsOfInterest``, ``getFramesNeeded``) and
   pulls them with ``renderInputImagesForRoI`` — which recurses into the upstream
   nodes' ``renderRoI``. This recursion is what walks the graph toward the
   readers; ``treeRecurseFunctor`` drives it.

7. **Tiling and the render action.** If the effect ``tilesSupported``, the RoI
   is split into tiles rendered in parallel; otherwise the RoI is expanded to the
   full bounds. ``renderRoIInternal`` wraps the effect's ``render`` action with
   thread-safety and returns a ``RenderRoIStatusEnum``
   (``eRenderRoIStatusImageAlreadyRendered``, ``…ImageRendered``,
   ``…RenderFailed``, ``…RenderOutOfGPUMemory``). For OpenFX nodes the ``render``
   action calls into the plug-in (see :ref:`maint-openfx`); for built-ins it runs
   the native C++ code.

8. **Store and return.** The freshly rendered tiles are written into the cached
   image (updating its bitmap) and ``renderRoI`` returns ``eRenderRoIRetCodeOk``.
   The Viewer then uploads the result as a ``FrameEntry`` texture.

Throughout, the per-frame context (hashes captured up front, the abort flag,
statistics) is read from the thread-local ``ParallelRenderArgs``, not from live
node state. That is what keeps steps 2–7 consistent even if the user changes a
parameter mid-render — and why bypassing that TLS is a classic source of render
bugs.
