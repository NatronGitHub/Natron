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

CMake picks up every ``.cpp`` and ``.h`` in ``Engine/`` and ``Gui/``, while
``Engine/Engine.pro`` and ``Gui/Gui.pro`` list them by name. When you add or
remove a source file, update the ``.pro`` file too;
``tools/utils/checkQmakeSources.py`` (run by the Linux CI job) reports any
mismatch.

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

Shiboken2 and PySide2 without CMake config files
------------------------------------------------

With Qt 5, CMake first looks for the Shiboken2 and PySide2 CMake config files
(``Shiboken2Config.cmake``, ``PySide2Config.cmake``; point CMake at them with
``Shiboken2_DIR``/``PySide2_DIR`` or ``CMAKE_PREFIX_PATH``). Some installs have
none: PySide2 built with ``setup.py``, such as MacPorts' ``py310-pyside2``, and
the PySide2 wheels used with a ``shiboken2_generator`` wheel from download.qt.io
(PyPI has no ``shiboken2_generator``, and its PySide2 wheels bundle their own
Qt, which Natron must then link too). If neither config package is found, CMake asks
``Python3_EXECUTABLE`` where the ``shiboken2``, ``shiboken2_generator`` and
``PySide2`` modules are (``cmake/PySide2FromPython.cmake``), and uses the
libraries, headers, typesystems and generator in their ``site-packages``
directories. This replaces the paths that the qmake build gets from
``config.pri`` (e.g. ``tools/jenkins/include/natron/OSX.pri``):

.. list-table::
   :header-rows: 1

   * - qmake (``config.pri``)
     - CMake
   * - ``SHIBOKEN=shiboken2-$$PYVER``
     - ``-DSHIBOKEN2_GENERATOR=/path/to/shiboken2`` (default: the binary in
       ``site-packages/shiboken2_generator``)
   * - ``INCLUDEPATH`` and ``LIBS`` under ``$$PYTHON_SITE_PACKAGES``
     - automatic, from ``-DPython3_EXECUTABLE=/path/to/python3.10``
   * - ``QMAKE_RPATHDIR`` under ``$$PYTHON_SITE_PACKAGES``
     - automatic in the build tree; installed binaries get theirs from the
       install rules
   * - ``TYPESYSTEMPATH``
     - automatic (``site-packages/PySide2/typesystems``)

For example, with MacPorts::

    cmake -S . -B build -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
        -DPython3_EXECUTABLE=/opt/local/bin/python3.10

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

macOS app bundle and install layout
-----------------------------------

On macOS, both builds produce ``App/Natron.app`` in the build tree, with
nearly the same contents: ``Info.plist`` generated from
``App/NatronInfo.plist.in`` (version, ``fr.inria.Natron`` bundle identifier,
``natronIcon.icns``, the ``.ntp``/``.nps``/``.nl`` document types), ``PkgInfo``
(``APPLNtrn``), the icons ``natronIcon.icns`` and ``natronProjectIcon_osx.icns``
in ``Contents/Resources``, and ``Contents/Resources/etc/fonts`` (``fonts.conf``
generated from ``Gui/Resources/etc/fonts/fonts.conf.in``, plus ``conf.d``).
The OpenColorIO configs are copied to ``Contents/Resources/OpenColorIO-Configs``
when they are available (qmake: ``OpenColorIO-Configs/`` in the source tree;
CMake: ``NATRON_OCIO_CONFIGS_DIR``). ``NatronRenderer``, ``natron-python`` and
``Tests`` are plain executables, outside the bundle.

The differences: the CMake build keeps the double quotes of the ``Info.plist``
XML declaration, which qmake's ``QMAKE_SUBSTITUTES`` strips (issue #1074);
qmake also copies ``fonts.conf.in`` and creates an empty
``Contents/Resources/empty.lproj``, which CMake does not. Neither reaches the
DMG: the installer script rebuilds the bundle from the executables,
``Info.plist`` and ``PkgInfo``.

``cmake --install`` supports two layouts, selected with
``NATRON_INSTALL_LAYOUT``:

