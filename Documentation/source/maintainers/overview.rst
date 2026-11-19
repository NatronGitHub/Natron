.. _maint-overview:

Overview and Architecture
=========================

Natron is a **node-based, non-destructive digital compositor** for film and
video: an open-source counterpart to Nuke, After Effects or Fusion. From a
maintainer's point of view it is best understood as three things layered on top
of each other:

1. **An OpenFX host.** The heart of Natron is a complete implementation of the
   `OpenFX <http://openeffects.org/>`_ image-effect plug-in API (OFX v1.4).
   Most of the "nodes" a user sees — blur, transform, color correction, readers,
   writers — are OpenFX plug-ins loaded at run time from shared libraries.
   Natron hosts them: it loads plug-ins, describes clips and parameters to them,
   asks them to render regions of images, and draws their overlays.

2. **A render engine.** On top of the host sits a demand-driven, multi-threaded,
   tile-based rendering engine with an aggressive RAM/disk cache. When the user
   moves the playhead or changes a parameter, Natron computes only the regions
   of only the images that are actually needed, reusing cached results wherever
   the inputs have not changed.

3. **A graphical application.** Around the engine is a Qt desktop application:
   the node graph, the OpenGL viewer, the animation curve editor and dope sheet,
   the parameter panels, roto/paint and tracking tools, and a Python scripting
   environment.

These three layers map almost directly onto the top-level source directories:
``HostSupport`` and ``libs/OpenFX`` (the OFX host), ``Engine`` (nodes, rendering,
cache, built-in effects, the Python API), and ``Gui`` (the Qt application). The
``App``, ``Renderer`` and ``PythonBin`` directories are thin ``main()`` wrappers
that produce the executables.

What makes Natron a compositor
------------------------------

A few product features drive most of the architectural decisions, and it helps
to keep them in mind when reading the code:

- **32-bit float, linear, multi-channel pipeline.** Images are stored as
  floating-point RGBA (and arbitrary extra layers/channels), color-managed
  through `OpenColorIO <https://opencolorio.org/>`_. This is why the ``Image``
  class, the ``ImagePlaneDesc`` (layer/channel descriptor) and the color
  ``Lut`` code are central and performance-critical.

- **A node graph with animation.** The document is a directed graph of
  ``Node`` objects. Almost every value in the graph — every *knob* — can be
  animated with keyframes and driven by Python expressions. This is why the
  parameter ("knob") system and the ``Curve``/animation code are so large.

- **Interactivity with real-time feedback.** Everything the user does produces
  immediate feedback in the viewer. This drives the demand-driven renderer, the
  cache, proxy (downscaled) rendering, and the separation of rendering from the
  GUI thread.

- **Headless operation.** The same engine must run without any GUI for
  command-line and render-farm use (``NatronRenderer``). This requirement is the
  single most important force shaping the architecture: **the Engine must not
  depend on the Gui.** The abstract interface pattern described in
  :ref:`maint-design` exists to enforce exactly this.

- **Extensibility.** Users extend Natron with OpenFX plug-ins, with *PyPlugs*
  (node groups saved as Python), and with Python callbacks and panels. The
  Python binding layer (Shiboken/PySide) and the ``Py*`` classes in ``Engine``
  and ``Gui`` exist for this.

The executables
---------------

Natron ships three binaries:

- ``Natron`` (from ``App/NatronApp_main.cpp``) — the full GUI application. It
  links ``Gui`` + ``Engine`` + ``HostSupport``. When the command line asks for
  background mode, it runs headless with a plain ``AppManager`` instead of a
  ``GuiApplicationManager``.

- ``NatronRenderer`` (from ``Renderer/NatronRenderer_main.cpp``) — a headless
  renderer for batch and render-farm use. It links ``Engine`` + ``HostSupport``
  **without** ``Gui``.

- ``natron-python`` (from ``PythonBin/python_main.cpp``) — a plain Python
  interpreter (``Py_Main``) set up with the same Python environment as Natron.
  It does not link the Engine, so the ``NatronEngine`` module is not available
  in it. Its main use is as Natron's Python package manager
  (``natron-python -m pip install …``); see :ref:`maint-python`.

Because ``Natron`` and ``NatronRenderer`` share the Engine, any change you make in ``Engine``
must build and behave correctly *without* the GUI. If you find yourself wanting
to call into ``Gui`` from ``Engine``, that is a signal to use one of the
abstract interfaces instead (see :ref:`maint-design`).

.. _maint-architecture:

Core architecture
-----------------

Layering
~~~~~~~~

Natron is built as a stack of layers with a strict dependency direction. The
table lists them from the highest level (the executables) down to the bundled
libraries; **each layer may depend only on the layers below it.**

