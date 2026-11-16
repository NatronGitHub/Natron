.. _maint-overview:

Overview: What Natron Is, at the Code Level
===========================================

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
``App`` and ``Renderer`` directories are thin ``main()`` wrappers that produce
the two shipped executables.

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

The two executables
-------------------

Natron ships two binaries built from the same libraries:

- ``Natron`` (from ``App/NatronApp_main.cpp``) — the full GUI application. It
  links ``Gui`` + ``Engine`` + ``HostSupport``.

- ``NatronRenderer`` (from ``Renderer/NatronRenderer_main.cpp``) — a headless
  renderer for batch and render-farm use. It links ``Engine`` + ``HostSupport``
  **without** ``Gui``.

A third binary, ``natron-python`` (``PythonBin/python_main.cpp``), is a Python
interpreter with the Natron modules preloaded, used for scripting and for
generating documentation. Being a full Python interpreter, it also serves as
Natron's Python package manager (``natron-python -m pip install …``); see
:ref:`maint-python`.

Because both applications share the Engine, any change you make in ``Engine``
must build and behave correctly *without* the GUI. If you find yourself wanting
to call into ``Gui`` from ``Engine``, that is a signal to use one of the
abstract interfaces instead (see :ref:`maint-design`).

A note on lineage
-----------------

Natron was originally developed at INRIA by Alexandre Gauthier-Foichat and is
now maintained by the community (the ``NatronGitHub`` organization). The code
base is long-lived and pragmatic: it predates C++11 in places (it was written
against C++98 and gradually modernized), it carries compatibility shims for
several compilers and three operating systems, and it has been migrated once
already from Qt 4 to Qt 5. Understanding this history explains many of the
idioms you will meet — the hand-rolled smart-pointer typedefs, the compiler
diagnostic pragmas, and the ``QtCompat`` shims among them.
