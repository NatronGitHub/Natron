.. _maint-github-builds:

How the GitHub Builds Work
==========================

Natron is built, tested and packaged on GitHub Actions. This chapter explains
what each workflow of ``.github/workflows`` on RB-2.6 does, when it runs, and
how to run it by hand. The rules behind it (three builds per pull request,
weekly runs, macOS informative, no COMMERCIAL builds) are summed up in
:doc:`guidelines`; the workflow files themselves have the details, in their
header comments.

The same rules hold for the plug-in repositories (openfx, openfx-supportext,
openfx-misc, openfx-io, openfx-arena, openfx-gmic): hosted runners are scarce,
macOS ones above all, so CI is kept small.

.. contents::
   :local:
   :depth: 2

.. _maint-ci:

Overview
--------

.. list-table:: The workflows on RB-2.6
   :header-rows: 1
   :widths: 25 30 45

   * - Workflow (file)
     - When
     - What
   * - *Tests* (``ci.yml``)
     - pull requests that are ready for review, pushes to ``master``,
       ``RB-*`` and ``v*`` tags; Sunday 03:17 UTC; by hand
     - builds Natron with CMake and runs ``ctest`` on Linux, Windows and
       macOS
   * - *Linux packages* (``linux_packages.yml``, ``linux_sdk_stage.yml``)
     - Sunday 04:47 UTC; by hand; pre-release tags
     - the Rocky Linux 9 SDK image, the Linux portable archives (x86_64 and
       aarch64), Natron-Tests on them, the ``natron_testing`` pre-release
   * - *macOS x86_64 DMG* (``macos_x86_64.yml``, ``macos_x86_64_sdk.yml``)
     - Sunday 05:17 UTC; by hand; pre-release tags
     - the MacPorts SDK, the x86_64 DMG, Natron-Tests on it
   * - *Build Installer* (``build_installer.yml``)
     - Sunday 06:17 UTC; by hand; ``v*`` tags
     - the Windows portable tree with Setup.exe, Natron-Tests on the
       installed Natron
   * - *Project Python round trip* (``project_python_roundtrip.yml``)
     - Sunday 13:37 UTC; by hand
     - the Python project format against the ``.ntp``, on the packages
   * - *Build pacman repo* (``build_pacman_repo.yml``)
     - by hand; ``v*`` tags
     - the Windows package repository (``tools/MINGW-packages``)
   * - *Render soak* (``render_soak.yml``)
     - by hand
     - hours of renders, to catch stalls (#248)
   * - *Windows ARM64* (``windows_arm64.yml``)
     - by hand
     - native Windows on ARM64 (MSYS2 CLANGARM64) and its unit tests
   * - *Forward-merge RB-2.6 into RB-2.7* (``forward_merge.yml``)
     - pushes to RB-2.6 and RB-2.7; by hand
     - keeps the forward-merge pull request open

No workflow builds Qt 6 on RB-2.6: Qt 6 is built by RB-2.7's CI (see
:doc:`qt6-migration`). No workflow builds a COMMERCIAL (non-GPL) Natron
(``NATRON_LICENSE=COMMERCIAL`` stays a manual option of the
``tools/jenkins`` scripts). The macOS arm64 DMG is not built on GitHub yet
(see `macOS`_).

Per pull request: ``ci.yml``
----------------------------

The *Tests* workflow has three jobs. Each one builds Natron with CMake and
runs ``ctest`` (the ``Tests`` google-test binary, the Python-driven
``NatronRenderer`` tests and the GUI tests, see :ref:`maint-tests`):

- **Linux** (``unix_test``): Ubuntu 24.04 with its Qt 5, PySide2 5.15.13 and
  Python 3.12 (pinned: Ubuntu 26.04 no longer ships PySide2), gcc, ``make``.
  Before building, it checks that the qmake project files list the same
  sources as the CMake build (``tools/utils/checkQmakeSources.py``) and runs
  the unit tests of the Python tools (``python3 -m unittest discover -s
  tools/utils -p 'test_*.py'``). The tests use the ``natron_testing``
  builds of openfx-io and openfx-misc (Ubuntu 24.04 zips) and
  OpenColorIO-Configs 2.5.
