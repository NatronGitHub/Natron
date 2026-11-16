.. _maint-architecture:

Core Architecture
=================

Layering
--------

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
     - Third-party code built from source: ``ceres`` / ``openMVG`` / ``libmv``
       (tracker), ``Eigen``, ``hoedown``, ``libtess``, ``gflags`` / ``glog``, …
       (external dependencies such as Qt, Boost, cairo, OpenColorIO and
       OpenImageIO are *not* here — see :ref:`maint-building`).

The golden rule is that **dependencies only point downward**. ``Gui`` knows
about ``Engine``; ``Engine`` must never include a ``Gui`` header. This is what
lets ``NatronRenderer`` link ``Engine`` without ``Gui`` and run on a headless
render farm. The mechanism that makes it possible while still letting the engine
"talk to" the GUI is the abstract-interface pattern in :ref:`maint-design`.

The runtime object model
------------------------

At run time Natron is a tree of a few long-lived objects. Understanding this
ownership graph is the key to understanding the whole program:

``AppManager`` (singleton, reached through the ``appPTR`` macro)
    The one global object. It owns the process-wide state: the plug-in registry
    (all discovered OpenFX and built-in plug-ins), the OpenFX host
    (``OfxHost``), the global ``Cache``, the user ``Settings``, the
    ``KnobFactory``, and the list of open documents. ``GuiApplicationManager``
    is the GUI subclass that additionally owns window/shortcut/icon state. You
    will see ``appPTR->…`` calls throughout the code — that is this singleton.

``AppInstance`` (one per open project; ``QObject``)
    A single "document"/session. It owns exactly one ``Project``. The GUI
    subclass ``GuiAppInstance`` additionally owns the main window (``Gui``). A
    single ``AppManager`` can hold several ``AppInstance`` objects at once (for
    example when rendering multiple projects headless).

``Project`` (``KnobHolder`` + ``NodeCollection``)
    The root of the document. It *is* a ``NodeCollection`` (a container of
    nodes) and it *is* a ``KnobHolder`` (it has parameters — format, frame
    range, color settings, …). Saving a project serializes the ``Project`` and
    everything it contains.

``NodeCollection``
    A container of ``Node`` objects with the wiring between them. ``Project`` is
    the top-level collection; each ``NodeGroup`` (a ``Group`` node, and also
    each PyPlug) is a nested collection, which is how Natron supports
    sub-graphs. This composite structure means the graph is recursive: a node
    can itself contain a graph.

``Node`` (``QObject``)
    A vertex in the graph. A ``Node`` holds graph-level state: its position and
    appearance, its input connections, its label and script name, its
    knobs-container, a pointer to its GUI (through ``NodeGuiI``), and — most
    importantly — a pointer to its **effect instance** (``getEffectInstance()``
    / ``setEffect()``). The ``Node`` is the stable identity; the effect is the
    behavior.

``EffectInstance`` (``NamedKnobHolder``)
    The actual image-processing behavior of a node. This is where rendering
    happens. There are two families of effects:

    - ``OfxEffectInstance`` — wraps a loaded **OpenFX plug-in**. Most user-
      visible nodes are of this kind.
    - Native ``EffectInstance`` subclasses — the **built-in** nodes implemented
      directly in C++: ``ViewerInstance``, ``RotoPaint``, ``TrackerNode``,
      ``ReadNode``/``WriteNode``, ``NodeGroup``, ``Backdrop``, ``Dot``,
      ``PrecompNode``, ``GroupInput``/``GroupOutput``, ``OneViewNode``,
      ``JoinViewsNode``, ``DiskCacheNode`` and a few no-ops.

    ``OutputEffectInstance`` is the base for effects that drive rendering (the
    Viewer and Writers); ``ViewerInstance`` and ``NodeGroup`` both derive from
    it.

So the chain from application to pixels is:
``appPTR`` → ``AppInstance`` → ``Project`` (a ``NodeCollection``) →
``Node`` → ``EffectInstance`` → (OpenFX plug-in or built-in C++).

Why ``Node`` and ``EffectInstance`` are separate
------------------------------------------------

This split is deliberate and worth internalizing. The ``Node`` is a durable
graph object with a stable identity, connections and undo history. The
``EffectInstance`` is the possibly-replaceable behavior. Keeping them apart lets
Natron do things like reset or reload a plug-in, swap a Read node's decoder when
the file type changes, or run several render clones of the same effect
concurrently, all without disturbing the graph topology the user built.

The OpenFX host at the center
-----------------------------

Because most nodes are OpenFX plug-ins, a large part of the Engine exists to
*be a good OpenFX host*: to load plug-ins and read their descriptors, to present
Natron's images to them as OFX clips (``OfxClipInstance``), to present Natron's
knobs to them as OFX parameters (``OfxParamInstance``), to translate the OFX
render actions into Natron's rendering pipeline (``OfxImageEffectInstance``,
``OfxEffectInstance``), and to route overlay drawing and interaction
(``OfxOverlayInteract``). This host layer is split between the reusable OpenFX
support code in ``HostSupport``/``libs/OpenFX`` and the Natron-specific glue in
``Engine/Ofx*``. See :ref:`maint-openfx` for detail.

The rendering pipeline in one paragraph
---------------------------------------

Rendering is **pull-based**. An output effect (the Viewer or a Writer) asks its
input for a *region of interest* of an image at a given time, view and
resolution (mip-map level / proxy). That call — ``EffectInstance::renderRoI()``
— recurses upstream: each effect computes which region of *its* inputs it needs,
asks for them (again through ``renderRoI``), then renders its own output tile by
tile, possibly on many threads. Results are stored in the ``Cache`` keyed by a
hash of everything that can affect them, so unchanged sub-results are reused on
the next frame or the next parameter tweak. The whole recursive render is
coordinated by scheduler threads and carries per-render state through
thread-local storage (``ParallelRenderArgs``). This is the single most complex
part of the code and has its own chapter, :ref:`maint-rendering`.