.. list-table::
   :header-rows: 1
   :widths: 22 26 52

   * - Layer
     - Directories
     - Responsibility
   * - Applications
     - ``App/`` · ``Renderer/`` · ``PythonBin/``
     - The three ``main()`` entry points: ``Natron`` (GUI), ``NatronRenderer``
       (headless), and ``natron-python`` (interpreter).
   * - GUI
     - ``Gui/``
     - The Qt desktop application: node graph, OpenGL viewer, parameter panels,
       curve editor / dope sheet, roto & tracker tools.
   * - Engine
     - ``Engine/``
     - The GUI-free core: nodes, knobs (parameters), the render pipeline, the
       cache, the built-in effects, color, images, the project model, and the
       Python API.
   * - OpenFX host
     - ``HostSupport/`` · ``libs/OpenFX``
     - The implementation of the OpenFX host contract that loads and drives
       plug-ins (most nodes are OFX plug-ins).
   * - Bundled libraries
     - ``libs/``
     - Third-party code built from source (listed in
       :ref:`maint-codebase-map`). External dependencies such as Qt, Boost,
       cairo, OpenColorIO and OpenImageIO are *not* here — see
       :ref:`maint-building`.

The diagram shows the CMake targets of these layers and what each one links.
``natron-python`` is a plain Python interpreter (``Py_Main``): it shares a few
``Global/`` helpers with Natron but does not link the Engine. ``HostSupport``
is compiled from the sources of ``libs/OpenFX``.

.. mermaid::
   :alt: Natron links NatronGui; NatronGui and NatronRenderer link
         NatronEngine; NatronEngine links HostSupport, which is compiled from
         the libs/OpenFX sources, embeds Python, and links the bundled ceres,
         libmv, openMVG, hoedown and libtess; NatronGui also links qhttpserver;
         natron-python links only Python. NatronGui implements the interfaces
         that NatronEngine declares in its *I.h headers.
   :caption: Layers and executables. Solid arrows: "links". Dotted arrows:
             HostSupport is compiled from libs/OpenFX; the GUI implements the
             Engine's ``*I.h`` interfaces.

   flowchart TB
       subgraph apps ["Executables"]
           App["App/<br/>Natron (GUI)"]
           Renderer["Renderer/<br/>NatronRenderer (headless)"]
           PythonBin["PythonBin/<br/>natron-python"]
       end
       Gui["Gui/<br/>NatronGui"]
       Engine["Engine/<br/>NatronEngine"]
       HostSupport["HostSupport/<br/>HostSupport"]
       OpenFX["libs/OpenFX<br/>OFX headers and host support sources"]
       Python(["Python"])
       subgraph bundled ["Bundled libraries (libs/)"]
           EngineLibs["ceres, libmv, openMVG,<br/>hoedown, libtess"]
           qhttpserver["qhttpserver"]
       end

       App --> Gui
       Gui --> Engine
       Gui --> qhttpserver
       Renderer --> Engine
       Engine --> HostSupport
       Engine --> EngineLibs
       Engine --> Python
       PythonBin --> Python
       HostSupport -. "compiled from" .-> OpenFX
       Gui -. "implements NodeGuiI, KnobGuiI, ..." .-> Engine

The golden rule is that **dependencies only point downward**. ``Gui`` knows
about ``Engine``; ``Engine`` must never include a ``Gui`` header. This is what
lets ``NatronRenderer`` link ``Engine`` without ``Gui`` and run on a headless
render farm. The mechanism that makes it possible while still letting the engine
"talk to" the GUI is the abstract-interface pattern in :ref:`maint-design`.

The runtime object model
~~~~~~~~~~~~~~~~~~~~~~~~

At run time Natron is a tree of a few long-lived objects. Understanding this
ownership graph is the key to understanding the whole program:

``AppManager`` (singleton, reached through the ``appPTR`` macro)
    The one global object. It owns the process-wide state
    (``Engine/AppManagerPrivate.h``): the plug-in registry (all discovered
    OpenFX and built-in plug-ins), the OpenFX host (``OfxHost``), the three
    image caches (see :ref:`maint-cache`), the user ``Settings``, the
    ``KnobFactory``, and the list of open documents (``AppInstance`` objects).
    ``GuiApplicationManager`` is the GUI subclass that additionally owns
    window/shortcut/icon state. You will see ``appPTR->…`` calls throughout the
    code — that is this singleton.

``AppInstance`` (one per open project; ``QObject``)
    A single "document"/session. It owns exactly one ``Project``. The GUI
    subclass ``GuiAppInstance`` additionally owns the main window (``Gui``). A
    single ``AppManager`` can hold several ``AppInstance`` objects at once (for
    example when rendering multiple projects headless).

