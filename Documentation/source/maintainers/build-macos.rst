.. _maint-build-macos:

Building and Packaging Natron on macOS
======================================

This chapter documents how the **official macOS binaries and ``.dmg`` are
produced** — the MacPorts-based release/packaging build driven by
``tools/jenkins/launchBuildMain.sh``. It is the packaging counterpart to
:ref:`maint-building` (which explains the CMake source build used for
day-to-day development). ``launchBuildMain.sh`` builds Natron with CMake
(see :ref:`maint-building`, "Release builds"). On RB-2.6,
``NATRON_BUILD_SYSTEM=qmake`` selects the deprecated qmake build instead, until
the CMake-built installers are shown to be equivalent
(`#1181 <https://github.com/NatronGitHub/Natron/issues/1181>`_).

The full, copy-pasteable dependency setup lives at the repository root in
``INSTALL_MACOS.md``. This chapter explains the *workflow* around it: preparing
the MacPorts overlay, **keeping MacPorts up to date**, running the build,
producing the styled DMG, signing/notarizing, and testing.

.. contents::
   :local:
   :depth: 2

Overview
--------

The build system is based on `MacPorts <https://www.macports.org>`_ plus a set
of **custom ports** in ``tools/MacPorts``. Homebrew is intentionally not used
for releases (MacPorts is easier to customize and can build universal binaries).
``launchBuildMain.sh`` performs six steps: checkout sources, build the plugins,
build Natron, build the installer/DMG, run unit tests, archive artifacts.
The plugins are built by ``tools/jenkins/build-plugins.sh``: openfx-misc,
openfx-io and openfx-arena with CMake (#1203, #1280), openfx-gmic with its
Makefile, all at ``-O3`` without fast math.
To build them by hand, see "Developing with the MacPorts SDK" in
``INSTALL_MACOS.md``.

Prerequisites
-------------

- **MacPorts**, up to date (``sudo port selfupdate``).
- The **Natron custom-ports overlay** registered in MacPorts (see below).
- A MacPorts **clang** matching the OS default (see *Compiler selection*).
- **macOS packaging tools**, verified early by ``launchBuildMain.sh`` and
  documented in ``INSTALL_MACOS.md``:

  - ``dmgbuild`` — ``sudo port install py311-dmgbuild`` and
    ``sudo ln -sfn dmgbuild-3.11 /opt/local/bin/dmgbuild`` (or
    ``python3 -m pip install dmgbuild``); writes the DMG ``.DS_Store`` layout
    without Finder/TCC.
  - ``oiiotool`` — from the ``openimageio`` port (DMG background processing).
  - ``identify`` — from the ``ImageMagick`` port (DMG background dimensions).

.. note::

   If you add a **new macOS build dependency**, add it to the early check in
   ``tools/jenkins/launchBuildMain.sh`` (the ``PKGOS = OSX`` block) *and*
   document it in ``INSTALL_MACOS.md`` so the build fails fast with a clear
   message instead of after a multi-hour compile.

Compiler selection
------------------

The MacPorts clang is chosen to match the OS's default (see
``clang_compilers.tcl``): Darwin ≥ 25 (macOS 26+) → clang-22, Darwin 24
(macOS 15) → clang-18, Darwin 11–23 → clang-17, Darwin ≤ 10 → clang-11.
``tools/jenkins/compiler-common.sh`` auto-selects a ``clang-mp-*`` for the
Natron build independently — keep its ladder in sync with the installed clang.

Sanity-check a new OS/compiler combination::

    printf 'int main(){}\n' > t.cpp
    clang++-mp-<v> -stdlib=libc++ -fopenmp t.cpp -o t && otool -L t | grep -E 'libc\+\+|libomp'
    # expect /usr/lib/libc++.1.dylib and /opt/local/lib/libomp/libomp.dylib