- ``FHS`` (the default, on every platform): ``bin/Natron`` (``bin/Natron.app``
  on macOS), ``bin/NatronRenderer``, ``bin/natron-python``, and the
  ``.desktop``, AppStream, MIME and icon files in ``share/``, as before this
  option existed. In addition: on Linux, the bundled PyPlugs in
  ``share/Natron/Plugins`` (Natron finds them only when installed with
  ``--prefix /usr``) and, when ``NATRON_OCIO_CONFIGS_DIR`` is set, the
  OpenColorIO configs in ``share/OpenColorIO-Configs``; on Windows,
  ``fonts.conf`` and the OpenColorIO configs in ``Resources/``, the only place
  where Natron looks for them on Windows; on macOS, the configs and fonts are
  inside ``Natron.app``. Installed binaries
  keep the absolute RPATHs of the libraries they were linked with, so that
  they run in place (``-DCMAKE_INSTALL_RPATH_USE_LINK_PATH=OFF`` to drop them).
- ``PORTABLE``: the layout of the binary installers built by
  ``tools/jenkins/build-*-installer.sh``. On Linux and Windows: ``bin/``
  (executables), ``Resources/etc/fonts`` (``fonts.conf`` and ``conf.d``),
  ``Resources/OpenColorIO-Configs``, ``Resources/pixmaps`` (Linux PNG or
  Windows ICO icons), ``Resources/stylesheets/mainstyle.qss``,
  ``Plugins/PyPlugs`` and ``docs/natron/LICENSE.txt``. On macOS, the same
  inside ``Natron.app/Contents``: the executables in ``MacOS/``, everything
  else in ``Resources/`` and ``Plugins/PyPlugs``. Installed binaries only get
  relative RPATHs: ``$ORIGIN/../lib`` on Linux; ``@loader_path/../Frameworks``
  and the PySide/Shiboken directories in the bundled Python framework on
  macOS. The installer scripts still add the third-party libraries, Qt,
  Python, the OpenFX plug-ins, and on macOS the DMG background
  (``Gui/Resources/Images/splashscreen.*``).

Each destination can be overridden: ``NATRON_INSTALL_BUNDLEDIR``,
``NATRON_INSTALL_BINDIR``, ``NATRON_INSTALL_RESOURCESDIR``,
``NATRON_INSTALL_PYPLUGSDIR``, ``NATRON_INSTALL_OCIODIR``,
``NATRON_INSTALL_DOCDIR`` and ``NATRON_INSTALL_TESTSDIR`` (relative to
``CMAKE_INSTALL_PREFIX``; empty means "don't install").

The ``Tests`` binary is only installed by
``cmake --install <build> --component Tests``, to ``NATRON_INSTALL_TESTSDIR``
(``bin/``; with ``PORTABLE`` on macOS, ``Natron.app/Contents/MacOS``, where its
relative RPATHs resolve and where ``build-OSX-installer.sh`` runs it).

With ``PORTABLE``, a missing OpenColorIO config is an error, and so is an empty
``NATRON_OCIO_CONFIGS_DIR``, so that a release can't silently ship without
them; with ``FHS`` a missing config is a warning. Set
``NATRON_OCIO_CONFIGS`` to the configs to bundle, e.g.
``-DNATRON_OCIO_CONFIGS="blender;nuke-default"`` with the OCIO 2.0 configs,
which have no ``natron`` config (``build-natron.sh`` drops it the same way).

On macOS, the ``PORTABLE`` RPATHs point at the PySide/Shiboken directories of
the bundled Python framework, for the Python version CMake found (printed at
configure time). It must be the version the installer script bundles
(``PYVER``).

.. list-table:: qmake build → CMake option (bundle and install)
   :header-rows: 1

   * - qmake
     - CMake
   * - ``make install`` (``INSTALLS``)
     - ``-DNATRON_INSTALL_LAYOUT=FHS`` (the default)
   * - the copy commands of ``tools/jenkins/build-natron.sh``
     - ``-DNATRON_INSTALL_LAYOUT=PORTABLE``
   * - ``OpenColorIO-Configs/`` in the source tree
     - ``-DNATRON_OCIO_CONFIGS_DIR=<dir>`` (default: ``OpenColorIO-Configs/``
       in the source tree, if present)
   * - ``OCIO.files`` in ``OpenColorIO-Configs.pri``
     - ``-DNATRON_OCIO_CONFIGS="blender;natron;nuke-default"``

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
