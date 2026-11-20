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
    ``python_main.cpp`` — the ``natron-python`` interpreter: a plain Python
    interpreter set up with Natron's Python environment (it does not link the
    Engine). It doubles as Natron's Python package manager (see
    :ref:`maint-python`).

``Global/``
    Small shared headers used across all modules: ``Macros.h`` (namespace,
    version and compiler-diagnostic macros), ``Enums.h`` and ``GlobalDefines.h``
    (project-wide enums and typedefs), ``QtCompat.h`` (Qt5/Qt6 shims),
    ``GLIncludes.h`` (the ``glad`` OpenGL loader), ``KeySymbols.h`` (toolkit-
    independent key codes), and utility code (``StrUtils``, ``ProcInfo``,
    ``PythonUtils``, ``FStreamsSupport``).

``HostSupport/``
    Only the qmake project of Natron's OpenFX **host** support library
    (``HostSupport.pro``, ``pch.h``). The library — the C++ classes that
    implement the host side of the OpenFX plug-in contract — is built from
    ``libs/OpenFX/HostSupport`` plus the parametric parameters of
    ``libs/OpenFX_extensions``. CMake builds openfx's own ``OfxHost`` target
    (``add_subdirectory(libs/OpenFX EXCLUDE_FROM_ALL)`` in the top-level
    ``CMakeLists.txt``, with Natron's ``OFX_*`` options).

``libs/``
    Bundled third-party libraries built from source: ``OpenFX`` and
    ``OpenFX_extensions`` (the plug-in API), ``ceres``/``openMVG``/``libmv``
    (the tracker's solver and matcher), ``Eigen3`` (linear algebra headers),
    ``md4c`` (Markdown), ``libtess`` (tessellation), ``gflags``/``glog``,
    ``qhttpserver`` (internal docs server), ``SequenceParsing`` and
    ``google-breakpad`` (crash reporting).

``Tests/``
    Google Test / Google Mock unit tests.

``CrashReporter/``, ``CrashReporterCLI/``, ``BreakpadClient/``
    The out-of-process crash reporter (built on Google Breakpad) and its GUI/CLI
    front-ends. The reporter starts Natron as a child process
    (``CrashReporter/CallbacksManager.cpp``), so that it survives a crash of
    Natron and can send the dump. Only the qmake build builds it (see
    :ref:`maint-subsystems`).

``Documentation/``
    The Sphinx sources for this manual (see :ref:`maint-contributing` for how the
    manual is organized and built).

``Shiboken/``
    ``typesystem_widgets.xml``, an empty placeholder typesystem left over from
    Shiboken 1.2 (PySide 1). The release script still adds this directory to
    the Shiboken search path; the CMake build doesn't.

``cmake/``
    CMake helper modules (for example ``PySide2FromPython.cmake``).

``.github/``
    The GitHub Actions workflows (CI, installers, forward merges).

``.git-hooks/``
    The ``pre-commit`` hook that runs astyle (see :ref:`maint-contributing`).

``tools/``
    Release and CI engineering: ``jenkins`` (the release build scripts) and
    ``buildmaster`` build farms, ``MacPorts`` and ``MINGW-packages`` port files,
    ``docker`` images (the Linux SDK), ``tart`` (macOS build virtual machines),
    ``utils`` (helper scripts, including the Python binding helpers
    ``sourceList.py`` and ``sourceCleanup.py``), ``valgrind`` suppressions, and
    ``genStaticDocs.sh``. ``appimage``, ``homebrew`` and ``travis`` are legacy
    (last changed in 2021–2022, and only mentioned by old sections of the
    ``INSTALL_*.md`` files).

The file-name suffixes used in ``Engine`` and ``Gui`` (``FooFwd.h``,
``FooI.h``, ``FooPrivate.h``, ``FooSerialization.h``, ``PyFoo.h``,
``OfxFoo.h``, numbered ``Gui20.cpp`` files) are explained in
:ref:`maint-naming`.