The packaging build compiles Natron and the plug-ins as C++17 for Qt5 and
C++20 for Qt6, from ``QT_VERSION_MAJOR``, in one of two ways:

- **CMake builds** (the default: ``NATRON_BUILD_SYSTEM=cmake``,
  ``PLUGINS_BUILD_SYSTEM=cmake``): ``build-natron.sh`` and ``build-plugins.sh``
  drop the ``-std=`` flag from ``$CXX``. Natron's ``CMakeLists.txt`` sets
  ``CMAKE_CXX_STANDARD`` from ``NATRON_QT6`` (with GNU extensions, i.e.
  ``-std=gnu++17``/``gnu++20``), and the plug-ins get ``-DOFX_CXX_STANDARD=17``
  or ``20`` (without GNU extensions).
- **qmake and Makefile builds** (``NATRON_BUILD_SYSTEM=qmake``,
  ``PLUGINS_BUILD_SYSTEM=make``): they compile with ``$CXX``, whose
  ``-std=c++17``/``c++20`` comes from ``CXXSTD`` in
  ``tools/jenkins/compiler-common.sh`` (``gnu++17``/``gnu++20`` on Windows).
  The plug-ins' Makefiles keep that standard. qmake takes the standard from
  ``QT_MAJOR_VERSION`` in ``global.pri`` (``CONFIG += c++17`` or ``c++2a``),
  and Qt's ``default_post.prf`` appends ``-std=gnu++1z`` or ``-std=gnu++2a``
  after the other flags (``CONFIG += strict_c++`` is not set), so Natron's
  qmake build uses GNU extensions, like its CMake build.

