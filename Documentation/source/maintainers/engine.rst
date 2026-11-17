.. _maint-engine:

The Engine Module
=================

``Engine`` is the core library: everything except the GUI. This chapter is a
functional reference to its main subsystems and the files that implement them.

Nodes and the graph
-------------------

- ``Node`` (``Node.cpp`` — the largest file in the project, with
  ``NodeInputs.cpp``, ``NodeName.cpp``, ``NodeMain.cpp``, ``NodeOverlay.cpp``,
  ``NodePrivate.h``): the graph vertex. Owns inputs/outputs, name/label,
  knobs container, the ``NodeGuiI`` back-pointer, and the ``EffectInstance``.
  Node creation goes through ``CreateNodeArgs`` and the ``AppInstance`` /
  ``AppManager`` factory path.
- ``NodeCollection`` and ``NodeGroup`` (``NodeGroup.cpp``): containers of nodes.
  ``NodeGroup`` is both an effect and a collection, giving Natron recursive
  sub-graphs and PyPlugs. ``GroupInput``/``GroupOutput`` are the group's I/O
  proxies.
- ``Project`` (``Project.cpp``, ``ProjectPrivate.*``): the document root, a
  ``NodeCollection`` and a ``KnobHolder``. Owns project format, frame range,
  frame rate, views and color settings, plus autosave.

Effects
-------

- ``EffectInstance`` (``EffectInstance.cpp``, ``EffectInstanceRenderRoI.cpp``,
  ``EffectInstancePrivate.*``): the abstract behavior of a node and the entry
  point to rendering (:ref:`maint-rendering`). Defines the *actions* an effect
  supports — region of definition, region of interest, frames needed, get
  metadata, and render.
- ``OutputEffectInstance``: base for effects that can *drive* a render (it
  owns a ``RenderEngine``). Despite the name, it is the base of most effects:
  OpenFX effects, the Viewer, groups and the no-op nodes (see
  :ref:`maint-architecture`).
- OpenFX effects: ``OfxEffectInstance`` (through ``AbstractOfxEffectInstance``)
  wraps a plug-in; see :ref:`maint-openfx`.
- Built-in effects, by base class:

  - ``OutputEffectInstance``: ``ViewerInstance``, ``DiskCacheNode``,
    ``NoOpBase`` (and its subclasses ``Dot``, ``Backdrop``, ``GroupInput``,
    ``GroupOutput``), ``NodeGroup``;
  - ``NodeGroup``: ``WriteNode`` (which embeds an OFX writer plug-in) and
    ``TrackerNode``;
  - ``EffectInstance``: ``ReadNode`` (which embeds an OFX reader plug-in),
    ``RotoPaint`` (and its subclass ``RotoNode``), ``RotoSmear`` (internal,
    used by smear strokes), ``PrecompNode``, ``OneViewNode``,
    ``JoinViewsNode``.

  They are registered in ``AppManager::loadBuiltinNodePlugins()``; see
  :ref:`maint-builtin-node` to add one.

Parameters: the Knob system
---------------------------

Knobs are Natron's animated, expression-driven parameters. The hierarchy is:

- ``KnobI`` (``Knob.h``) — the interface. It derives from ``OverlaySupport``
  and ``std::enable_shared_from_this<KnobI>``.
- ``KnobHelper`` — the base implementation (animation curves, expressions,
  links, listeners).
- ``Knob<T>`` (``Knob.h``, implemented in ``KnobImpl.h``) — the class template
  that stores the values of type ``T``, with the typedefs ``KnobIntBase``,
  ``KnobDoubleBase``, ``KnobBoolBase`` and ``KnobStringBase``.
- Concrete knobs (``KnobTypes.h``/``KnobTypes.cpp``, ``KnobFile.*``):
  ``KnobInt``, ``KnobDouble``, ``KnobBool``, ``KnobChoice``, ``KnobColor``,
  ``KnobString``, ``KnobPath``, ``KnobFile``, ``KnobOutputFile``, ``KnobGroup``,
  ``KnobPage``, ``KnobButton``, ``KnobSeparator``, ``KnobParametric``,
  ``KnobTable``, ``KnobLayers``.
- ``KnobHolder`` (a ``QObject``) — anything that owns knobs (every
  ``EffectInstance``, the ``Project`` and the ``Settings``). ``KnobFactory``
  (``KnobFactory.cpp``) constructs them.
- ``KnobSignalSlotHandler`` — the ``QObject`` companion that emits a knob's
  signals.
