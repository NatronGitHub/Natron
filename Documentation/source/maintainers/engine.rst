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
- ``OutputEffectInstance``: base for effects that *drive* a render (writers,
  the viewer).
- OpenFX effects: ``OfxEffectInstance`` wraps a plug-in; see
  :ref:`maint-openfx`.
- Built-in effects: ``ViewerInstance``, ``RotoPaint``, ``TrackerNode``,
  ``ReadNode``/``WriteNode`` (which themselves dispatch to OFX reader/writer
  plug-ins), ``Backdrop``, ``Dot``, ``PrecompNode``, ``DiskCacheNode``,
  ``OneViewNode``, ``JoinViewsNode``, ``NoOpBase`` subclasses.

Parameters: the Knob system
---------------------------

Knobs are Natron's animated, expression-driven parameters. The hierarchy is:

- ``KnobI`` (``Knob.h``) — the interface.
- ``KnobHelper`` — the base implementation (animation, expressions, listeners,
  cloning). ``KnobImpl.h`` provides the templated value machinery.
- Concrete knobs (``KnobTypes.h``/``KnobTypes.cpp``, ``KnobFile.*``):
  ``KnobInt``, ``KnobDouble``, ``KnobBool``, ``KnobChoice``, ``KnobColor``,
  ``KnobString``, ``KnobPath``, ``KnobFile``, ``KnobOutputFile``, ``KnobGroup``,
  ``KnobPage``, ``KnobButton``, ``KnobSeparator``, ``KnobParametric``,
  ``KnobTable``, ``KnobLayers``.
- ``KnobHolder`` — anything that owns knobs (every ``EffectInstance``, and
  ``Settings``). ``KnobFactory`` (``KnobFactory.cpp``) constructs them.
- ``KnobSignalSlotHandler`` — the ``QObject`` companion that emits a knob's
  signals (knobs are not ``QObject``).
- Related: ``Curve`` (``Curve.cpp``) and ``KeyFrame`` for animation;
  ``Interpolation.cpp`` for keyframe interpolation; ``StringAnimationManager``
  for animating string values.

Rendering, scheduling and caching
---------------------------------

Covered in depth in :ref:`maint-rendering`. Key files: ``EffectInstanceRenderRoI.cpp``,
``ParallelRenderArgs.*``, ``OutputSchedulerThread.*``, ``GenericSchedulerThread.*``,
``ThreadPool.*``, ``AbortableRenderInfo.*``, ``RenderStats.*``, and the cache
(``Cache.h``, ``CacheEntry.h``, ``LRUHashTable.h``, ``MemoryFile.*``,
``FrameEntry.*`` for the viewer texture cache).

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
  plug-in discovery/registry, OFX host, global cache, settings, factories.
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