- **Windows** (``win-test``): ``windows-latest`` with MSYS2 UCRT64 and the stock
  MSYS2 packages (Qt 5, PySide2, MSYS2's Python), gcc and Ninja. It doesn't
  need Natron's package repository, which only the plug-ins and the installer
  use. The tests use the ``natron_testing`` UCRT64 builds of openfx-io and
  openfx-misc.
- **macOS** (``macos-test``): macOS 15 arm64, the system (Xcode) clang and
  MacPorts libraries (Python 3.11, PySide2), never Homebrew, which the job
  takes off the ``PATH``. ``/opt/local`` is restored from a cache keyed on the
  MacPorts version, the overlay (``tools/MacPorts``), the port list and
  ``MACPORTS_CACHE_VERSION`` (bump it to drop the cache). There are no prebuilt
  macOS plug-ins, so ``BaseTest.*`` is filtered out.

The jobs run on pull requests against ``master`` and ``RB-*``, and on pushes
to ``master``, ``RB-*`` and ``v*`` tags only: a topic branch is built once,
through its pull request. In addition:

- **Draft pull requests run no CI.** A draft is rebased on the latest base
  branch before it is marked ready, so its earlier runs would be redone
  anyway; marking it ready (``ready_for_review``) starts the run. To test a
  draft, run the workflow by hand (see `How to`_).
- **macOS is informative.** Its job starts only after Linux and Windows have
  passed (``needs:``), because macOS runners are scarce, and it is never a
  required check (see :doc:`guidelines` for what counts as green CI).
- Changes under ``Documentation/`` run nothing (``paths-ignore``): Read the
  Docs builds the documentation (``.readthedocs.yaml``, see
  :ref:`maint-building-manual`).
- A new push to a pull request cancels its previous run (``concurrency``);
  runs for pushes to a branch or a tag are never cancelled.
- The workflow runs with read-only ``permissions`` (``contents: read``).

Build configurations
~~~~~~~~~~~~~~~~~~~~

``matrix.config`` (``CI_CONFIG`` in the steps) is one of:

- **Fast**, for pull requests and branch pushes: CMake Debug (assertions,
  ``Q_ASSERT`` and ``DEBUG`` on) compiled with ``-O1`` and without ``-g``, which
  compiles and links much faster than ``-O0 -g`` or ``-O2 -g``. Linux and
  Windows use ccache; the cache is saved only by pushes to ``master`` and
  ``RB-*`` and by manual runs, so a pull request restores its base branch's
  cache but never evicts it.
- **Debug** (``-O0 -g``) and **Release** (``-O3 -DNDEBUG``), both, for the
  weekly run, tags and manual runs (unless ``build=fast`` is chosen), without
  a compiler cache.

GUI tests
~~~~~~~~~

The Linux job runs under ``xvfb-run`` with Mesa's llvmpipe, and sets
``NATRON_GUI_TESTS=1`` and ``NATRON_TEST_INTERACTIVE=1``: the GUI tests
(``Tests/gui``, ctest label ``gui``, see :ref:`guiTests`) run there, the
interactive ones included, as they can't take a user's keyboard and mouse.
A step prints the GLX context first; without one, the GUI tests are skipped
(exit status 77), not failed (#536). They run in the Fast and Release builds,
not a second time in the weekly Debug build (``ctest -LE gui``). The Linux
tests run in parallel (``ctest -j``; each test has its own cache
directories, #1505), the GUI tests one at a time (``RESOURCE_LOCK
NatronGui``).

The Windows and macOS jobs don't set ``NATRON_GUI_TESTS``, so their GUI tests
are skipped. ``NATRON_BUILD_GUI_TESTS`` (on by default) is the CMake option
that registers them.

Each job uploads ``LastTest.log`` as an artifact (``<os> <config> Test
Logs``), also when a test fails.

Weekly runs
-----------

Every Sunday, GitHub runs the scheduled workflows on the default branch,
RB-2.6, one after the other (after the plug-in repositories' Saturday runs):

.. list-table::
   :header-rows: 1
   :widths: 15 35 50

   * - UTC
     - Workflow
     - Notes
   * - 03:17
     - ``ci.yml``
     - Debug and Release on all three OSes; macOS runs whatever the
       Linux and Windows results
   * - 04:47
     - ``linux_packages.yml``
     - SDK image (only if its definition changed), archives, Natron-Tests,
       ``natron_testing``
   * - 05:17
     - ``macos_x86_64.yml``
     - SDK (reused, updated or rebuilt), DMG, Natron-Tests
   * - 06:17
     - ``build_installer.yml``
     - UCRT64 only, GPL only
   * - 13:37
     - ``project_python_roundtrip.yml``
     - uses the packages of the runs above

The weekly runs are **never skipped**, even when RB-2.6 didn't change:
every one of them uses a rolling toolchain or moving inputs (MSYS2 with
``pacman -Syu``, apt on the Ubuntu images, the runners' Xcode, MacPorts
upgrades, the plug-ins built from their branches, the ``natron_testing``
plug-in builds). There are no nightly runs. The weekly artifacts (packages,
test results, logs) are kept 14 days, long enough to compare two weekly
runs; manual and tag runs of ``build_installer.yml`` keep theirs 90 days.

Linux packages
--------------

``linux_packages.yml`` builds the Linux portable archives, x86_64 and
aarch64, natively on GitHub's ``ubuntu-24.04`` and ``ubuntu-24.04-arm``
runners. The Linux distribution baseline is Rocky Linux 9 (EL9, glibc 2.34,
VFX Reference Platform CY2027).

The SDK image
~~~~~~~~~~~~~

All of Natron's dependencies are built in the Rocky Linux 9 SDK Docker image
(``tools/docker/natron-sdk-rockylinux``, with ``ROCKY=9``; GCC 14 from
``gcc-toolset-14``, Python 3.11, Qt 5.15.19, PySide2 5.15.19; see
``INSTALL_LINUX.md``), published as
``ghcr.io/natrongithub/natron-sdk-rockylinux9``.

- **The SDK hash.** The ``sdk_check`` job hashes the git tree entries of
  everything the SDK build uses: ``tools/jenkins/include/scripts/build-Linux-sdk.sh``,
  ``tools/jenkins/include/scripts/pkg``, ``tools/jenkins/include/patches``,
  ``tools/jenkins/common.sh``, ``tools/jenkins/compiler-common.sh`` and
  ``tools/docker/natron-sdk-rockylinux`` (first 16 hex digits of the
  sha256). The image is tagged ``:<hash>`` (multi-arch), ``:<hash>-amd64`` and
  ``:<hash>-arm64``; on the default branch, ``:latest`` moves to it.
- **What triggers a rebuild:** only a change of one of those files (a new
  hash with no image yet), or a manual run with ``rebuild_sdk`` checked,
  which rebuilds from scratch with the newest Rocky Linux 9 base image and
  packages, without the layer cache. Otherwise the existing image is used as
  is, so a weekly run doesn't pick up new Rocky Linux packages by itself.
- **Stages.** The whole SDK takes longer than the 6 hours a GitHub-hosted job
  may run, so ``tools/docker/natron-sdk-rockylinux/split-stages.sh`` splits
  its Dockerfile into stages (``STAGE_ENDS="postgresql libheif ffmpeg"``),
  each built by one job (``sdk_stage1`` to ``sdk_stage4``, then
  ``sdk_final``) of ``linux_sdk_stage.yml`` and pushed as
  ``:<hash>-<arch>-stage<N>``. A stage that already exists (a re-run) isn't
  rebuilt. The stages reuse the layers of earlier builds through a registry
  cache (``:buildcache-<arch>-<stage>``), which only the default branch
  updates. Changing ``STAGE_ENDS`` changes the hash, and the number of
  ``sdk_stage<N>`` jobs in ``linux_packages.yml`` must match it.

The archives and their tests
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- **package**: ``tools/jenkins/launchBuildMain.sh`` runs in the SDK image,
  with the ``tools/jenkins`` scripts of the commit being built (not the
  copies in the image). It builds Natron and the plug-ins with CMake (the
  plug-ins from their branches, ``Global/plugin-branches.sh``, or from their
  ``Natron-<version>`` tags for a release tag), runs the bundled unit tests,
  and makes the portable archive, ``*-no-installer.tar.xz``.
  ``build-Linux-installer.sh`` checks the bundle (``checkBundle``): system
  libraries and symbol versions no newer than Rocky Linux 9.0. The job also
  fails if the archive has Python caches under ``Plugins/`` (#1923), and
  writes ``Natron-Linux-<arch>-commits.txt``, the commits of Natron and of
  each bundled plug-in.
- **test**: Natron-Tests (``master``) on the archive, in a clean
  ``rockylinux/rockylinux:9`` container with only the system libraries that
  the archive never bundles (``tools/docker/natron-test.sh``): this checks
  that the archive is self-contained and runs on the oldest supported
  distribution.
- **test_latest**: the same suite directly on the latest Ubuntu runners
  (``ubuntu-26.04`` and ``ubuntu-26.04-arm``; ``natron-test.sh -H``), the
  archive extracted into a new directory and run with an empty ``HOME``, with
  the runtime libraries a desktop has (newer glibc, libstdc++, X11 and GL
  libraries than Rocky Linux 9's; #1964, #1965). The runner image has much
  more than a desktop, so only the Rocky Linux 9 job (and ``checkBundle``)
  catches a library missing from the archive.

**Bundled libraries.** The archives bundle the SDK's libraries, but never the
system libraries that must match the user's system: glibc and the compiler
runtime, GLVND/Mesa and the GPU drivers, the core X11 and xcb client
libraries (libX11, libX11-xcb, libxcb, as in the `AppImage excludelist
<https://github.com/AppImageCommunity/pkg2appimage/blob/master/excludelist>`_),
ALSA. The xcb extension and xcb-util libraries and libxkbcommon,
which a desktop doesn't always have, are bundled (#2024). The full list, and
what ``checkBundle`` checks, is in ``INSTALL_LINUX.md`` ("Runtime requirements
of the Linux binaries"); keep both in sync with ``build-Linux-installer.sh``.

The natron_testing pre-release
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

On RB-2.6, a weekly or manual run whose packages and Natron-Tests (both
jobs, both architectures) all passed replaces the assets of the rolling
`natron_testing <https://github.com/NatronGitHub/Natron/releases/tag/natron_testing>`_
pre-release: ``Natron-RB-2.6-Linux-<arch>.tar.xz``,
``Natron-RB-2.6-Linux-<arch>-commits.txt`` and ``natron_testing-commit.txt``
(the Natron commit; the tag itself doesn't move). It is all or nothing, so
both archives always match ``natron_testing-commit.txt``. The plug-in
repositories' CI downloads these archives without a token (see
:doc:`../devel/guitesting`), and so do ``render_soak.yml`` and
``project_python_roundtrip.yml``.

Windows
-------

Natron is never built with MSVC: CI and the packages use MSYS2 **UCRT64**
(gcc, x86-64). Native Windows on ARM64 (#1494) uses MSYS2 **CLANGARM64**
(clang, libc++, LLVM's libomp) on GitHub's ``windows-11-arm`` runner; it isn't
released yet, so it is built by hand only (``windows_arm64.yml``, or the
``msystem`` input of ``build_pacman_repo.yml`` and ``build_installer.yml``).

The package repository
~~~~~~~~~~~~~~~~~~~~~~

The plug-ins and the installer need packages that MSYS2 doesn't have, or
that Natron patches (``tools/MINGW-packages``: ``mingw-w64-natron_*``,
``osmesa``, ``freetype`` and ``qt5-base`` with Natron's fixes, …).
``build_pacman_repo.yml`` builds them into a pacman repository, by hand or on
``v*`` tags, and uploads it as an artifact. It isn't published automatically:

1. Change ``tools/MINGW-packages`` and bump
   ``tools/MINGW-packages/windows_pacman_repo_version.txt`` (``YYYYMMDD-N``).
2. Run *Build pacman repo* on the branch, download its
   ``natron_package_repo-<version>`` artifact, and upload it as
   ``natron_package_repo-<version>.zip`` to the
   ``windows-mingw-package-repo`` release (``gh release upload
   windows-mingw-package-repo natron_package_repo-<version>.zip -R
   NatronGitHub/Natron``; CLANGARM64:
   ``natron_package_repo-clangarm64-<version>.zip``). Never delete the zips of
   older versions: older branches and tags use them.
3. Run *Build Installer* on the branch, then merge it. Until the zip is
   published, every installer run on that branch fails (a manual run can use
   the artifact instead, with ``pacman_repo_run_id``; the weekly run can't).

The packages are built against the MSYS2 of the day, so the repository must
also be rebuilt when MSYS2 moves under it, even if no PKGBUILD changed. In
particular, **OSMesa links MSYS2's static LLVM libraries and pins the exact
LLVM version it was built with**: every MSYS2 LLVM update breaks the
installer until the repository is rebuilt and republished (#2042).
``tools/MINGW-packages/README.md`` has the full procedure, the list of
packages and when to rebuild.

The installer
~~~~~~~~~~~~~

``build_installer.yml`` (*Build Installer*) builds the Windows package with
``tools/jenkins/launchBuildMain.sh`` (CMake, UCRT64, GPL only) on
``windows-2022``, with the package repository pinned by
``windows_pacman_repo_version.txt``. Its ``win-installer`` job makes the
portable tree with Setup.exe in it, checks that every plug-in loads
(``verify_plugin_loads.cpp``), runs the bundle without MSYS2, installs,
upgrades, runs and uninstalls it with Setup.exe, and compares its file list
and DLLs with the 2.6.0-alpha1 installer (the ``compare-<installer>``
artifact). The build runs in a short workspace path (``D:/nbw``), because of
Windows' 260-character ``MAX_PATH``.

The ``natron-tests`` job then runs Natron-Tests on Natron as users get it
(#1959): on a fresh ``windows-latest`` runner that never had MSYS2, it installs
the portable tree with its Setup.exe into a new folder and runs
``runTests.sh`` with the runner's Git Bash against the installed
``NatronRenderer.exe``. Setup.exe reads its files with ``MAX_PATH`` paths, so
the tree is first moved to a short path (``C:\ns``) and installed into
``C:\nt`` (#1990). A known failure that passes (``UNEXPECTED PASS``) fails the
job; other failures are only warnings until the Windows known failures have
been checked against a 2.6 run (#1629).

macOS
-----

All macOS builds use MacPorts and the system (Xcode or Command Line Tools)
clang, never Homebrew (see :doc:`build-macos`). Natron 2.6 supports
macOS 14 Sonoma and later, on arm64 and x86_64: the release builds have a
deployment target of 14.0, and ``tools/tart/check-minos.sh`` checks that no
Mach-O file in the DMG (``minos``) needs a newer macOS. The CI job of
``ci.yml`` isn't shipped, so it targets the runner's macOS.

x86_64 on GitHub
~~~~~~~~~~~~~~~~

``macos_x86_64.yml`` builds the x86_64 DMG on ``macos-15-intel`` runners:

1. **SDK** (``macos_x86_64_sdk.yml``, called up to four times as
   ``sdk-1`` … ``sdk-4``): the MacPorts SDK of ``tools/tart/provision.sh``
   (``/opt/local``, ``/opt/osmesa``, ``/opt/llvm``, the overlay), for macOS 14.0.
   The MacPorts binary archives on macOS 15 are built for macOS 15, so
   ``provision.sh`` runs with ``BUNDLE_ONLY_REBUILD=1``: the archives are used
   for the build tools, and the ports whose files go into the DMG (and the
   overlay ports) are built from source. From scratch, that is about 8.5
   hours of runner time.

   - *Key, reuse and update.* The SDK artifact,
     ``natron-sdk-macos-x86_64-<key>`` (kept 90 days), is keyed by a hash of
     what defines it (``tools/MacPorts``, ``provision.sh``,
     ``check-minos.sh``, ``tools/jenkins/macos-release.sh``,
     ``macos_x86_64_sdk.sh``). The ``find`` job (``macos_x86_64_sdk.sh find``,
     on a Linux runner) reuses an SDK of the same key if it is less than 30
     days old; otherwise it updates the newest SDK of the branch or of the
     default branch (port sync, ``upgrade outdated``, rebuild: about 50
     minutes); it builds from scratch only if there is none. The ``sdk`` input
     of a manual run forces ``update`` or ``scratch``. An update keeps the
     OSMesa it restores: after changing the OSMesa pin, run with
     ``sdk=scratch``.
   - *Deadlines, save and resume.* A GitHub job may run 360 minutes. Each
     stage runs ``provision.sh`` until it is done or until its budget
     (``BUDGET_MINUTES``, 290 minutes after the job started), then stops it
     (``macos_x86_64_sdk.sh run``, exit status 3), saves the whole state
     (``macos_x86_64_sdk.sh save``: ``/opt/local`` with MacPorts' build
     directories, OSMesa and LLVM, the done markers of ``provision.sh``) as an
     ``sdk-state-<N>`` artifact (3 days), and the next stage restores it and
     resumes: the step that was interrupted is run again, and MacPorts resumes
     its build. A scratch build takes two stages. The last stage fails if the
     SDK still isn't finished: add stages to ``macos_x86_64.yml``.

2. **dmg**: on a fresh runner, the SDK is restored and Homebrew moved out of
   ``/usr/local`` (on Intel Macs the compilers search it by default), then
   ``provision.sh sync fixups build`` runs ``launchBuildMain.sh`` (CMake,
   ``NATRON_MACOS_MIN``), makes the DMG and runs ``check-minos.sh``; the job
   also checks that ``Natron`` is x86_64 only. The ``-minos.txt`` list is
   uploaded even when the check fails: it names the files, and so the ports,
   built for a newer macOS.
3. **test**: Natron-Tests on another fresh runner, against the DMG's
   ``Natron.app``, with Homebrew moved away and no SDK
   (``tools/tart/run-natron-tests.sh``).
4. **release**: on pre-release tags only, the DMG and the test results are
   attached to the GitHub pre-release.

arm64 with tart
~~~~~~~~~~~~~~~

The arm64 DMG is built outside GitHub Actions, on a maintainer's Apple
Silicon Mac, in a clean macOS 14 VM made with `tart <https://tart.run>`_
(``tools/tart``: ``natron-sdk.sh create|update``, ``natron-build.sh``,
``natron-test.sh``), with the same ``provision.sh`` as the x86_64 SDK; see
``tools/tart/README.md`` and :doc:`build-macos`. Moving it to GitHub's macOS
15 arm64 runners (deployment target 14.0), validated on a clean macOS 14 tart
VM, is planned (#2047).

OSMesa
------

Natron's software OpenGL renderer (for Shadertoy and the other OpenGL
plug-ins on machines without a GPU) is OSMesa, built by `osmesa-install
<https://github.com/devernay/osmesa-install>`_ as static, mangled libraries
(``libMangledOSMesa32``) with LLVM's llvmpipe driver. Each platform gets it
differently:

- **Linux**: the SDK step ``tools/jenkins/include/scripts/pkg/osmesa.sh``,
  which pins the Mesa, GLU, mesa-demos and LLVM tarballs (with their sha256)
  and the osmesa-install commit (``OSMESA_INSTALL_COMMIT``). It builds Mesa
  18.3.6 with LLVM 6.0.1 today. Changing it changes the SDK hash, so the next
  run of ``linux_packages.yml`` rebuilds the SDK.
- **macOS**: the ``osmesa`` step of ``tools/tart/provision.sh``, with the
  osmesa-install checkout of the tart host, or on GitHub the commit pinned
  by ``NATRON_OSMESA_INSTALL_COMMIT`` in ``tools/jenkins/macos-release.sh``,
  built for ``MACOSX_DEPLOYMENT_TARGET=14.0`` (rebuilt if it was built for a
  newer macOS) with osmesa-install's default Mesa and LLVM: Mesa 18.3.6 and
  LLVM 6.0.1 at the pinned commit.
- **Windows**: the ``mingw-w64-osmesa`` package of the package repository
  (Mesa 25.0.7, the last Mesa with OSMesa, with MSYS2's static LLVM, see
  above).

osmesa-install also publishes prebuilt archives of Mesa 25.0.7 with LLVM
19.1.7 (releases ``osmesa-mesa-<Mesa>-llvm-<LLVM>``), whose Linux archives
pass a `piglit <https://gitlab.freedesktop.org/mesa/piglit>`_ release gate
before they are published (osmesa-install's ``README.md``, "Testing with
piglit"). Moving the Linux and macOS SDKs to Mesa 25.0.7 and LLVM 19.1.7,
and then to these prebuilt archives instead of building LLVM, is tracked in
#1978.

**Bumping a pin**: update the osmesa-install commit and the version
variables in every place above (``pkg/osmesa.sh``, ``macos-release.sh``, the
``mingw-w64-osmesa`` PKGBUILD), then rebuild what uses them: the Linux SDK
(automatic, through its hash), the macOS SDK (``sdk=scratch``), the Windows
package repository (bump ``windows_pacman_repo_version.txt`` and republish),
and the tart SDK image.

Natron-Tests
------------

`Natron-Tests <https://github.com/NatronGitHub/Natron-Tests>`_ is the render
regression suite: ``runTests.sh`` renders each test project with
``NatronRenderer`` and compares the result with its reference images. The
package workflows run it on the packaged Natron, with its bundled plug-ins,
from Natron-Tests' ``master``: ``linux_packages.yml`` (``test`` and
``test_latest``), ``macos_x86_64.yml`` (``test``), ``build_installer.yml``
(``natron-tests``), and ``tools/tart/natron-test.sh`` for the arm64 DMG. Each
uploads ``<package>-tests.txt``, ``-tests-summary.txt`` and
``-unit_tests_failures.zip`` (the outputs and references of the failed
tests).

**Known failures.** Natron-Tests lists the tests that are expected to fail in
``known_failures/``: ``common.txt`` (every platform, where most of them should
be), ``linux.txt``, ``macos.txt``, ``windows.txt``, and ``arm64.txt``,
``x86_64.txt``. ``runTests.sh`` reports them as ``KNOWN FAIL``; a listed test
that passes is an ``UNEXPECTED PASS`` and fails the run, like a failure that
isn't listed, because the lists no longer describe the build. **Every entry
cites an open Natron issue**: never add a test to a list without one, and when
the issue is closed, remove the entry or give it another open issue. A test
goes into a platform or architecture list only when a run shows that it
passes elsewhere.

**References.** To add a test or regenerate references, follow Natron-Tests'
``README.md`` (e.g. its "FFmpeg reader tests" section: the ``TestRead*``
references are the Linux x86_64 outputs of a ``linux_packages.yml`` run,
checked against the aarch64 outputs). Run the workflows on a branch to check
a change of Natron-Tests before merging it.

Other workflows
---------------

- **Render soak** (``render_soak.yml``, by hand only; it takes hours): on Linux
  and Windows, ``Tests/gui/run_render_soak_test.py`` renders synthetic graphs
  and Natron-Tests' BayMax project, thousands of frames per run, with several
  threading and RAM settings; a render that stops making progress saves the
  backtraces of all its threads in the artifacts and fails the job (#248). By
  default it soaks the packages (the ``natron_testing`` Linux archive, the
  Windows portable tree of the last successful ``build_installer.yml`` run on
  RB-2.6), or the ``linux_packages.yml`` or ``build_installer.yml`` run given
  by ``package_run_id``; ``natron_build=source`` builds ``natron_ref``
  instead (RelWithDebInfo, for backtraces with symbols), e.g. a commit before
  a fix. ctest runs a short version without BayMax (``RenderSoakTest``) in
  every CI build.
- **Project Python round trip** (``project_python_roundtrip.yml``, weekly
  and by hand): ``Tests/gui/run_ntp_python_roundtrip_test.py`` saves each
  Natron-Tests project (BayMax and Spaceship included) and each project of
  ``Tests/data`` as a text ``.ntp``, and as Python, which it loads and saves
  as a text ``.ntp`` again, and compares the two ``.ntp`` files (#2001). It
  builds nothing: it uses the Linux x86_64 archive of ``natron_testing`` and
  the Windows portable tree of the last successful ``build_installer.yml``
  run on RB-2.6 (a manual run can name other runs). A difference fails the job
  unless ``Tests/gui/ntp_python_roundtrip_allowlist.txt`` lists it, with the
  issue that tracks it; remove the entry when the issue is fixed. The
  ``NtpPythonRoundTripTest`` ctest runs the same check on the built-in
  projects only.
- **Forward merge** (``forward_merge.yml``): on every push to RB-2.6, keeps
  one "Merge RB-2.6 into RB-2.7" pull request open (its head branch is RB-2.6
  itself), and opens an issue if the branches conflict; a push to RB-2.7
  closes that issue once a hand-made merge lands. It pushes nothing. The
  first run of a new forward-merge pull request starts no *Tests* run (events
  caused by ``GITHUB_TOKEN`` trigger no workflow): close and reopen it. See
  :ref:`maint-forward-merge` for how to merge it.
- **Windows ARM64** (``windows_arm64.yml``, by hand): Natron built with CMake
  in MSYS2 CLANGARM64 on ``windows-11-arm``, and its unit tests (headless:
  the runner has no GPU), with openfx-io built in the same environment
  (``openfx_io_ref``; its plug-ins go in ``Contents/Win-arm64``). Unless
  ``patched_qt`` is off, it builds Natron's ``qt5-base`` with the QMutex fix
  of #1453 (cached; the first run after a change takes 1-2 hours), without
  which a test can hang (#1451). The ``qt`` input selects Qt 6 only on
  RB-2.7.

Releases
--------

Natron's GitHub releases are made from tags:

- **Pre-release tags** (``v2.6.0-alpha1``, ``-beta1``, ``-rc1``, ``-RC1``,
  ``-alpha.1``, …): ``linux_packages.yml``, ``macos_x86_64.yml`` and
  ``build_installer.yml`` build the packages from the tag and attach them, with their Natron-Tests
  results, to the GitHub pre-release
  (``.github/workflows/attach_to_prerelease.sh``, which creates the
  pre-release if it doesn't exist yet): the Linux archives, the x86_64 DMG,
  the Windows portable zip with Setup.exe. Only ``linux_packages.yml`` makes a
  release build (``RELEASE_TAG``), whose plug-ins come from their
  ``Natron-<version>`` tags; ``macos_x86_64.yml`` and ``build_installer.yml``
  pass the tag as a branch name, so they make a snapshot build with the
  plug-ins of ``Global/plugin-branches.sh``. ``linux_packages.yml`` attaches
  every archive it built, with the test results, whatever they are;
  ``macos_x86_64.yml`` and ``build_installer.yml`` attach nothing unless their
  Natron-Tests job passed. The arm64 DMG is built with ``tools/tart`` and
  attached by hand.
- **Every** ``v*`` **tag** also runs ``ci.yml`` (Debug and Release),
  ``build_pacman_repo.yml`` and ``build_installer.yml`` (whose artifacts
  are then kept 90 days).
- **Before a pre-release tag**, collect the CHANGELOG lines of the merged pull
  requests (see :ref:`maint-changelog`). The source tarballs of the Linux SDK
  are mirrored on SourceForge, so that a release can be rebuilt when a
  project's site goes away; the mirror is updated with every SDK version bump
  (see :doc:`sdk-source-mirror`).

How to
------

**Run a workflow by hand.** Every workflow above can be run on any branch
(``workflow_dispatch``; ``linux_sdk_stage.yml`` and ``macos_x86_64_sdk.yml``
are only called by the package workflows): in the
*Actions* tab, choose the workflow and *Run workflow*, or with ``gh``::

    gh workflow run ci.yml -R NatronGitHub/Natron --ref <branch> -f build=fast
    gh workflow run linux_packages.yml -R NatronGitHub/Natron --ref <branch>
    gh workflow run macos_x86_64.yml -R NatronGitHub/Natron --ref <branch> -f sdk=reuse
    gh workflow run build_installer.yml -R NatronGitHub/Natron --ref <branch>
    gh run list -R NatronGitHub/Natron --branch <branch>

``build=fast`` is the pull-request build (Linux, Windows, then macOS); without
it, ``ci.yml`` builds Debug and Release, as the weekly run. Before marking a
draft pull request ready (see :doc:`guidelines`), use this to validate its
rebased head, and to run the package workflows on a change that only they
build (``tools/jenkins``, ``tools/docker``, ``tools/tart``, ``tools/MacPorts``,
``tools/MINGW-packages``). Don't dispatch a run on a
commit that already has one, and poll runs sparingly: all of them share the
repository's API and runner quotas.

**New workflows.** GitHub offers *Run workflow*, and runs the ``schedule:``
triggers, only for workflow files that are on the default branch, RB-2.6. A
new workflow can therefore be dispatched, on any branch, only once its file
has been merged into RB-2.6; until then, test it through a trigger that runs
on the branch (e.g. a temporary ``pull_request`` trigger), or add it to RB-2.6
first. Changes to an existing workflow can be dispatched on the branch that
has them.