``Project`` (``KnobHolder`` + ``NodeCollection``)
    The root of the document. It *is* a ``NodeCollection`` (a container of
    nodes) and it *is* a ``KnobHolder`` (it has parameters — format, frame
    range, color settings, …). Saving a project serializes the ``Project`` and
    everything it contains (see :ref:`maint-serialization`).

``NodeCollection``
    A container of ``Node`` objects with the wiring between them. ``Project`` is
    the top-level collection; each ``NodeGroup`` (a ``Group`` node, and also
    each PyPlug) is a nested collection, which is how Natron supports
    sub-graphs. This composite structure means the graph is recursive: a node
    can itself contain a graph.

``Node`` (``QObject``)
    A vertex in the graph. A ``Node`` holds graph-level state: its position and
    appearance, its input connections, its label and script name, a pointer to
    its GUI (through ``NodeGuiI``), and — most importantly — its **effect
    instance** (``getEffectInstance()``). The ``Node`` is the stable identity;
    the effect is the behavior.

``EffectInstance`` (a ``NamedKnobHolder``, so it owns the node's knobs)
    The actual image-processing behavior of a node. This is where rendering
    happens. There are two families of effects:

    - ``OfxEffectInstance`` — wraps a loaded **OpenFX plug-in**. Most user-
      visible nodes are of this kind.
    - The **built-in** nodes implemented directly in C++, listed in the next
      section.

So the chain from application to pixels is:
``appPTR`` → ``AppInstance`` → ``Project`` (a ``NodeCollection``) →
``Node`` → ``EffectInstance`` → (OpenFX plug-in or built-in C++).

Ownership follows the rule given in :ref:`maint-design`: each parent holds its
children through a ``shared_ptr`` (``AppManager`` its ``AppInstance`` objects,
a ``NodeCollection`` its ``Node`` objects, a ``Node`` its ``EffectInstance``,
a ``KnobHolder`` its knobs), and children point back to their parent with a
``weak_ptr``.

The effect class hierarchy
~~~~~~~~~~~~~~~~~~~~~~~~~~

``OutputEffectInstance`` (``Engine/OutputEffectInstance.h``) derives from
``EffectInstance`` and adds what an effect needs to *drive* a render: a
``RenderEngine`` and its scheduler threads (see :ref:`maint-rendering`). More
classes derive from it than just the Viewer and the writers:

- ``OfxEffectInstance`` derives from ``AbstractOfxEffectInstance``, which
  derives from ``OutputEffectInstance`` (``Engine/OfxEffectInstance.h``), so
  that any OFX plug-in, including an OFX writer, can drive a render.
- ``ViewerInstance``, ``DiskCacheNode`` and ``NoOpBase`` derive from
  ``OutputEffectInstance``. ``Dot``, ``Backdrop``, ``GroupInput`` and
  ``GroupOutput`` derive from ``NoOpBase``.
- ``NodeGroup`` derives from both ``OutputEffectInstance`` and
  ``NodeCollection`` (``Engine/NodeGroup.h``). ``WriteNode`` and
  ``TrackerNode`` derive from ``NodeGroup``: they are groups with an internal
  graph.
- ``ReadNode``, ``RotoPaint`` (and its subclass ``RotoNode``), ``RotoSmear``
  (internal, used by smear strokes), ``PrecompNode``, ``OneViewNode`` and
  ``JoinViewsNode`` derive from ``EffectInstance`` directly.

``ReadNode`` and ``WriteNode`` are registered only when
``NATRON_ENABLE_IO_META_NODES`` is set (it is, in ``Global/Macros.h``), and the
Viewer only when Natron is not running in background mode
(``AppManager::loadBuiltinNodePlugins`` in ``Engine/AppManager.cpp``).

Why ``Node`` and ``EffectInstance`` are separate
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This split is deliberate and worth internalizing. The ``Node`` is a durable
graph object with a stable identity, connections and undo history. The
``EffectInstance`` is the possibly-replaceable behavior. Keeping them apart lets
Natron do things like reset or reload a plug-in, swap a Read node's decoder when
the file type changes, or run several render clones of the same effect
concurrently, all without disturbing the graph topology the user built.

The OpenFX host at the center
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Because most nodes are OpenFX plug-ins, a large part of the Engine exists to
*be a good OpenFX host*: to load plug-ins and read their descriptors, to present
Natron's images to them as OFX clips (``OfxClipInstance``), to present Natron's
knobs to them as OFX parameters (``OfxParamInstance``), to translate the OFX
render actions into Natron's rendering pipeline (``OfxImageEffectInstance``,
``OfxEffectInstance``), and to route overlay drawing and interaction
(``OfxOverlayInteract``). This host layer is split between the reusable OpenFX
support code in ``libs/OpenFX/HostSupport`` and the Natron-specific glue in
``Engine/Ofx*``. See :ref:`maint-openfx` for detail.

