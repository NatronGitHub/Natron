.. _maint-codebase-map:

Map of the Code Base
====================

This chapter is a directory-level tour. Use it to find *where* something lives;
the later chapters explain *how* each part works.

Top-level directories
---------------------

``Engine/``
    The core, GUI-free library. Nodes, the parameter ("knob") system, the
    rendering pipeline, the RAM/disk cache, the OpenFX host glue, the built-in
    effects (Roto, RotoPaint, Tracker, Read, Write, Group, Backdrop, Dot,
    Viewer, Precomp, …), color management, image classes, the project model,
    animation curves, serialization, and the Python API (the ``Py*`` classes).
    This is by far the largest and most important module. It links against Qt
    (for ``QObject``, threads, mutexes and containers) but **not** against any
    GUI widgets.

``Gui/``
    The Qt desktop application: the node graph (``NodeGraph*``), the OpenGL
    viewer (``ViewerGL``, ``ViewerTab*``), the animation curve editor
    (``CurveEditor``, ``CurveWidget``) and dope sheet (``DopeSheet*``), the
    dockable parameter panels (``DockablePanel``, ``KnobGui*``), roto and
    tracker panels, the preferences panel, the script editor, and dozens of
    custom widgets. Everything here can depend on ``Engine``; nothing in
    ``Engine`` may depend on anything here.

``App/``
    ``NatronApp_main.cpp`` — the ``main()`` for the ``Natron`` GUI executable.

``Renderer/``
    ``NatronRenderer_main.cpp`` — the ``main()`` for the headless
    ``NatronRenderer`` executable.

``PythonBin/``
    ``python_main.cpp`` — the ``natron-python`` interpreter, a Python binary
    with the Natron modules available. It also doubles as Natron's Python
    package manager (see :ref:`maint-python`).

``Global/``
    Small shared headers used across all modules: ``Macros.h`` (namespace,
    version and compiler-diagnostic macros), ``Enums.h`` and ``GlobalDefines.h``
    (project-wide enums and typedefs), ``QtCompat.h`` (Qt5/Qt6 shims),
    ``GLIncludes.h`` (the ``glad`` OpenGL loader), ``KeySymbols.h`` (toolkit-
    independent key codes), and utility code (``StrUtils``, ``ProcInfo``,
    ``PythonUtils``, ``FStreamsSupport``).

``HostSupport/``
    Natron's OpenFX **host** support library — the C++ classes that implement
    the host side of the OpenFX plug-in contract, built on top of the reference
    OpenFX headers in ``libs/OpenFX``.

``libs/``
    Bundled third-party libraries built from source: ``OpenFX`` and
    ``OpenFX_extensions`` (the plug-in API), ``ceres``/``openMVG``/``libmv``
    (the tracker's solver and matcher), ``Eigen3`` (linear algebra headers),
    ``hoedown`` (Markdown), ``libtess`` (tessellation), ``gflags``/``glog``,
    ``qhttpserver`` (internal docs server), ``SequenceParsing`` and
    ``google-breakpad`` (crash reporting).

``Tests/``
    Google Test / Google Mock unit tests.

``CrashReporter/``, ``CrashReporterCLI/``, ``BreakpadClient/``
    The out-of-process crash reporter (built on Google Breakpad) and its GUI/CLI
    front-ends. Natron renders in a separate process from the reporter so that a
    crash in one does not take down the other.

``Documentation/``
    The Sphinx sources for this manual (see :ref:`maint-contributing` for how the
    manual is organized and built).

``tools/``
    Release and CI engineering: ``jenkins`` and ``buildmaster`` build farms,
    ``MacPorts`` and ``MINGW-packages`` port files, ``docker`` images,
    ``appimage`` packaging, and helper scripts such as ``genStaticDocs.sh``.

Naming conventions you will see everywhere
------------------------------------------

Once you recognize a handful of file-name suffixes, the ~600 files in
``Engine`` and ``Gui`` become navigable:

``FooFwd.h``
    Forward declarations and smart-pointer typedefs for a whole module.
    ``Engine/EngineFwd.h`` and ``Gui/GuiFwd.h`` are the master catalogs — start
    there when you meet an unfamiliar type.

``FooI.h``
    An *abstract interface* (pure virtual class), e.g. ``NodeGuiI``,
    ``KnobGuiI``, ``OpenGLViewerI``, ``NodeGraphI``, ``DockablePanelI``. These
    are the seams between ``Engine`` and ``Gui`` (see :ref:`maint-design`).

``FooPrivate.h`` / ``FooPrivate.cpp``
    The private implementation of a class ``Foo`` (the "PIMPL" idiom). The
    public ``Foo.h`` holds only a pointer to ``FooPrivate``.

``FooSerialization.h``
    The ``boost::serialization`` description of how ``Foo`` is written to and
    read from a project file.

``PyFoo.h`` / ``PyFoo.cpp``
    The Python-facing wrapper class for ``Foo`` (exposed through Shiboken).

``OfxFoo.h`` / ``OfxFoo.cpp``
    The OpenFX host implementation for ``Foo`` (e.g. ``OfxEffectInstance``,
    ``OfxClipInstance``, ``OfxParamInstance``).

Numbered GUI files (``Gui20.cpp``, ``ViewerTab30.cpp``, ``NodeGraph45.cpp`` …)
    Several large GUI classes are split across many ``.cpp`` files purely to
    keep individual translation units to a manageable size; they all implement
    the same class declared in the corresponding ``.h``. There is no semantic
    meaning to the numbers beyond grouping related methods.
