.. _maint-building:

Building Natron from Source
===========================

Natron has **two parallel build systems**. Both are checked in and both are
kept working; which one you use depends on your platform and taste.

- **qmake** — the traditional build. The entry point is ``Project.pro`` at the
  repository root, which is a ``TEMPLATE = subdirs`` project pulling in every
  module and bundled library. Shared configuration lives in ``global.pri`` and
  ``libs.pri``; platform dependency locations are in ``config-macports.pri`` and
  ``config-homebrew.pri``.

- **CMake** — a newer, increasingly preferred build. The entry point is
  ``CMakeLists.txt`` at the root, with one ``CMakeLists.txt`` per module. It
  requires CMake ≥ 3.16.

.. note::

   **C++17 is required for every build**, not just the CMake one:
   ``Global/Macros.h`` fails compilation with ``#error "Natron 2.6+ requires
   C++17"`` if the standard is older. Configure your toolchain accordingly
   regardless of which build system you use.

.. note::

   If you touch the source layout — add a file, add a module, rename something —
   you must update **both** build systems (the relevant ``*.pro``/``*.pri`` and
   the relevant ``CMakeLists.txt``). A change that only updates one will break
   the build on the other platforms.

Platform-specific, step-by-step instructions (including how to obtain the
third-party dependencies) live at the repository root in ``INSTALL_LINUX.md``,
``INSTALL_MACOS.md``, ``INSTALL_WINDOWS.md`` and ``INSTALL_FREEBSD.md``. This
chapter explains the *structure* of the build so those instructions make sense.

Modules and their build order
-----------------------------

``Project.pro`` declares the module dependency graph explicitly. Reading it is
the fastest way to understand how the pieces fit together::

    glog.depends     = gflags
    ceres.depends    = glog gflags
    libmv.depends    = gflags ceres
    openMVG.depends  = ceres
    Engine.depends   = libmv openMVG HostSupport libtess ceres
    Renderer.depends = Engine
    Gui.depends      = Engine qhttpserver
    Tests.depends    = Gui Engine
    App.depends      = Gui Engine

So the build order is: the bundled libraries first (``gflags``, ``glog``,
``ceres``, ``libmv``, ``openMVG``, ``qhttpserver``, ``hoedown``, ``libtess``),
then ``HostSupport``, then ``Engine``, then the front-ends (``Renderer``,
``Gui``), then ``App``, ``Tests`` and ``PythonBin``.

Third-party dependencies
------------------------

Some dependencies are **bundled** in ``libs/`` and built from source as part of
the project: ``ceres``, ``openMVG`` and ``libmv`` (the tracker), ``Eigen3``
(headers), ``gflags`` and ``glog``, ``hoedown`` (Markdown → HTML for node docs),
``libtess`` (polygon tessellation for roto), ``qhttpserver`` (the internal
documentation web server), ``SequenceParsing`` (image sequence detection) and
``google-breakpad`` (crash reporting). With CMake, ``-DNATRON_SYSTEM_LIBS=ON``
uses system copies of some of these instead.

Other dependencies must be present on the system: **Qt** (5 or 6), **Boost**
(notably ``boost::serialization``), **Python 3** plus **Shiboken/PySide**,
**cairo** (roto/paint rasterization), **OpenColorIO**, and OpenGL. The reader
and writer plug-ins pull in **OpenImageIO** and **FFmpeg**, but those live in
the separate ``openfx-io`` repository, not here.

Qt 5 and Qt 6
-------------

The CMake build already exposes a Qt version switch::

    option(NATRON_QT6 "use Qt6" OFF)

- With ``-DNATRON_QT6=OFF`` (default) it builds against **Qt 5.15**,
  **Shiboken2** and **PySide2**.
- With ``-DNATRON_QT6=ON`` it builds against **Qt 6.3+**, **Shiboken6** and
  **PySide6**, and additionally requires the ``OpenGLWidgets`` component
  (``QOpenGLWidget`` moved into its own module in Qt 6).

The qmake build does **not** yet have an equivalent switch and targets Qt 5.
Completing and stabilizing Qt 6 support is an active work item; see
:ref:`maint-qt6`.

Two compile definitions set in the CMake build are worth knowing about because
they affect how you must write code:

- ``QT_NO_CAST_FROM_ASCII`` — you cannot implicitly build a ``QString`` from a
  ``const char*``. Natron treats all strings as UTF-8, so wrap literals in
  ``QString::fromUtf8("…")`` (or the ``tr()`` machinery for translatable text).
- ``QT_NO_DEBUG_OUTPUT`` (release builds) — ``qDebug()`` output is compiled out
  of release builds.

Build type, sanitizers and debugging
------------------------------------

- CMake defaults to ``RelWithDebInfo`` when no build type is given; a ``Debug``
  build additionally defines ``DEBUG``.
- ``DEBUG`` builds enable **floating-point exception trapping** at startup (see
  ``Global/FloatingPointExceptions.h`` and the ``main()`` functions) so that a
  stray ``NaN`` or division by zero aborts immediately instead of silently
  propagating through the image pipeline.
- The qmake build supports ``CONFIG+=addresssanitizer`` for an
  AddressSanitizer build (see the messages at the bottom of ``Project.pro``),
  and ``CONFIG+=enable-breakpad`` to build with the Breakpad crash reporter.

Code style and the pre-commit hook
----------------------------------

Natron enforces a consistent C++ style with **astyle**. The exact invocation is
pinned in ``.git-hooks/pre-commit``::

    astyle -p -H -f -j -z2 -c -k3 -U -A8

The hook runs on every staged ``.c``/``.cpp``/``.h`` file under ``Global/``,
``Engine/``, ``Gui/``, ``Readers/`` and ``Writers/`` and **rejects the commit**
if the file is not already formatted. Install it once per clone::

    cd Natron
    mkdir -p .git/hooks
    ln -s ../../.git-hooks/pre-commit .git/hooks/pre-commit

If the hook complains, re-format and re-stage the offending file::

    astyle -p -H -f -j -z2 -c -k3 -U -A8 -n path/to/File.cpp
    git add path/to/File.cpp

A ``.clang-format`` file is also provided for editor integration, but astyle is
the authoritative formatter used by the hook.

Verifying a build
-----------------

The ``Tests`` module builds a Google Test / Google Mock suite
(``Curve_Test``, ``Lut_Test``, ``Image_Test``, ``Hash64_Test``,
``KnobFile_Test``, ``Tracker_Test``, ``FileSystemModel_Test``,
``OSGLContext_Test``). With CMake,
tests are registered with CTest (``enable_testing()``) and built unless you pass
``-DNATRON_BUILD_TESTS=OFF``. Run them after building to confirm your toolchain
and your change are sane before opening a pull request.