The rendering pipeline in one paragraph
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Rendering is **pull-based**. An output effect (the Viewer or a Writer) asks its
input for a *region of interest* of an image at a given time, view and
resolution (mip-map level / proxy). That call — ``EffectInstance::renderRoI()``
— recurses upstream: each effect computes which region of *its* inputs it needs,
asks for them (again through ``renderRoI``), then renders its own output,
possibly split into rectangles rendered on several threads. Results are stored
in the cache under a hash of everything that can affect them, so unchanged
sub-results are reused on the next frame or the next parameter tweak. The whole
recursive render is coordinated by scheduler threads and carries per-render
state through thread-local storage (``ParallelRenderArgs``). This is the single
most complex part of the code and has its own chapter, :ref:`maint-rendering`.

.. _maint-startup:

Startup sequence
----------------

``Natron`` and ``NatronRenderer`` build an ``AppManager`` (``Natron`` builds a
``GuiApplicationManager`` unless it runs in background mode) and call
``AppManager::load()`` (``loadW()`` on Windows). The order of what follows
matters when you add anything that runs at startup (``Engine/AppManager.cpp``
unless noted):

1. ``load()`` sets the locale, stores the command line and calls
   ``loadFromArgs()``.
2. ``loadFromArgs()`` creates the OpenGL context pool and calls
   ``initializeOpenGLFunctionsOnce()`` (before the ``QApplication`` exists, see
   the comment there), then ``initializeQApp()``, which creates the
   ``QApplication`` (or ``QCoreApplication``). It then installs Natron's own
   ``ThreadPool`` as the global ``QThreadPool`` (if ``QT_CUSTOM_THREADPOOL`` is
   defined), sets ``FONTCONFIG_PATH`` to the bundled fontconfig files if it is
   not set, and only then calls ``initPython()``, and finally
   ``loadInternal()``.

   ``initPython()`` sets up the Python environment, registers the built-in
   ``NatronEngine`` module (and ``NatronGui`` in the GUI) with
   ``PyImport_AppendInittab``, and starts the interpreter (see
   :ref:`maint-python`).
3. ``loadInternal()`` sets the application name, starts logging and the crash
   reporter client if Natron was launched by the crash reporter, creates and
   restores the ``Settings``, then calls ``initGui()``.
4. ``initGui()`` is virtual. In headless mode, ``AppManager::initGui()`` calls
   ``loadInternalAfterInitGui()`` directly. In the GUI,
   ``GuiApplicationManager::initGui()`` (``Gui/GuiApplicationManager.cpp``)
   shows the splash screen, updates the fontconfig cache on a worker thread,
   starts the documentation server, and enters the Qt event loop:
   ``loadInternalAfterInitGui()`` runs later, from the slot
   ``onFontconfigCacheUpdateFinished()``.
5. ``loadInternalAfterInitGui()`` creates the three caches (sized from the
   settings), restores or clears the disk caches, sets the OpenFX host
   properties, and calls ``loadAllPlugins()``.
6. ``loadAllPlugins()`` registers the built-in nodes
   (``loadBuiltinNodePlugins()``), loads the OpenFX plug-ins
   (``OfxHost::loadOFXPlugins()``, see :ref:`maint-openfx`), loads the PyPlugs
   and runs the ``init.py`` scripts (``loadPythonGroups()``), restores the
   per-plug-in settings, and calls ``onAllPluginsLoaded()``.
7. Back in ``loadInternalAfterInitGui()``, ``newAppInstance()`` creates the
   first ``AppInstance`` (a ``GuiAppInstance`` in the GUI) and calls its
   ``load()``, which creates the ``Project``, runs the Python commands given on
   the command line, and then opens the project, runs the script or starts the
   writers that the command line asks for (``AppInstance::load`` in
   ``Engine/AppInstance.cpp``).

In background mode, ``loadInternalAfterInitGui()`` then resets the project and
quits.

A note on lineage
-----------------

Natron was originally developed at INRIA by Alexandre Gauthier-Foichat and is
now maintained by the community (the ``NatronGitHub`` organization). The code
base is long-lived and pragmatic: it predates C++11 in places (it was written
against C++98 and gradually modernized), it carries compatibility shims for
several compilers and three operating systems, it has been migrated once
already from Qt 4 to Qt 5, and it is being ported to Qt 6 (see
:ref:`maint-qt6`). Understanding this history explains many of the idioms you
will meet — the hand-rolled smart-pointer typedefs, the compiler diagnostic
pragmas, and the ``QtCompat`` shims among them.