- Related: ``Curve`` (``Curve.cpp``) and ``KeyFrame`` for animation;
  ``Interpolation.cpp`` for keyframe interpolation; ``StringAnimationManager``
  for animating string values.

How values, animation, links and expressions interact is explained in
:ref:`maint-knobs`.

Rendering, scheduling and caching
---------------------------------

Covered in depth in :ref:`maint-rendering`. Key files: ``EffectInstanceRenderRoI.cpp``,
``ParallelRenderArgs.*``, ``OutputSchedulerThread.*``, ``GenericSchedulerThread.*``,
``ThreadPool.*``, ``AbortableRenderInfo.*``, ``RenderStats.*``, and the cache
(``Cache.h``, ``CacheEntry.h``, ``LRUHashTable.h``, ``MemoryFile.*``,
``FrameEntry.*`` for the viewer texture cache; see :ref:`maint-cache`).

Images and color
----------------

- ``Image`` (``Image.cpp``, ``ImageConvert.cpp``, ``ImageCopyChannels.cpp``,
  ``ImageMaskMix.cpp``): the tiled floating-point image buffer, the fundamental
  data the pipeline moves around. ``ImageParams``, ``ImageKey`` and
  ``ImagePlaneDesc`` describe an image and its layer/channel layout.
- ``RectI`` / ``RectD``: integer and double rectangles (regions).
- Color management: ``Lut.cpp`` (color-space lookup tables) and the
  ``Color::Lut`` API, feeding into OpenColorIO integration.
- ``Texture`` / ``TextureRect``: GL textures for the viewer.

Roto and paint
--------------

Vector shapes and paint strokes: ``RotoContext`` (``RotoContext.cpp``),
``RotoItem``/``RotoLayer``/``RotoDrawableItem``, ``Bezier``/``BezierCP`` (cubic
Bézier shapes and control points), ``RotoStrokeItem`` (paint strokes),
``RotoPaint`` (the effect), ``RotoPaintInteract`` (viewer interaction),
``RotoSmear``, ``FeatherPoint`` and ``CoonsRegularization`` (feathering).
Rasterization uses **cairo**. ``RotoUndoCommand`` implements undo for roto edits.

Tracking
--------

The planar/point tracker: ``TrackerContext`` (``TrackerContext.cpp``,
``TrackerContextPrivate.*``), ``TrackMarker``, ``TrackerNode`` +
``TrackerNodeInteract`` (the built-in node and its overlay), and
``TrackerFrameAccessor`` (feeds pixels to the solver). The heavy lifting is done
by the bundled **libmv** (``mv::AutoTrack``), **ceres** and **openMVG**
libraries.

Application services
--------------------

- ``AppManager`` (``AppManager.cpp``, ``AppManagerPrivate.*``): the singleton;
  plug-in discovery/registry, OFX host, caches, settings, factories, and the
  startup sequence (see :ref:`maint-startup`).
- ``AppInstance`` (``AppInstance.cpp``): one document/session.
- ``Settings`` (``Settings.cpp`` — very large): the user preferences, themselves
  modeled as a ``KnobHolder`` so the preferences panel is built from knobs.
- ``Plugin`` / ``LibraryBinary``: plug-in metadata and dynamic loading.
- ``CLArgs`` (``CLArgs.cpp``): command-line parsing shared by both executables.
- ``TimeLine``: the current-frame model.
- ``StandardPaths``, ``ProcInfo``, ``MemoryInfo``, ``Log``: environment,
  process and logging utilities.
- ``OSGLContext`` (+ ``_mac``/``_win``/``_x11``/``_wayland`` backends, with the
  ``_xdg`` helper on Linux): off-screen OpenGL contexts for GPU rendering
  (on Linux the X11 and Wayland backends use the internal ``OSGLContext_glx_data``
  / ``OSGLContext_egl_data`` structures). ``GPUContextPool`` pools them.

The Python API
--------------

The classes prefixed ``Py`` (``PyNode``, ``PyParameter``, ``PyRoto``,
``PyTracker``, ``PyAppInstance``, ``PyNodeGroup``, ``PyGlobalFunctions``,
``PyExprUtils``) are the C++ side of Natron's Python API. They are thin,
scripting-friendly facades over the engine objects, exposed to Python through
Shiboken using ``typesystem_engine.xml``. See :ref:`maint-python`.

More on the Engine
------------------

The following chapters go deeper into the knob system, the project file format,
and how to add a built-in node.

.. toctree::
   :maxdepth: 1

   knobs
   serialization
   builtin-node
