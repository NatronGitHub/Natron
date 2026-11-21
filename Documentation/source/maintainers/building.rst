.. _maint-building:

Building Natron from Source
===========================

Natron is built with **CMake**. The entry point is ``CMakeLists.txt`` at the
repository root, with one ``CMakeLists.txt`` per module. It requires CMake ≥
3.21, which libs/OpenFX requires too (its policies are those of CMake 3.31).
The CI builds and tests Natron with CMake on Linux, macOS and Windows.

The **qmake** build is **deprecated** (`#1181
<https://github.com/NatronGitHub/Natron/issues/1181>`_). Its entry point is
``Project.pro``, a ``TEMPLATE = subdirs`` project pulling in every module and
bundled library; shared configuration lives in ``global.pri`` and
``libs.pri``, and the platform dependency locations in ``config.pri`` (examples:
``config-macports.pri``, ``config-homebrew.pri``,
``tools/jenkins/include/natron/*.pri``). On RB-2.6 it still works, and the
release scripts in ``tools/jenkins`` still use it, until they move to CMake. It
will be removed in a future release, first on RB-2.7, once installers built
with CMake have been shown to be equivalent. Running qmake prints a warning to
that effect.

CMake picks up every ``.cpp`` and ``.h`` in ``Engine/`` and ``Gui/``, while
``Engine/Engine.pro`` and ``Gui/Gui.pro`` list them by name. As long as the
qmake build exists, when you add or remove a source file, update the ``.pro``
file too;
``tools/utils/checkQmakeSources.py`` (run by the Linux CI job) reports any
mismatch. No per-PR CI job builds with qmake any more: only these source lists
are checked, so a broken ``.pri`` or ``DEFINES`` setting would only show up
when the release scripts build an installer (a nightly qmake job would catch
it earlier).

.. note::

   **C++ standard:** RB-2.6 builds with **C++17 for Qt5 and C++20 for Qt6**,
   and so do the plug-ins built with it, so the code must compile with both.
   CMake (``NATRON_QT6``), qmake (``QT_MAJOR_VERSION``, in ``global.pri``) and
   ``tools/jenkins/compiler-common.sh`` (``QT_VERSION_MAJOR``) pick the
   standard from the Qt version. ``Global/Macros.h`` fails compilation with
   ``#error "Natron 2.6+ requires C++17"`` if the standard is older.
   RB-2.7 is Qt6-only and uses C++20 everywhere.

.. note::

   If you touch the source layout — add a file, add a module, rename something —
   update the relevant ``CMakeLists.txt`` and, while the deprecated qmake build
   exists, the relevant ``*.pro``/``*.pri`` as well: the release scripts still
   build with qmake on RB-2.6.

Platform-specific, step-by-step instructions (including how to obtain the
third-party dependencies) live at the repository root in ``INSTALL_LINUX.md``,
``INSTALL_MACOS.md``, ``INSTALL_WINDOWS.md`` and ``INSTALL_FREEBSD.md``. This
chapter explains the *structure* of the build so those instructions make sense.

Modules and their build order
-----------------------------

The qmake ``Project.pro`` declares the module dependency graph explicitly
(the CMake build gets the same order from the targets' link dependencies).
Reading it is the fastest way to understand how the pieces fit together::

    glog.depends     = gflags
    ceres.depends    = glog gflags
    libmv.depends    = gflags ceres
    openMVG.depends  = ceres
    Engine.depends   = libmv openMVG HostSupport libtess ceres md4c
    Renderer.depends = Engine
    Gui.depends      = Engine qhttpserver
    Tests.depends    = Gui Engine
    App.depends      = Gui Engine

So the build order is: the bundled libraries first (``gflags``, ``glog``,
``ceres``, ``libmv``, ``openMVG``, ``qhttpserver``, ``md4c``, ``libtess``),
then ``HostSupport``, then ``Engine``, then the front-ends (``Renderer``,
``Gui``), then ``App``, ``Tests`` and ``PythonBin``.

Third-party dependencies
------------------------

Some dependencies are **bundled** in ``libs/`` and built from source as part of
the project: ``ceres``, ``openMVG`` and ``libmv`` (the tracker), ``Eigen3``
(headers), ``gflags`` and ``glog``, ``md4c`` (CommonMark Markdown → HTML for node docs and tooltips),
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

The deprecated qmake build targets Qt 5. With a Qt 6 ``qmake`` it compiles
(``QT += openglwidgets`` under ``equals(QT_MAJOR_VERSION, 6)``), but qmake
doesn't run shiboken, for Qt 5 either (``tools/jenkins/build-natron.sh``
generates the bindings before running qmake), so the Qt 6 Python bindings are
only generated by the CMake build. qmake won't get further Qt 6 work: use CMake
for Qt 6. Completing and stabilizing Qt 6 support is an active work item; see
:ref:`maint-qt6`.

Shiboken2 and PySide2 without CMake config files
------------------------------------------------

With Qt 5, CMake first looks for the Shiboken2 and PySide2 CMake config files
(``Shiboken2Config.cmake``, ``PySide2Config.cmake``; point CMake at them with
``Shiboken2_DIR``/``PySide2_DIR`` or ``CMAKE_PREFIX_PATH``). Some installs have
none: PySide2 built with ``setup.py``, such as MacPorts' ``py311-pyside2``, and
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
     - automatic, from ``-DPython3_EXECUTABLE=/path/to/python3.11``
   * - ``QMAKE_RPATHDIR`` under ``$$PYTHON_SITE_PACKAGES``
     - automatic in the build tree; installed binaries get theirs from the
       install rules
   * - ``TYPESYSTEMPATH``
     - automatic (``site-packages/PySide2/typesystems``)

For example, with MacPorts::

    git submodule update --init --recursive   # --recursive: libs/SequenceParsing/tinydir
    cmake -S . -B build -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
        -DPython3_EXECUTABLE=/opt/local/bin/python3.11

If shiboken2 fails to parse the libc++ headers of a recent Xcode SDK
(``use of undeclared identifier '__builtin_clzg'``), use an older SDK with
``export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk``, exported
for both the configure and ``cmake --build``: shiboken runs during the build.

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
  The CMake equivalents of the qmake ``CONFIG`` flags are listed below.

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

CMake build options
-------------------

These cache variables reproduce the qmake ``CONFIG`` flags of ``global.pri``
and ``libs.pri``. Their defaults give a plain developer build; the installers
(``tools/jenkins/build-natron.sh``) turn on the release flavour, OpenMP and
``NATRON_NO_ASSERTIONS``, but not ``NATRON_FAST_MATH``: the release builds use
``-O3`` without fast math.

``NATRON_BUILD_CONFIG`` (empty by default)
   The release flavour shown in the version string, splash screen and About
   window: empty for a ``Devel`` build, or ``SNAPSHOT``, ``ALPHA``, ``BETA``,
   ``RC``, ``STABLE`` or ``CUSTOM``. It defines the matching
   ``NATRON_CONFIG_*`` macro read by ``Global/Macros.h``. ``CUSTOM`` is a
   ``STABLE`` build that shows "Natron for *name*" instead of the version, and
   needs ``NATRON_CUSTOM_BUILD_USER_NAME``. Only the build farm should set it.
``NATRON_BUILD_NUMBER`` (``0``)
   The build number of an ``ALPHA``, ``BETA`` or ``RC`` release, e.g. ``2``
   for RC2. It defines ``NATRON_BUILD_NUMBER``. ``-DBUILD_NUMBER=…``, the
   qmake variable, sets its initial value.
``NATRON_CUSTOM_BUILD_USER_NAME`` (empty)
   The name shown by a ``CUSTOM`` build. With ``NATRON_BUILD_CONFIG=CUSTOM``
   only, it defines ``NATRON_CUSTOM_BUILD_USER_TOKEN`` to the bare name, which
   ``Global/Macros.h`` turns into a string, so the name can't contain a comma,
   a quote, a backslash, a parenthesis, ``;``, ``#``, ``//`` or ``/*``:
   CMake stops with an error. The old ``-DBUILD_USER_NAME=…`` still sets its
   initial value.
``NATRON_USE_OPENMP`` (``ON`` with GCC except on macOS, ``OFF`` otherwise, as in ``libs.pri``)
   Compile the bundled ceres and openMVG, which the tracker uses, with OpenMP
   (``CERES_USE_OPENMP``, ``OPENMVG_USE_OPENMP``, ``OpenMP::OpenMP_CXX``).
   Apple clang has no OpenMP runtime: install MacPorts' ``libomp``, which is
   found automatically in ``/opt/local/include/libomp`` and
   ``/opt/local/lib/libomp``. For another libomp, set ``OpenMP_ROOT`` to its
   prefix (e.g. ``-DOpenMP_ROOT=$(brew --prefix libomp)``).
   When OpenMP is not found, the default only gives a warning and builds
   without it; an explicit ``-DNATRON_USE_OPENMP=ON`` stops the configure.
``NATRON_NO_ASSERTIONS`` (``OFF``)
   Define ``NDEBUG``, ``QT_NO_DEBUG``, ``QT_NO_DEBUG_OUTPUT`` and
   ``QT_NO_WARNING_OUTPUT`` in every configuration, which removes
   ``assert()`` and the ``qDebug()``/``qWarning()`` output. Release and
   RelWithDebInfo builds already define ``NDEBUG``.
``NATRON_FAST_MATH`` (``OFF``)
   Compile everything with ``-O3 -ffast-math`` (MSVC: ``/O2 /fp:fast``),
   which overrides the optimization level of the build type and allows
   floating-point computations that do not conform to IEEE 754. qmake's
   ``CONFIG+=fast`` uses ``-Ofast``, which clang 19 and later deprecate in
   favor of ``-O3 -ffast-math``. The installers use neither.
``NATRON_SANITIZE`` (empty)
   Sanitizers passed to ``-fsanitize=``, comma-separated: ``address``,
   ``thread``, ``undefined``, ``address,undefined``… Use it with a ``Debug``
   or ``RelWithDebInfo`` build. Unlike qmake, it does not force a debug
   build with ``-O1``.
``NATRON_COVERAGE`` (``OFF``)
   Instrument for code coverage with ``--coverage`` (GCC and Clang). Use it
   with a ``Debug`` build.

The following table maps each qmake flag that ``tools/jenkins/build-natron.sh``
passes, and the other ``global.pri`` flags, to CMake.

.. list-table:: qmake CONFIG flag → CMake option
   :header-rows: 1
   :widths: 40 60

   * - qmake
     - CMake
   * - ``CONFIG+=debug``, ``release``, ``relwithdebinfo`` (``COMPILE_TYPE``)
     - ``-DCMAKE_BUILD_TYPE=Debug``, ``Release``, ``RelWithDebInfo``
   * - ``CONFIG+=snapshot``, ``alpha``, ``beta``, ``RC``, ``stable``
     - ``-DNATRON_BUILD_CONFIG=SNAPSHOT``, ``ALPHA``, ``BETA``, ``RC``, ``STABLE``
   * - ``BUILD_NUMBER=N``
     - ``-DNATRON_BUILD_NUMBER=N``
   * - ``CONFIG+=custombuild BUILD_USER_NAME=name``
     - ``-DNATRON_BUILD_CONFIG=CUSTOM -DNATRON_CUSTOM_BUILD_USER_NAME=name``
   * - ``CONFIG+=openmp``, ``CONFIG-=openmp``
     - ``-DNATRON_USE_OPENMP=ON``, ``OFF``
   * - ``CONFIG+=noassertions``
     - ``-DNATRON_NO_ASSERTIONS=ON``
   * - ``CONFIG+=fast``
     - ``-DNATRON_FAST_MATH=ON``
   * - ``CONFIG+=addresssanitizer``, ``threadsanitizer``
     - ``-DNATRON_SANITIZE=address``, ``thread``
   * - ``CONFIG+=coverage``
     - ``-DNATRON_COVERAGE=ON``
   * - ``CONFIG+=python3 PYTHON_CONFIG=pythonX.Y-config``
     - Always Python 3; choose it with ``-DPython3_EXECUTABLE=…``
   * - ``CONFIG+=enable-cairo``
     - Always on: cairo is required
   * - ``CONFIG+=enable-breakpad``, ``CONFIG-=enable-breakpad``
     - Not available: Breakpad is being replaced by sentry-native (#1183)
   * - ``CONFIG+=enable-osmesa OSMESA_PATH=… LLVM_PATH=…``
     - None: Natron itself doesn't use OSMesa (only openfx-misc's Shadertoy
       does), and the flag has no effect on the qmake build either
   * - The GCC-version-dependent flags of ``global.pri``
       (``-ftemplate-depth-1024``, ``-fpermissive``, ``-Wno-deprecated-declarations``,
       ``-Wno-int-in-bool-context``, ``-Wno-expansion-to-defined``)
     - None: the CMake build needs a C++17 compiler (GCC 8 or later) and sets
       its own warning flags in ``CMakeLists.txt``. ``-fpermissive`` was a
       workaround for Boost 1.65 and earlier (MrKepzie/Natron#1659)
   * - ``-spec macx-clang``, ``macx-g++``, ``win32-g++``;
       ``QMAKE_CC``, ``QMAKE_CXX``, ``QMAKE_LINK``
     - ``-DCMAKE_C_COMPILER=…``, ``-DCMAKE_CXX_COMPILER=…`` (or the ``CC`` and
       ``CXX`` environment variables), and a generator such as
       ``-G "MSYS Makefiles"`` or ``-G Ninja``
   * - ``QMAKE_OBJECTIVE_CC``, ``QMAKE_OBJECTIVE_CXX``
     - ``-DCMAKE_OBJCXX_COMPILER=…``
   * - ``CONFIG+=x86_64``, ``arm64`` (macOS)
     - ``-DCMAKE_OSX_ARCHITECTURES=x86_64``, ``arm64``
   * - ``QMAKE_MACOSX_DEPLOYMENT_TARGET=X.Y``
     - ``-DCMAKE_OSX_DEPLOYMENT_TARGET=X.Y``
   * - ``CFLAGS``, ``CXXFLAGS`` in the environment
     - The same variables, read at the first configure, or
       ``-DCMAKE_C_FLAGS=…``, ``-DCMAKE_CXX_FLAGS=…``
   * - ``CONFIG+=silent``
     - The default; ``cmake --build build --verbose`` shows the commands
   * - ``CONFIG+=enforce-gcc8``
     - None: it only bypasses the ``#error`` for GCC 8.1 and 8.2 in
       ``Gui/TimeLineGui.cpp`` (issue #279), which were replaced by GCC 8.3
       in 2019 and are not used by any SDK. Pass
       ``-DCMAKE_CXX_FLAGS=-DENFORCE_GCC8`` if you must.

Release builds
--------------

The installers are built by ``tools/jenkins/launchBuildMain.sh``, which calls
``build-plugins.sh``, ``build-natron.sh`` and ``build-<OS>-installer.sh``
(macOS: through ``tools/tart``; Linux: in the SDK Docker image; Windows:
``build_installer.yml``). ``build-natron.sh`` builds Natron with CMake:

- it configures with ``-DNATRON_INSTALL_LAYOUT=PORTABLE``, the release flavour
  (``NATRON_BUILD_CONFIG``, ``NATRON_BUILD_NUMBER``,
  ``NATRON_CUSTOM_BUILD_USER_NAME``), ``NATRON_USE_OPENMP`` (``OFF`` only
  with Apple clang), ``NATRON_NO_ASSERTIONS`` (except for ``COMPILE_TYPE=debug``),
  the OpenColorIO configs it downloaded, and the compilers, Qt and Python
  chosen by ``common.sh`` and ``compiler-common.sh``. ``COMPILE_TYPE``
  ``release``, ``relwithdebinfo`` and ``debug`` become the ``Release``,
  ``RelWithDebInfo`` (with ``-O3``) and ``Debug`` build types;
- it runs ``cmake --install`` (plus ``--component Tests``) into a staging
  directory, and copies the result to the layout the installer scripts read
  in ``$TMP_BINARIES_PATH``: ``bin/`` (on macOS with ``Info.plist`` and
  ``PkgInfo``), ``Resources/``, ``PyPlugs/``, ``docs/natron/``, and on macOS
  the DMG background. The installer scripts are unchanged;
- the Python bindings are generated by the CMake build: the shiboken2 commands
  of the qmake build are not run;
- the unit tests run from ``bin/``, except on macOS, where the ``Tests`` of
  the build tree is run (the installed one only finds its libraries inside the
  final bundle; ``build-OSX-installer.sh`` runs it again from there).

``NATRON_EXTRA_CMAKE_FLAGS`` adds options, separated by spaces, e.g.
``NATRON_EXTRA_CMAKE_FLAGS=-DNATRON_FAST_MATH=ON``. Two differences with the
former qmake release builds are intentional: there is no fast math, because
``-ffinite-math-only`` defeats the NaN and infinity checks (see #1277; the
qmake path no longer adds ``CONFIG+=fast`` either, and
``-DNATRON_FAST_MATH=ON`` brings it back), and there is no Breakpad crash
reporter (#1183), so ``launchBuildMain.sh`` sets ``DISABLE_BREAKPAD=1``. The C++ standard is the one ``CMakeLists.txt`` sets;
a ``-std=`` flag in ``$CXX`` is ignored.

On RB-2.6, ``NATRON_BUILD_SYSTEM=qmake`` selects the deprecated qmake build
(and ``NATRON_EXTRA_QMAKE_FLAGS``), until the CMake-built installers are shown
to be equivalent (#1181). ``tools/compare-builds.sh`` compares two builds: the
file lists, the resources, ``Info.plist``, the linked libraries and rpaths of
every binary, and with ``-r`` the version strings and the Python modules.

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
