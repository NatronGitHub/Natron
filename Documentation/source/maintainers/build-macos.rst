.. _maint-build-macos:

Building and Packaging Natron on macOS
======================================

This chapter documents how the **official macOS binaries and ``.dmg`` are
produced** — the MacPorts-based release/packaging build driven by
``tools/jenkins/launchBuildMain.sh``. It is the packaging counterpart to
:ref:`maint-building` (which explains the qmake/CMake source builds used for
day-to-day development).

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

Prerequisites
-------------

- **MacPorts**, up to date (``sudo port selfupdate``).
- The **Natron custom-ports overlay** registered in MacPorts (see below).
- A MacPorts **clang** matching the OS default (see *Compiler selection*).
- **macOS packaging tools**, verified early by ``launchBuildMain.sh`` and
  documented in ``INSTALL_MACOS.md``:

  - ``dmgbuild`` — ``sudo port install py310-dmgbuild`` and
    ``sudo ln -s dmgbuild-3.10 /opt/local/bin/dmgbuild`` (or
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

The Natron packaging build compiles with ``-std=c++20`` (poppler requires it);
C++17 remains the language *minimum* enforced by ``Global/Macros.h``.

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

.. _maint-macos-update-macports:

Keeping the local MacPorts up to date
-------------------------------------

Natron pins several ports in the overlay (e.g. ``libomp`` is frozen at 11.1.0).
When upstream MacPorts moves, re-synchronise carefully:

#. **Update MacPorts itself**::

       sudo port selfupdate

#. **Find outdated overlay files.** ``check.sh`` compares each file in the
   overlay against its upstream MacPorts counterpart and lists what changed::

       cd .../Natron/tools/MacPorts
       ./check.sh

#. **Update each outdated file** following ``check.sh``'s per-file instruction,
   **resolving conflicts manually**. Respect the local pins — for a frozen port
   only the *effective* Portfile (version/checksums) is frozen; syntax and
   compatibility fixes are allowed, and ``Portfile.orig`` still tracks upstream.

#. **Regenerate the Portfile patch** for any port that carries a
   ``Portfile.orig`` / ``Portfile.patch`` pair and whose ``Portfile`` you had to
   edit to resolve a conflict::

       diff -u Portfile.orig Portfile > Portfile.patch

   (``tools/MacPorts/skills`` includes a helper to regenerate every
   ``Portfile.patch`` at once.)

#. **Re-sync the overlay** (``tools/MacPorts/sync.sh``) so the changes reach the live
   ports tree, then ``portindex``.

#. **Upgrade the installed ports.** Either:

   - ``sudo port -v upgrade outdated``, after ``./sync.sh`` (it can trigger long
     rebuilds of the dependency graph; upgrade port by port if it fails); then
   - reinstall the pinned ports that came out newer (``sudo port -f uninstall
     <port>`` and install it again with the same variants), as
     ``tools/tart/provision.sh`` does in its ``update`` and ``pins`` steps.
     Re-running ``sudo port install ...`` does not upgrade installed ports.

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
- ``macOS<version>`` comes from a ``sw_vers -productVersion`` ``case`` in
  ``launchBuildMain.sh``. That table must list current OS versions; unlisted
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