The release scripts build Natron and the plug-ins with the MacPorts clang
rather than Apple's, even on macOS 14 where Apple clang 15 supports C++17 and
C++20. The CMake builds could use Apple clang with OpenMP (Natron's
``CMakeLists.txt`` and the plug-ins' ``OFX_LIBOMP_ROOT``, which
``build-plugins.sh`` passes, find MacPorts' ``libomp``), but Apple's driver has
no ``-fopenmp`` (only ``-Xpreprocessor -fopenmp``) and Xcode ships no OpenMP
runtime, so the builds that pass ``-fopenmp`` themselves, the deprecated qmake
build and the plug-ins' Makefiles, need a clang with OpenMP; the scripts use
the MacPorts clang for all of them, so that the default and fallback builds use
the same compiler. Apple clang compiles the Objective-C++ files on macOS 12 to
14 (``compiler-common.sh``).

openfx-arena compiles its poppler-based reader as C++20 with GNU extensions in
any case, because recent poppler headers require it: ``POPPLER_CXX_STANDARD``
in its CMake build (``CXX_EXTENSIONS ON``, i.e. ``-std=gnu++20``), and
``POPPLER_CXXSTD`` (``-std=gnu++2a``, after ``$CXX``'s ``-std``) in its
Makefiles.

The MacPorts overlay
--------------------

Register the overlay so ``port`` sees Natron's custom ports. Rsync it to a
stable location (what the official builder uses), so that the live overlay is
decoupled from your working tree. (Pointing ``sources.conf`` at the source tree
itself only works if the unprivileged MacPorts user can read it, which is
usually not the case for a home directory.)::

    sudo mkdir -p /opt/MacPorts-Natron
    sudo chown "$(id -un):staff" /opt/MacPorts-Natron
    # sources.conf line:  file:///opt/MacPorts-Natron
    rsync -avh --delete-after .../Natron/tools/MacPorts/ /opt/MacPorts-Natron/ \
        && (cd /opt/MacPorts-Natron && portindex)

``tools/MacPorts/sync.sh`` wraps the rsync + ``portindex`` step.

The variants are passed explicitly on each ``port install`` command, so no
global ``variants.conf`` is needed (a global ``+quartz`` would conflict with the
``-quartz`` installs).

Installing the dependencies is a long ``sudo port install ...`` sequence; it is
maintained in ``INSTALL_MACOS.md`` — follow it there rather than duplicating it.

.. _maint-macos-minimum-version:

Minimum macOS version
---------------------

Natron 2.6 for macOS runs on **macOS 14 (Sonoma) or later** (maintainer
decision, 2026-11-18), and macOS 14 is the **distribution baseline**
(2026-11-21): the release is built *on* macOS 14, in the tart SDK image
``natron-sdk-sonoma`` (made from the stock
``ghcr.io/cirruslabs/macos-sonoma-base`` image), with only the Command Line
Tools for Xcode 15.3 (macOS 14.4 SDK, Apple clang 15.0.0) and no Xcode. GitHub
CI keeps its ``macos-15`` runners, because GitHub has no macOS 14 runners.
Every Mach-O file in the ``.dmg`` must have a minimum macOS (``minos`` in
``otool -l``'s ``LC_BUILD_VERSION``) of 14.0 or older:

- ``tools/jenkins/common.sh`` exports ``MACOSX_DEPLOYMENT_TARGET=14.0`` only
  for release builds, i.e. when ``NATRON_MACOS_MIN`` is set (to
  ``NATRON_MACOS_RELEASE_MIN``; ``provision.sh``'s build step and the release
  commands of ``INSTALL_MACOS.md`` set it). Otherwise it exports the build
  Mac's major version (e.g. ``15.0``; major.minor on 10.x), see below. An
  explicit ``MACOSX_DEPLOYMENT_TARGET`` wins.
  ``compiler-common.sh`` keeps it. qmake gets it as
  ``QMAKE_MACOSX_DEPLOYMENT_TARGET``, the plug-ins' CMake builds as
  ``CMAKE_OSX_DEPLOYMENT_TARGET`` (``OFX_DEPLOY_STRICT_MINOS`` then rejects a
  bundled library that is too new), and openfx's ``Makefile.master`` reads it
  from the environment.
- The bundled libraries, Qt, PySide2 and the Python framework come from
  MacPorts. ``macports.conf`` has ``macosx_deployment_target 14.0``. On macOS
  14 that is MacPorts' default, and its binary archives (built for the macOS
  they were made on) are used. On a newer build macOS, ``buildfromsource
  always`` is needed too (``step_macports_conf`` sets it there), and
  ``qt5-qtwebengine`` then needs a full Xcode to build. ``macosx_sdk_version``
  keeps its default (the build OS's SDK).
- OSMesa and LLVM are built with ``MACOSX_DEPLOYMENT_TARGET=14.0``
  (osmesa-install honours it).
- ``tools/tart/check-minos.sh`` lists the Mach-O files (in directories, files,
  or a ``.dmg``) that need a newer macOS than ``-t TARGET``.
  ``launchBuildMain.sh`` (release builds) and ``tools/tart/natron-build.sh`` run
  it on the ``.dmg`` and fail the build if any file needs one.
  ``tools/tart/provision.sh`` uses it to find the installed ports, and the
  OSMesa/LLVM builds, that must be rebuilt (``step_rebuild_target``).
  For a MacPorts install outside tart, see "Building for an older macOS" in
  ``INSTALL_MACOS.md``.

To change the minimum version, change ``tools/jenkins/macos-release.sh``, the
single source for the release target (``NATRON_MACOS_RELEASE_MIN``, read by
``common.sh`` through ``NATRON_MACOS_MIN``, ``provision.sh``,
``natron-build.sh`` and the default of ``check-minos.sh``), the Command Line
Tools of the SDK image (``NATRON_MACOS_CLT_VERSION``) and its base image
(``NATRON_MACOS_BASE_IMAGE``, pinned by digest; ``BASE_IMAGE`` overrides it).
Then rebuild the SDK image (``tools/tart/natron-sdk.sh create``), update the
``NATRON_MACOS_MIN=14.0`` of the release commands in section 10 of
``INSTALL_MACOS.md`` (hand-typed copies of that value), and update the system
requirements in the User Guide.

Only release builds target that version: ``common.sh`` uses it when
``NATRON_MACOS_MIN`` is set (``provision.sh``'s build step sets it), and
``launchBuildMain.sh`` then checks the DMG with ``check-minos.sh``. Without it,
development builds target the build Mac's own macOS, so that the plug-ins can
bundle MacPorts' ordinary archives (``OFX_DEPLOY_STRICT_MINOS``).

.. _maint-macos-update-macports:

Keeping the local MacPorts up to date
-------------------------------------

The step-by-step routine (update the checkout, update MacPorts, publish and
check the overlay, upgrade the ports, re-apply the install fixups, reclaim disk
space, rebuild and test) and the procedure after a macOS upgrade are in
``INSTALL_MACOS.md``, section "11. Keeping MacPorts up to date". This section
covers the maintainer part of it: bringing Natron's overlay up to date when
upstream MacPorts changes one of its ports.

**MacPorts reads the overlay only from** ``/opt/MacPorts-Natron``, **which is a
copy of the** ``tools/MacPorts`` **directory of the checkout that last ran**
``./sync.sh``. After working on a reintegration branch, switch back to the
branch you build and run ``./sync.sh`` again, otherwise MacPorts keeps using
the branch's Portfiles.

.. _maint-macos-overlay-reintegrate:

Reintegrating upstream Portfiles into the overlay
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Natron's overlay, ``tools/MacPorts/<category>/<port>/``, holds modified copies
of some MacPorts ports. A port that also exists upstream has three files:

``Portfile``
   What MacPorts uses: the upstream Portfile plus Natron's changes.
``Portfile.orig``
   The upstream Portfile it was last synchronised with.
``Portfile.patch``
   Natron's changes, as ``diff -u Portfile.orig Portfile``.

Ports that exist only in the overlay (for example ``graphics/seexpr211`` or
the older ``openimageio*`` versions) have no ``Portfile.orig`` or
``Portfile.patch``. Patches to the port's sources live in ``files/``, and
MacPorts applies them with ``-p0``.

The upstream ports tree is the one MacPorts downloads with ``port selfupdate``:
``/opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports``,
or ``/opt/local/var/macports/sources/distfiles.macports.org/ports`` when the
default source in ``/opt/local/etc/macports/sources.conf`` is the HTTPS one.
``check.sh`` finds it by itself.

#. **Update MacPorts and list what changed upstream**::

       sudo port selfupdate
       cd $NATRON_SRC/tools/MacPorts
       ./check.sh

   ``check.sh`` compares the overlay ports that are installed (all of them
   with ``--all``) with their upstream MacPorts counterparts, by content. It
   lists the ports whose upstream Portfile no longer matches
   ``Portfile.orig``, with the command to reintegrate them; the ``files/``
   entries that upstream added or changed; the ``files/`` entries that no
   Portfile uses; and whether ``/opt/MacPorts-Natron`` needs a ``./sync.sh``.

#. **Reintegrate each listed port.** The usual commands are the ones
   ``check.sh`` prints; spelled out, with ``P`` the port directory::

       ports=/opt/local/var/macports/sources/rsync.macports.org/macports/release/tarballs/ports
       P=graphics/openimageio
       cp "$ports/$P/Portfile" "$P/Portfile.orig"
       cp "$ports/$P/Portfile" "$P/Portfile"
       (cd "$P" && patch < Portfile.patch)

   That is: take the new upstream Portfile as the baseline, then re-apply
   Natron's changes on top of it. If ``patch`` reports rejected hunks, open the
   ``Portfile.rej`` file, apply those changes by hand to ``Portfile`` in the
   new upstream context, then delete the ``.rej`` and ``.orig`` files that
   ``patch`` left. Copy the ``files/`` entries that ``check.sh`` reports as
   missing or changed, and check that Natron's own ``files/`` patches still
   apply to the new upstream version.

   **Pinned ports are the exception.** A port directory with a ``PINNED`` file
   is deliberately kept at a version other than upstream's (the table is in
   ``INSTALL_MACOS.md``, section 11; today ``graphics/libraw``,
   ``graphics/opencolorio`` and ``print/freetype``). The file says which
   kind of pin it is:
   ``ahead`` (our version is newer than upstream's: unpin it, by deleting
   ``PINNED`` and taking upstream's Portfile, as soon as upstream's version
   is greater than or equal to ours, which ``check.sh`` reports as
   "upstream caught up") or ``older`` (held back until the reason is gone).
   For pinned ports, ``check.sh`` only prints the ``Portfile.orig`` update:
   keep the ``Portfile``. Syntax or compatibility fixes from upstream can
   still be copied into it by hand.

#. **Regenerate the patches,** so that each ``Portfile.patch`` is again the
   difference between ``Portfile.orig`` and ``Portfile``::

       (cd "$P" && diff -u Portfile.orig Portfile > Portfile.patch)

   or, for every port at once::

       skills/macports-natron-upgrade/scripts/regen-portfile-patches.sh .

   This only rewrites ``Portfile.patch`` files, never a ``Portfile``.

#. **Repeat** ``./check.sh`` until it lists nothing.

#. **Publish the overlay and test it**::

       ./sync.sh
       diff -rq -x 'PortIndex*' $NATRON_SRC/tools/MacPorts /opt/MacPorts-Natron

   ``sync.sh`` copies the overlay to ``/opt/MacPorts-Natron`` and runs
   ``portindex``, which must report ``Ports failed: 0``. A failure there is
   usually a Tcl syntax error in a ``Portfile``. Then upgrade the installed
   ports (``INSTALL_MACOS.md``, section 11) to check that the reintegrated
   ports still build, and fix the ``Portfile`` or a ``files/`` patch where they
   don't (the port's build log is under ``/opt/local/var/macports/logs/``).

#. **Commit the changes,** one commit per port where practical, and mention
   the upstream version in the message.

``tools/MacPorts/skills/macports-natron-upgrade/SKILL.md`` describes the same
procedure for running it with an AI coding agent.

Running the build
------------------

::

    workspace=$HOME/Development/workspace
    mkdir -p "$workspace"
    cd "$NATRON_SRC/tools/jenkins"     # NATRON_SRC: your Natron checkout

    NOUPDATE=1 WORKSPACE="$workspace" DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL \
      GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=RB-2.6 \
      UNIT_TESTS=true BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 \
      COMPILE_TYPE=release MKJOBS=8 \
      ./launchBuildMain.sh

Useful knobs:

- ``NOUPDATE=1`` keeps the script from self-updating the ``tools/jenkins``
  scripts over your working copy (so local edits persist).
- ``BUILD_FROM=N`` / ``BUILD_TO=N`` restrict which of the six steps run
  (1=checkout, 2=plugins, 3=natron, 4=installer/DMG, 5=unit tests, 6=archive).
  Steps reuse the artifacts under ``$WORKSPACE/tmp`` from earlier steps.
- ``UNIT_TESTS=true`` runs the full render/regression suite after packaging
  (see *Testing*).

Artifacts (the ``.dmg`` and logs) are written under
``$WORKSPACE/builds_archive/<BUILD_NAME>/<BUILD_NUMBER>/``.

Artifacts and file naming
-------------------------

Release artifacts follow a common ``INSTALLER_BASENAME`` (assembled in
``launchBuildMain.sh``)::

    Natron-<branch>-<date>-<commit>-macOS<version>-<arch>[.dmg|-tests.txt|-unit_tests_failures.zip]
    e.g.  Natron-RB-2.6-202305220414-dc73d01-macOS12-arm64.dmg

- ``<commit>`` (``NATRON_VERSION_STRING``) is the 7-char git SHA for SNAPSHOT
  builds. It is set during the **checkout** step; if you *resume* a build from a
  later step (``BUILD_FROM=4/5``) the checkout is skipped and this field can be
  empty, producing a ``--`` in the name.
- ``macOS<version>`` is the oldest macOS the build runs on: it comes from a
  ``case`` on ``MACOSX_DEPLOYMENT_TARGET`` (or ``sw_vers -productVersion`` if
  it is unset) in ``launchBuildMain.sh``. That table must list current OS versions; unlisted
  versions fall through to the literal ``OSX``. It now covers 10.5–10.15, 11–16
  and the year-based 26–30, with a ``macOS<major>`` catch-all for the future.
  (The ``tools/docker/natron-sdk`` SDK build uses a *gitignored copy* of this
  script derived from ``tools/jenkins``; ``tools/jenkins/launchBuildMain.sh`` is
  the source of truth.)

Three artifacts are produced per build and distributed together (generated by
``runUnitTests.sh``, which runs on macOS/Linux/Windows when ``UNIT_TESTS=true``):

- ``…-<arch>.dmg`` — the installer/disk image.
- ``…-<arch>-tests.txt`` — the render-suite ``result.txt`` (per-test PASS/FAIL).
- ``…-<arch>-unit_tests_failures.zip`` — a zip of the ``failed/`` directory
  (reference/output/comp images for each failing test), for post-mortem review.

The DMG (background and layout)
-------------------------------

The window background and icon layout are written **directly into the DMG's
``.DS_Store``** by ``dmgbuild`` (``build-OSX-installer.sh``). This replaced the
older approach of driving Finder with AppleScript (``osascript``), which
required GUI Automation (TCC) permission and failed in headless/CI or
unprivileged sessions with a ``-10004 privilege violation`` — producing a DMG
with **no background**.

Layout (unchanged from the historical design): the background is the Natron
splashscreen, contrast-reduced with ``oiiotool --powc 0.3``; the window matches
the background size; 104-px icons; ``Natron.app`` at ``(120,180)`` and the
``Applications`` alias at ``(400,180)``; format ``UDBZ``.

To build a styled DMG by hand from an existing ``Natron.app``::

    oiiotool -i splashscreen.png --powc 0.3 -o /tmp/bg.png
    cat > /tmp/settings.py <<'EOF'
    import os.path
    app = os.path.abspath("Natron.app"); appname = os.path.basename(app)
    format = "UDBZ"; volume_name = "Natron"
    files = [app]; symlinks = {"Applications": "/Applications"}
    background = "/tmp/bg.png"
    window_rect = ((200, 200), (767, 465)); icon_size = 104
    icon_locations = {appname: (120, 180), "Applications": (400, 180)}
    EOF
    dmgbuild -s /tmp/settings.py "Natron" Natron.dmg

Code signing and notarization
------------------------------

On Apple Silicon, native code must carry **at least an ad-hoc signature** to run
at all; the linker/``macdeployqt`` apply this automatically. That is enough to
run locally but **Gatekeeper rejects** ad-hoc/unsigned apps downloaded from the
internet (they are quarantined).

Ad-hoc (re-)sign an app and DMG cleanly::

    codesign --force --deep --options runtime --sign - Natron.app
    codesign --verify --deep --strict --verbose=2 Natron.app   # expect "valid on disk"
    codesign --force --sign - Natron.dmg

.. note::

   Running the test suite can invalidate a prior signature by writing Python
   ``__pycache__/*.pyc`` files into the bundle ("a sealed resource is missing or
   invalid"). Re-sign **after** testing, before packaging/distribution.

Distribution options:

.. list-table::
   :header-rows: 1
   :widths: 26 12 62

   * - Option
     - Cost
     - Result
   * - Ship ad-hoc + document bypass
     - Free
     - Runs, but users must clear quarantine:
       ``xattr -dr com.apple.quarantine /Applications/Natron.app``. First-launch
       warning. Common for small OSS.
   * - Developer ID + notarize + staple
     - $99/yr
     - Only path to a warning-free install:
       ``codesign --options runtime -s "Developer ID Application: …"`` (deep) →
       ``xcrun notarytool submit`` the DMG → ``xcrun stapler staple``.
   * - Self-signed certificate
     - Free
     - Useless for distribution — Gatekeeper does not trust it.

**A free Apple ID cannot notarize or issue a "Developer ID Application"
certificate** — those require the paid Apple Developer Program (or a fee waiver
for eligible nonprofits). Release binaries should be signed under the project's
Developer ID, not a contributor's personal Apple ID.

Testing
-------

Two independent test layers:

- **Basic unit tests** — the google-test ``Tests`` binary, run automatically
  during packaging. Expect ``[ PASSED ] N tests``.
- **Full render/regression suite** — ``runTests.sh`` from the
  `Natron-Tests <https://github.com/NatronGitHub/Natron-Tests>`_ repo, invoked
  by ``runUnitTests.sh`` when ``UNIT_TESTS=true``. It renders ~200 ``.ntp``
  projects with ``NatronRenderer`` and compares each to a reference image with
  ``idiff``.

To run the full suite by hand against a built app::

    git clone https://github.com/NatronGitHub/Natron-Tests.git
    cd Natron-Tests
    APP=.../Natron.app
    env SRCDIR="$PWD" \
        OCIO="$APP/Contents/Resources/OpenColorIO-Configs/blender/config.ocio" \
        FFMPEG="$APP/Contents/MacOS/ffmpeg" COMPARE="$APP/Contents/MacOS/idiff" \
        bash runTests.sh "$APP/Contents/MacOS/NatronRenderer"

**Required before every macOS release:** check that the DMG is
self-contained by running the suite on a bare macOS 14 with
``tools/tart/natron-test.sh <dmg>``: it uses a throw-away clone of the stock
``macos-sonoma-base`` tart image without the Command Line Tools, MacPorts or
Homebrew, copies in only ``Natron.app`` and the Natron-Tests data, and writes
``<dmg name>-tests.txt`` and ``<dmg name>-unit_tests_failures.zip`` next to the
DMG. Attach both to the GitHub release, as for v2.5.0
(``Natron-2.5.0-macOS12-x86_64-tests.txt`` and
``Natron-2.5.0-macOS12-x86_64-unit_tests_failures.zip``). It exits with status
1 when a test fails that is not a known failure, so it can gate the release.
The known failures are listed in Natron-Tests, shared by the three platforms:
``known_failures/common.txt`` and ``macos.txt``, ``linux.txt``, ``windows.txt``
(one test per line, with its issue). ``runTests.sh`` reports them as
``KNOWN FAIL``, and a listed test that passes as ``UNEXPECTED PASS``: update
the lists then. See ``tools/tart/README.md``.

Gotchas:

- ``runTests.sh`` runs ``set -e`` and downloads two example assets
  (``Natron_2.3.12_Spaceship.zip``, ``Natron_2.3.12_BayMax.zip``) from
  SourceForge at startup. A transient network/SSL error there aborts the whole
  suite before any test runs. Pre-download both zips into the ``SRCDIR`` (or the
  Natron-Tests dir) to skip the flaky fetch.
- ``idiff`` thresholds are strict. After a toolchain bump (ffmpeg, OpenImageIO,
  ImageMagick) many "failures" are **stale reference images**: the output is
  visually identical but not bit-identical (lossy-codec precision, anti-aliased
  edges). Inspect ``failed/*-{reference,output,comp}*`` before treating a
  failure as a regression; genuine regressions show a non-trivial *mean* error
  (e.g. a global gamma/tone shift), not just a few hot edge pixels.
