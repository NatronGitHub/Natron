Instructions for installing Natron from sources on macOS
=======================================================

These are step-by-step instructions to compile Natron on macOS. This file is the reference for macOS installs. There are two ways to build:

- **In a VM, fully automated:** `tools/tart/README.md` builds the same MacPorts SDK in a clean macOS 15 (Sequoia, the distribution baseline) [tart](https://tart.run) VM, then builds the `.dmg` in a throw-away clone. Its provisioning script (`tools/tart/provision.sh`) runs the MacPorts steps below. tart only runs on **Apple Silicon Macs** and only produces arm64 builds; for Intel or universal builds, or for macOS 11 and older, use the MacPorts steps below on an Intel Mac (see "Notes for older systems"). On any Mac, the MacPorts steps below can also be run natively.
- **On your Mac:** follow the [MacPorts section](#macports-from-scratch) below, or use Homebrew for development builds.

The official macOS binaries are built with [MacPorts](https://www.macports.org) plus the custom ports in `tools/MacPorts`, using the `tools/jenkins/launchBuildMain.sh` script. That script does everything: checks out the sources, compiles the plugins and Natron, builds the `.dmg`, runs the tests and archives the result. The [MacPorts section](#macports-from-scratch) below describes how to prepare a Mac for it from scratch. On Apple Silicon it produces native arm64 binaries.

[Homebrew](https://brew.sh) can also be used for development builds (see [Homebrew](#homebrew)). It is not used for releases, because MacPorts is easier to customize.

Natron is built with **CMake** (see [Building with CMake](#building-with-cmake)). The release scripts (`launchBuildMain.sh`, and `tools/tart/provision.sh`, which drives it) build Natron with CMake too. The qmake build is deprecated: on RB-2.6 it stays available to them as a fallback (`NATRON_BUILD_SYSTEM=qmake`) until the CMake-built installers are shown to be equivalent (#1181); it will be removed in a future release.

The workflow around the release build is documented in the Maintainer Guide (`Documentation/source/maintainers/build-macos.rst`). That covers keeping MacPorts up to date, the overlay maintenance, signing, and the test suite.

## Before you start

A from-scratch MacPorts install builds LLVM, Mesa (OSMesa), Qt 5 and Qt WebEngine from source where no binary archive exists: plan for several hours and about 60 GB of free disk space (Xcode itself is a large download). Run the long steps inside `caffeinate -i` or `tmux`. On a fresh Mac, the first `git` command prompts you to install the Command Line Tools.

## Checkout sources

The MacPorts overlay is part of the source tree, so check out the sources first:

```Shell
git clone https://github.com/NatronGitHub/Natron.git
cd Natron
git submodule update --init --recursive
```

`--recursive` matters: `libs/SequenceParsing` has its own submodule (`tinydir`), which the build needs.

In the rest of this document, `$NATRON_SRC` is the absolute path to this checkout. Set it in every shell you build from:

```Shell
export NATRON_SRC=$PWD   # in the Natron directory
export CLANG=22          # the MacPorts clang of step 5 (see the table there)
```

## Install Dependencies

There are two exclusive options: MacPorts (used for official builds) or Homebrew.

### MacPorts from scratch

Run every step below from a terminal. Some `port install` steps take a long time (clang and Qt are built from source when no binary archive is available), so running them inside `caffeinate -i` (or `tmux`, installed in step 6) is a good idea.

If Homebrew is also installed, keep it out of your `PATH` while building with MacPorts. Otherwise build scripts may silently pick up Homebrew tools and hide missing MacPorts dependencies. These shell functions (bash or zsh: put them in `~/.zshrc`, or `~/.bash_profile` for bash) switch between the two:

```Shell
path_remove() {  # remove one exact entry from PATH
  PATH=":$PATH:"; PATH=${PATH//":$1:"/":"}; PATH=${PATH#:}; PATH=${PATH%:}
}
port_switch() {  # use MacPorts, hide Homebrew
  local p
  for p in /opt/homebrew /usr/local; do
    if [ -x "$p/bin/brew" ]; then
      path_remove "$p/bin"; path_remove "$p/sbin"; path_remove "$p/opt/python/libexec/bin"
    fi
  done
  unset HOMEBREW_PREFIX HOMEBREW_CELLAR HOMEBREW_REPOSITORY
  path_remove /opt/local/bin; path_remove /opt/local/sbin
  if [ -x /opt/local/bin/port ]; then PATH="/opt/local/bin:/opt/local/sbin:$PATH"; fi
}
brew_switch() {  # use Homebrew, hide MacPorts
  path_remove /opt/local/bin; path_remove /opt/local/sbin
  if [ -x /opt/homebrew/bin/brew ]; then eval "$(/opt/homebrew/bin/brew shellenv)"
  elif [ -x /usr/local/bin/brew ]; then eval "$(/usr/local/bin/brew shellenv)"; fi
}
```

Run `port_switch` in the terminal you build from (before `launchBuildMain.sh` or `make`).

#### 1. Xcode

Install Xcode from the App Store. The Command Line Tools alone are not enough. Then select it and accept the license:

```Shell
sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
sudo xcodebuild -license accept
```

Under *System Settings › Privacy & Security › Developer Tools* (*System Preferences › Security & Privacy › Privacy › Developer Tools* before macOS 13), enable your terminal application. This avoids Gatekeeper scans on every freshly built binary. On older macOS versions, the App Store only offers Xcode versions that need the latest macOS: download an older Xcode from <https://developer.apple.com/download/all/> (e.g. Xcode 12.4 for macOS 10.15).

#### 2. MacPorts

Download and install the `.pkg` for your macOS version from <https://www.macports.org/install.php> (or <https://github.com/macports/macports-base/releases/latest>). Then update it:

```Shell
sudo port selfupdate
```

#### 3. Natron's custom ports

Natron needs modified ports. Examples: the `+natron` variants of ImageMagick, ImageMagick7 and openimageio and the `+natronmini` variant of ffmpeg; libraw 0.22.3 with the GPL demosaic packs restored by a patch; the subports `ffmpeg-lgpl`, `libraw-lgpl` and `libheif-lgpl` for COMMERCIAL builds; freetype 2.15.0 with an upstream fix for rotated text backported; onetbb with a patch for a crash at exit (#1253); qt5-qtbase with a patch for a QMutex lost wake-up that hangs render threads on Apple Silicon (#1451); ffmpeg and ffmpeg-lgpl with patches for black 1-pixel columns in 10-bit DNxHD/DNxHR decoded on Apple Silicon ([openfx-io#266](https://github.com/NatronGitHub/openfx-io/issues/266)) and for a DNxHR encoder abort on high-entropy images ([openfx-io#267](https://github.com/NatronGitHub/openfx-io/issues/267)). If you already have a MacPorts SDK with qt5, run `sudo port -v upgrade qt5-qtbase` after `./sync.sh` below: the patched revision (5.15.19_1) has no binary archive, so it builds from source. Likewise, run `sudo port -v upgrade ffmpeg` (and `ffmpeg-lgpl`, if installed) to get the patched FFmpeg (9.0.2_5). MacPorts reads them from a copy of `tools/MacPorts` in `/opt/MacPorts-Natron`. A copy is used rather than the source tree itself, because MacPorts accesses the ports as an unprivileged user that usually can't read your home directory.

```Shell
sudo mkdir -p /opt/MacPorts-Natron
sudo chown $(id -un):staff /opt/MacPorts-Natron
cd $NATRON_SRC/tools/MacPorts
./sync.sh          # rsync to /opt/MacPorts-Natron and run portindex, which must report "Ports failed: 0"
```

Then register it **before** the default source. Add this line above the line ending in `[default]` (usually the last line: `rsync://… [default]`, or `https://… [default]`) of `/opt/local/etc/macports/sources.conf`:

```
file:///opt/MacPorts-Natron
```

for example with:

```Shell
grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf ||
    sudo perl -0pi -e 's|^(\S+\s+\[default\b[^\]]*\])|file:///opt/MacPorts-Natron\n$1|m' /opt/local/etc/macports/sources.conf
grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf || echo 'Error: add the line to sources.conf by hand'
```

Check it with `port file openimageio`, which should print a path under `/opt/MacPorts-Natron`. Re-run `./sync.sh` whenever the files in `tools/MacPorts` change, including after a `git pull` or a branch switch, and before installing or upgrading ports: an out-of-date `PortIndex` makes MacPorts resolve the overlay ports with stale versions.

`./check.sh` (in the same directory; `--all` to check every overlay port, not only the installed ones) often reports obsolete Portfiles right after a `port selfupdate`: you can still use the committed overlay. It also warns when `/opt/MacPorts-Natron` differs from your checkout, meaning `./sync.sh` must be re-run. Reintegrating it with upstream is a maintainer task, see "Reintegrating upstream Portfiles into the overlay" in `Documentation/source/maintainers/build-macos.rst`.

#### 4. OSMesa and LLVM

Natron and the openfx-misc plugins use a mangled OSMesa (software OpenGL, used for headless rendering) installed in `/opt/osmesa`, with a static LLVM in `/opt/llvm`. Build both with [osmesa-install](https://github.com/devernay/osmesa-install) **before** installing the other ports. The Mesa 18 build needs `python` to be Python 2.7, which is only selected for this step.

```Shell
sudo mkdir -p /opt/osmesa /opt/llvm
sudo chown $(id -un):staff /opt/osmesa /opt/llvm
sudo port -v -N install python27 cmake autoconf automake libtool pkgconfig gsed
sudo port select --set python python27

cd $NATRON_SRC/..   # anywhere outside the Natron checkout (not in tools/MacPorts)
git clone https://github.com/devernay/osmesa-install.git
mkdir osmesa-install/build && cd osmesa-install/build
env LLVM_BUILD=1 MKJOBS=8 ../osmesa-install.sh     # LLVM 6.0.1 into /opt/llvm, Mesa 18.3.6 into /opt/osmesa; no sudo

sudo port select --set python none
```

Use osmesa-install master: it has the LLVM and Mesa build fixes needed with recent macOS SDKs and Xcode versions (verified with Xcode 27.2).

**Mesa 25.0.7 (optional).** osmesa-install also builds Mesa 25.0.7 with LLVM 19.1.7, which gives thread-safe OSMesa and OpenGL 4.5 compatibility contexts to the Shadertoy plugin; Mesa 18.3.6 remains the default. It needs meson, ninja and a Python 3 with mako and PyYAML instead of Python 2.7 and gsed. Install it next to the default one, and select it for the Natron build with `OSMESA_PREFIX` and `LLVM_PREFIX`:

```Shell
sudo port -v -N install meson ninja py311-mako py311-yaml
sudo mkdir -p /opt/osmesa-25.0.7 /opt/llvm-19.1.7
sudo chown $(id -un):staff /opt/osmesa-25.0.7 /opt/llvm-19.1.7
cd $NATRON_SRC/../osmesa-install/build   # the osmesa-install checkout of the step above
env OSMESA_VERSION=25.0.7 OSMESA_PREFIX=/opt/osmesa-25.0.7 LLVM_PREFIX=/opt/llvm-19.1.7 \
    PYTHON=/opt/local/bin/python3.11 MKJOBS=8 ../osmesa-install.sh   # builds LLVM 19.1.7 first; no sudo
# then build Natron (step 10) with: env OSMESA_PREFIX=/opt/osmesa-25.0.7 LLVM_PREFIX=/opt/llvm-19.1.7 ...
```

Don't use a MacPorts or Homebrew LLVM (`LLVM_SYSTEM=1`) for release builds: the Shadertoy plugin would then depend on their dylibs.

#### 5. Compiler

Ports and Natron are compiled with a MacPorts clang (rather than Apple's) for full OpenMP support. Install the version MacPorts uses by default on your OS:

| macOS | clang port |
|---|---|
| 26 and later | `clang-22` |
| 15 | `clang-18` |
| 10.7 to 14 | `clang-17` |
| 10.6 | `clang-11` |

```Shell
CLANG=22   # from the table above: 18 on macOS 15, 17 on 10.7 to 14 (also export it next to NATRON_SRC)
sudo port -v -N install clang-$CLANG
sudo port setrequested clang-$CLANG
```

`tools/jenkins/compiler-common.sh` builds Natron itself with the newest installed `clang-mp-*` that is not newer than the default in the table above, so older systems such as macOS 10.15 keep using clang-17.

On a new macOS or clang version, check that OpenMP programs link against the system libc++ and MacPorts' libomp:

```Shell
printf 'int main(){}\n' > /tmp/t.cpp
clang++-mp-$CLANG -stdlib=libc++ -fopenmp /tmp/t.cpp -o /tmp/t && otool -L /tmp/t
# expect /usr/lib/libc++.1.dylib and /opt/local/lib/libomp/libomp.dylib
```

#### 6. Build tools

```Shell
sudo port -v -N install gsed gawk coreutils findutils keychain tmux gzip bash wget bison flex doxygen \
    texinfo meson ninja help2man unzip tcl expect
```

#### 7. Libraries

```Shell
sudo port -v -N install dbus
sudo port -v -N install glib2 -x11
sudo port -v -N install libjpeg-turbo libpng tiff expat libmng boost
sudo port -v -N install cairo -x11
sudo port -v -N install x264 x265
sudo port -v -N install libvpx +highbitdepth
sudo port -v -N install libraw +gpl2

# OpenEXR 3 replaces ilmbase (ignore "not installed" errors)
sudo port -v -N uninstall ilmbase opencv4 field3d
sudo port -v -N install -f openexr

# OpenColorIO 2 and 1
sudo port -v -N install minizip-ng pystring
[ -e /opt/local/include/minizip-ng ] || sudo ln -s . /opt/local/include/minizip-ng
sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
sudo port -v -N install opencolorio  -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314
sudo port -v -N install opencolorio1 -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314

# openfx-io (SeExpr 2.11 only, for SEEXPR_HOME)
sudo port -v -N install seexpr211

# openfx-arena
sudo port -v -N install librsvg -quartz
sudo port -v -N install poppler librevenge libcdr-0.1 sox libzip
sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
sudo port -v -N install ImageMagick +natron -x11
# ImageMagick 7, which openfx-arena uses since Natron 2.6 (static, in /opt/local/lib/ImageMagick7,
# so it doesn't conflict with ImageMagick 6 above)
sudo port -v -N install ImageMagick7 +natron -x11 -openexr

# openfx-gmic needs no extra port: its build downloads pocketfft, its FFT library

# openfx-io
sudo port -v -N install ffmpeg +gpl2 +highbitdepth +natronmini
# COMMERCIAL builds (NATRON_LICENSE=COMMERCIAL) only: LGPL FFmpeg, LibRaw and libheif (no x264/x265),
# installed in /opt/local/libexec next to the GPL ones (see "COMMERCIAL builds" in section 10)
sudo port -v -N install ffmpeg-lgpl libraw-lgpl libheif-lgpl
sudo port -v -N install openimageio +freetype +natron +opencolorio +openjpeg +python311 -python310 -python312 -python313 -python314
```

#### 8. Python 3.11, Qt 5 and PySide2

Python 3.10 is end-of-life and is no longer used (Python version policy, 2026-11-17): Qt 5 builds use Python 3.11 with Qt 5.15.19 and PySide2/Shiboken2 5.15.19 (MacPorts `python311`, `py311-pyside2`); Qt 6 builds use Python 3.13. To switch an existing install from Python 3.10, see [Switching from Python 3.10 to 3.11](#switching-from-python-310-to-311).

```Shell
sudo port -v -N install python311 py311-pip
sudo port -v -N install py311-numpy -gfortran
sudo port -v -N install py311-sphinx py311-sphinx_rtd_theme
sudo port -v -N install qt5 qt5-qtwebengine
sudo port -v -N install py311-pyside2   # its shiboken2 builds against the clang/llvm it depends on

sudo port select --set python python311
sudo port select --set python3 python311
sudo port select --set sphinx py311-sphinx
sudo pip-3.11 install qtpy==1.11.2

# links expected by the build scripts
PY=/opt/local/Library/Frameworks/Python.framework/Versions/3.11
sudo ln -sfn shiboken2 $PY/lib/python3.11/site-packages/shiboken2_generator/shiboken2-3.11
sudo ln -sfn shiboken2-3.11 /opt/local/bin/shiboken2
sudo ln -sfn Frameworks /opt/local/Library/lib
```

#### 9. Packaging tools (only needed to build the `.dmg`)

See [macOS packaging tools](#macos-packaging-tools) below.

#### 10. Build Natron and the DMG

```Shell
mkdir -p $HOME/Development/workspace
cd $NATRON_SRC/tools/jenkins
env NOUPDATE=1 WORKSPACE=$HOME/Development/workspace DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL \
    GIT_URL=https://github.com/NatronGitHub/Natron.git GIT_BRANCH=RB-2.6 \
    UNIT_TESTS=false BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 \
    COMPILE_TYPE=release MKJOBS=8 \
    ./launchBuildMain.sh
```

`launchBuildMain.sh` builds Natron with CMake (`tools/jenkins/build-natron.sh` configures it with the release options, runs `cmake --install` with `NATRON_INSTALL_LAYOUT=PORTABLE`, and stages the result for the installer script). Extra CMake options go in `NATRON_EXTRA_CMAKE_FLAGS`, e.g. `NATRON_EXTRA_CMAKE_FLAGS=-DNATRON_FAST_MATH=ON`. `NATRON_BUILD_SYSTEM=qmake` selects the deprecated qmake build, the only one with the Breakpad crash reporter (the CMake build implies `DISABLE_BREAKPAD=1`). It checks out Natron (`GIT_URL`, `GIT_BRANCH`) and the plug-ins from GitHub: local changes in `$NATRON_SRC` are not built, except for the `tools/` scripts and the overlay. To build local changes, see "Developing with the MacPorts SDK".

The `.dmg` ends up in `$HOME/Development/workspace/builds_archive/natron_github_RB2/1/`, named `Natron-<branch>-<date>-<commit>-macOS<version>-<arch>.dmg`. Use `UNIT_TESTS=true` to also run the render regression suite (Natron-Tests); this adds `-tests.txt` and `-unit_tests_failures.zip` next to the DMG. `BUILD_FROM=N` / `BUILD_TO=N` run only some of the six steps (checkout, plugins, Natron, installer, tests, archive).

The plug-ins (openfx-misc, openfx-io, openfx-arena, openfx-gmic) are built with their CMake builds, with `-DOFX_DEPLOY_BUNDLES=ON` (`tools/jenkins/build-plugins.sh`); `PLUGINS_BUILD_SYSTEM=make` builds them with their Makefiles instead.

For a release, build a tag instead of a branch:

```Shell
env NATRON_DEV_STATUS=STABLE RELEASE_TAG=2.6.0 NATRON_BUILD_NUMBER=1 NOUPDATE=1 WORKSPACE=$HOME/Development/workspace \
    DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL GIT_URL=https://github.com/NatronGitHub/Natron.git \
    UNIT_TESTS=true BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 COMPILE_TYPE=release MKJOBS=8 \
    ./launchBuildMain.sh
```

**COMMERCIAL builds.** `NATRON_LICENSE=COMMERCIAL` builds a DMG without any GPL component (#1302, #1177). It needs the overlay's subports `ffmpeg-lgpl`, `libraw-lgpl` and `libheif-lgpl` (install section 7), which install an LGPL FFmpeg (the same as `ffmpeg +highbitdepth +natronmini`, without `+gpl2` and libzvbi), a LibRaw without the demosaic packs and a libheif without the x264 and x265 encoders in `/opt/local/libexec/ffmpeg-lgpl`, `/opt/local/libexec/libraw-lgpl` and `/opt/local/libexec/libheif-lgpl`. openfx-io is built against the first two (`tools/jenkins/common.sh`), and `build-plugins.sh` replaces the libheif and LibRaw that openimageio links in the plug-in bundles with theirs (same version and ABI). Both `build-plugins.sh` and `build-OSX-installer.sh` then run `tools/jenkins/check-commercial-bundle.sh`, which fails the build if a bundled library is GPL, links a GPL library (x264, x265, Xvid, poppler, SoX, FFTW, ...), is an FFmpeg library whose licence (`avcodec_license()`, `ffmpeg -L`) is not LGPL 2.1 or later, is a LibRaw with a GPL demosaic pack, or is one of the Qt modules that are GPL-3 only (Qt Charts, Data Visualization, Network Authorization, Designer, ...) or their PySide2 module. `build-OSX-installer.sh` leaves these Qt frameworks and PySide2 modules out of COMMERCIAL DMGs: Natron doesn't use them. The check can also be run by hand on any app, bundle or directory: `tools/jenkins/check-commercial-bundle.sh Natron.app`. With tart: `tools/tart/natron-build.sh -l COMMERCIAL`.

Signing, notarization and the test suite are described in `Documentation/source/maintainers/build-macos.rst`.

#### 11. Keeping MacPorts up to date

Do this every few weeks, and before a release build. Each step is safe to stop after; the `sudo` steps need your password.

**MacPorts reads Natron's ports only from `/opt/MacPorts-Natron`, which is a copy of the `tools/MacPorts` directory of the checkout you last ran `./sync.sh` from.** If that was an old branch or another worktree, MacPorts silently uses its old Portfiles. So always sync from an up-to-date checkout of the branch you build (step 1), and check the copy (step 3).

1. **Update the checkout** of the branch you build (usually `RB-2.6`):

   ```Shell
   cd $NATRON_SRC && git switch RB-2.6 && git pull
   ```

2. **Use MacPorts, not Homebrew, in this terminal.** Run `port_switch` (see "MacPorts from scratch" above), or `export PATH=/opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin; unset HOMEBREW_PREFIX HOMEBREW_CELLAR HOMEBREW_REPOSITORY`. Then check that none of these resolve to `/opt/homebrew` or `/usr/local`:

   ```Shell
   which port pkg-config cmake python3
   ```

3. **Update MacPorts and publish the overlay:**

   ```Shell
   sudo port selfupdate
   cd $NATRON_SRC/tools/MacPorts
   ./sync.sh
   diff -rq -x 'PortIndex*' $NATRON_SRC/tools/MacPorts /opt/MacPorts-Natron
   ```

   `./sync.sh` must end with `Ports failed: 0`, and the `diff` must print nothing.

4. **Check the overlay against upstream:**

   ```Shell
   ./check.sh
   ```

   It lists each overlay Portfile (or `files/` patch) whose upstream version changed, with a suggested command. If it lists anything, bring the overlay up to date first, as described in "Reintegrating upstream Portfiles into the overlay" in the Maintainer Guide (`Documentation/source/maintainers/build-macos.rst`), commit the result, and run `./sync.sh` again from that checkout. You can also skip this and keep using the committed overlay: it still works, it's just behind upstream.

   Some overlay ports are deliberately **pinned** to a version other than upstream's. Their `Portfile` is kept when reintegrating; only `Portfile.orig` follows upstream:

   | port | pinned version | reason |
   |---|---|---|
   | `graphics/libraw` | 0.22.3 (upstream: 0.22.2) + patch | the GPL demosaic packs (`+gpl2`) are restored by a Natron patch |
   | `graphics/opencolorio` | 2.6.0 (upstream: 2.5.1) | VFX Reference Platform CY2027; keep it until upstream reaches 2.6, and rev-bump `openimageio` when the OCIO minor version changes |
   | `print/freetype` | 2.15.0_1 (upstream: 2.15.0_0) + patch | backport of FreeType's unreleased fix 05f343a: 2.15.0 resets reflected transforms rotated within about 1.8° of 45°, 135°, 225° or 315° to the identity, so such text in openfx-arena's Text comes out unrotated (openfx-arena #86, #88); unpin when MacPorts ships a FreeType release that contains 05f343a (2.15.1 or later). Check that upstream has 05f343a before unpinning: `check.sh` only compares version and revision, so a rev-bump alone isn't enough. `sudo port -v upgrade outdated` picks up the revision bump |

5. **See what would be upgraded,** now that the overlay is published:

   ```Shell
   port outdated
   ```

6. **Upgrade the installed ports:**

   ```Shell
   sudo port -v upgrade outdated
   ```

   This can pull in new dependencies or variants, and can take hours when a big port (Qt, LLVM) changed. If it stops on a failure, upgrade the remaining ports one by one, so that one broken port doesn't block the others:

   ```Shell
   sudo -v; for p in $(port -q echo outdated | awk '{print $1}'); do echo "== $p"; sudo port -v upgrade "$p"; done 2>&1 | tee /tmp/natron-port-upgrade.log
   ```

   MacPorts never downgrades a port. If a pinned port was installed in a newer version, remove the inactive versions first (`port uninstall` refuses to choose when several versions are installed), then uninstall it and install it again with the variants of install section 7:

   ```Shell
   sudo port uninstall inactive
   sudo port -f uninstall libraw && sudo port -v -N install libraw +gpl2
   ```

   If `port installed ffmpeg` shows `+vmaf`, reinstall it the same way with `+gpl2 +highbitdepth +natronmini`.

7. **Re-apply the install fixups.** An upgrade of the ports concerned can undo them. Re-run the `ln`, `sed`, `port select` and `pip` commands of install sections 7 (the `minizip-ng` link and `sed`, the `libzip.pc` `sed`) and 8 (`port select` for `python`, `python3` and `sphinx`, `pip-3.11 install qtpy==1.11.2`, and the `shiboken2` and `/opt/local/Library/lib` links). They are safe to run again. In tart, the `fixups` step of `tools/tart/provision.sh` re-applies the links, `sed`s and `port select`s after every update (`qtpy` is installed by its `python_qt` step).

8. **Reclaim disk space:**

   ```Shell
   sudo port uninstall inactive
   sudo port clean --work --dist installed
   sudo port reclaim
   ```

   `port clean --work --dist installed` removes the build directories and the downloaded source archives of the installed ports, and keeps their logs. Don't delete `/opt/local/var/macports/software`: MacPorts needs those archives to deactivate and reactivate ports.

9. **Rebuild and test,** if the upgrade touched anything Natron or the plug-ins link to: `qt5`, `py311-pyside2`, `python311`, `boost`, `ffmpeg`, `openimageio`, `opencolorio`, `libraw`, `openexr`, `ImageMagick`, `ImageMagick7`, `poppler`, `cairo`, `freetype`, … Rebuild from a fresh build directory (install section 10, or "Developing with the MacPorts SDK" below), clear Natron's cache (`rm -rf ~/Library/Caches/INRIA/Natron`), and run the unit tests (see [Testing](#testing)). Before a release, also run the Natron-Tests regression suite on the new DMG (`UNIT_TESTS=true` with `launchBuildMain.sh`, see install section 10, or `runTests.sh`, see "Testing" in `Documentation/source/maintainers/build-macos.rst`).

With the tart VMs (`tools/tart`), `tools/tart/natron-sdk.sh update` does steps 3 to 7 inside a copy of the SDK image: it installs the MacPorts version set by `MACPORTS_VERSION` in `tools/tart/provision.sh` and syncs the ports tree (instead of `selfupdate`), copies the host checkout's `tools/` into the VM (so update the host checkout first, step 1), runs `check.sh` (and only warns if the overlay is behind upstream: reintegrate on the host first), `./sync.sh`, reinstalls the pinned ports whose installed version differs from the overlay, upgrades the outdated ports one by one, installs `ImageMagick7 +natron` if the image doesn't have it yet, and re-applies the fixups. The previous image is kept as `natron-sdk-sequoia-prev`. See `tools/tart/README.md`.

#### Switching from Python 3.10 to 3.11

Until 2026-11-17 the SDK used Python 3.10, which is end-of-life. On a machine installed before, after the routine above (steps 1 to 6), install the Python 3.11 ports, switch `openimageio`'s Python bindings, and redo the selections and links of install sections 8 and 9:

```Shell
sudo port -v -N install python311 py311-pip
sudo port -v -N install py311-numpy -gfortran
sudo port -v -N install py311-sphinx py311-sphinx_rtd_theme
sudo port -v -N install py311-pyside2       # builds from source if there is no binary archive: allow an hour or more
sudo port -v -N install py311-dmgbuild      # only to build the .dmg
# -n: only openimageio is rebuilt (without it, --enforce-variants would apply the variants to its dependencies)
sudo port -n -v -N upgrade --enforce-variants openimageio +freetype +natron +opencolorio +openjpeg \
    +python311 -python310 -python312 -python313 -python314

sudo port select --set python python311
sudo port select --set python3 python311
sudo port select --set sphinx py311-sphinx
sudo pip-3.11 install qtpy==1.11.2

PY=/opt/local/Library/Frameworks/Python.framework/Versions/3.11
sudo ln -sfn shiboken2 $PY/lib/python3.11/site-packages/shiboken2_generator/shiboken2-3.11
sudo ln -sfn shiboken2-3.11 /opt/local/bin/shiboken2
sudo ln -sfn dmgbuild-3.11 /opt/local/bin/dmgbuild   # if py311-dmgbuild is installed
```

If you built Mesa 25.0.7 (install section 4), also `sudo port -v -N install py311-mako py311-yaml`; it only needs them to rebuild Mesa.

Check that the build sees Python 3.11: `python3 --version`, `ls -l /opt/local/bin/shiboken2`, and `port installed openimageio` (it must show `+python311`). `opencolorio` and `opencolorio1` are installed without Python bindings and need no change. Then rebuild Natron from a **fresh** build directory, with `-DPython3_EXECUTABLE=/opt/local/bin/python3.11` (a build directory configured with Python 3.10 keeps it in its CMake cache), and run the tests (step 9).

Once Natron builds and passes the tests, remove Python 3.10 and everything that uses it. Remove the inactive ports first: the variant switch leaves the old `openimageio +python310` installed but inactive, and it would still show up as a dependent of `python310`. Then check that `port rdependents python310` (recursive) lists only `py310-*` ports; if the active `openimageio` is listed, the variant switch above didn't happen, so don't go on:

```Shell
sudo port uninstall inactive
port rdependents python310
sudo port uninstall --follow-dependents python310
sudo port uninstall python3_select-310   # the python3 selector entry for 3.10, a dependency of python310
port echo leaves                         # review: unrequested ports nothing depends on any more
```

The `pip`, `cython` and `sphinx` selector entries for 3.10 belong to the `py310-*` ports and go away with them. Uninstall the ports listed by `port echo leaves` only if you don't need them (`sudo port uninstall leaves` removes them all, including build dependencies such as `clang-18` that a later rebuild of `py311-pyside2` would install again).

With tart, `tools/tart/natron-sdk.sh update` does all of this except the removal of Python 3.10 (`step_python_migrate` in `tools/tart/provision.sh`, then its `fixups` step).

#### After a macOS upgrade

A new major version of macOS (e.g. 15 to 26) needs a new MacPorts and a reinstall of every port: ports built for the previous macOS may not work any more, and `port selfupdate` doesn't fix that. This follows the [MacPorts migration procedure](https://trac.macports.org/wiki/Migration).

1. **Update Xcode** from the App Store for the new macOS, then redo install section 1 (`xcode-select -s` and `xcodebuild -license accept`).

2. **Install the MacPorts `.pkg` for the new macOS** from <https://www.macports.org/install.php>. It keeps your `/opt/local/etc/macports/sources.conf`. Check that the overlay is still registered (otherwise redo install section 3):

   ```Shell
   grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf && port file openimageio
   ```

   `port file openimageio` must print a path under `/opt/MacPorts-Natron`.

3. **Use MacPorts, not Homebrew,** and update the checkout (steps 1 and 2 of "Keeping MacPorts up to date").

4. **Save the list of installed and requested ports,** after removing the inactive versions (otherwise they are reinstalled too), and keep a copy:

   ```Shell
   sudo port uninstall inactive
   port -qv installed > ~/myports.txt
   port echo requested | cut -d ' ' -f 1 | uniq > ~/requested.txt
   test -s ~/myports.txt && test -s ~/requested.txt && cp ~/myports.txt ~/requested.txt ~/Desktop/
   ```

   Don't go on unless the `test` succeeded.

5. **Uninstall all ports,** then clean up:

   ```Shell
   sudo port -fp uninstall installed
   sudo port reclaim
   ```

6. **Publish the overlay again,** so that the reinstall uses Natron's ports:

   ```Shell
   cd $NATRON_SRC/tools/MacPorts && ./sync.sh
   diff -rq -x 'PortIndex*' $NATRON_SRC/tools/MacPorts /opt/MacPorts-Natron
   ```

7. **Rebuild OSMesa** if you use it (install section 4). It must be built before the ports that need it, so do it now, before the reinstall.

8. **Reinstall the ports** with MacPorts' `restore_ports.tcl` script, which reinstalls them with the same variants. Download it outside the Natron checkout:

   ```Shell
   cd /tmp
   curl --location --remote-name https://github.com/macports/macports-contrib/raw/master/restore_ports/restore_ports.tcl
   chmod +x restore_ports.tcl
   sudo ./restore_ports.tcl ~/myports.txt
   sudo port unsetrequested installed
   xargs sudo port setrequested < ~/requested.txt
   ```

   (A file downloaded with a web browser instead of `curl` also needs `xattr -d com.apple.quarantine restore_ports.tcl`.) If a port fails, reinstall it by hand with the commands and variants of install sections 4 to 9, which are the reference for Natron's variants (`ImageMagick +natron`, `ImageMagick7 +natron`, `ffmpeg +natronmini`, `libraw +gpl2`, ...).

9. **Re-apply the install fixups** (step 7 of "Keeping MacPorts up to date"): `restore_ports.tcl` doesn't restore `port select` choices, `pip` packages or hand-made links.

10. **Check the compiler** (install section 5), then rebuild from a fresh build directory and test (step 9 of "Keeping MacPorts up to date"). A clean rebuild is mandatory after a macOS upgrade.

#### Developing with the MacPorts SDK

`launchBuildMain.sh` builds everything from fresh checkouts. To work on Natron or the plugins directly, build them from your own checkouts.

The OpenFX plugins ([openfx-io](https://github.com/NatronGitHub/openfx-io), [openfx-misc](https://github.com/NatronGitHub/openfx-misc), [openfx-arena](https://github.com/NatronGitHub/openfx-arena), [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic)) build the way `tools/jenkins/build-plugins.sh` builds them: openfx-misc, openfx-io and openfx-arena with CMake (#1203, #1280), openfx-gmic with `make`. The bundles go to `~/OFX/Natron-release`, which is the directory that `OFX_PLUGIN_PATH` points to in the commands below.

```Shell
mkdir -p ~/src && cd ~/src
for p in openfx-io openfx-misc openfx-arena openfx-gmic; do git clone --recursive https://github.com/NatronGitHub/$p.git; done
```

openfx-misc, openfx-io and openfx-arena use openfx's CMake helpers. Build them with the MacPorts clang (for OpenMP), as C++17 with a Qt5 Natron (`-DOFX_CXX_STANDARD=20` with Qt6), and install the bundles straight into `~/OFX/Natron-release`. openfx-arena needs the ImageMagick 7 of install section 7 (`ImageMagick7 +natron`, in `/opt/local/lib/ImageMagick7`): its pkg-config files must come first, as `build-plugins.sh` does with `MAGICK7_PREFIX`, or pkg-config finds the ImageMagick 6 of `/opt/local` instead.

```Shell
for p in openfx-misc openfx-io openfx-arena; do
  cd ~/src/$p
  env PKG_CONFIG_PATH=/opt/local/lib/ImageMagick7/lib/pkgconfig \
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_CXX_COMPILER=clang++-mp-$CLANG -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_PREFIX_PATH=/opt/local -DSEEXPR_HOME=/opt/local -DOFX_CXX_STANDARD=17 \
      -DPLUGIN_INSTALLDIR=$HOME/OFX/Natron-release
  cmake --build build && cmake --install build
done
```

CMake's `Release` uses `-O3`, and `build-plugins.sh` uses `-O3 -g` for its `RelWithDebInfo` builds, without fast math (`-Ofast` would break the plug-ins' NaN and infinity checks). `build-plugins.sh` also passes `-DOFX_DEPLOY_BUNDLES=ON`, which copies the MacPorts libraries into the bundles for the DMG; a development build doesn't need it. `SEEXPR_HOME` is only used by openfx-io, and `PKG_CONFIG_PATH` only by openfx-arena: the others ignore them (openfx-misc warns that `SEEXPR_HOME` is unused). For CPU rendering in Shadertoy, add `-DUSE_OSMESA=ON` for openfx-misc. It uses the OSMesa and LLVM of install section 4: `/opt/osmesa` and `/opt/llvm` by default, or set `-DOSMESA_PATH` and `-DLLVM_PATH`. openfx-arena builds with OpenMP, as Natron's ImageMagick 7 does. The CMake options are listed in each repository's `README.md` and `CMakeLists.txt`.

openfx-gmic builds with `make` (`CONFIG=debug` or `release`, OpenMP with the MacPorts clang, no OpenGL), at `-O3` like the CMake builds:

```Shell
cd ~/src/openfx-gmic
env OPENMP=1 CXX="clang++-mp-$CLANG -stdlib=libc++ -std=c++17" OPTFLAG=-O3 \
    CXXFLAGS_EXTRA="-isysroot $(xcrun --sdk macosx --show-sdk-path)" \
    CONFIG=release make -j8
```

Use `-std=c++17` with a Qt5 Natron and `-std=c++20` with a Qt6 Natron (RB-2.6 policy; the packaging scripts do this automatically).

The bundle is written to `GMIC_OFX/Darwin-64-release/GMIC.ofx.bundle`. To use it, symlink it next to the others (or use `/Library/OFX/Plugins`):

```Shell
ln -sf ~/src/openfx-gmic/GMIC_OFX/Darwin-64-release/GMIC.ofx.bundle ~/OFX/Natron-release/
```

Natron itself builds with CMake, the system (Xcode) clang and the MacPorts libraries (see [Building with CMake](#building-with-cmake) below), from `$NATRON_SRC`, with Homebrew off the `PATH`:

```Shell
cd $NATRON_SRC
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DNATRON_BUILD_TESTS=ON \
    -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
    -DPython3_EXECUTABLE=/opt/local/bin/python3.11
cmake --build build -j$(sysctl -n hw.ncpu)
```

The application is then `build/App/Natron.app`. The release scripts add the options of the official builds: `-DNATRON_BUILD_CONFIG=SNAPSHOT` (or `ALPHA`, `BETA`, `RC`, `STABLE`), `-DNATRON_USE_OPENMP=ON` with a MacPorts clang, `-DNATRON_NO_ASSERTIONS=ON` and `-DNATRON_INSTALL_LAYOUT=PORTABLE` (see `Documentation/source/maintainers/building.rst`). The deprecated qmake build, still available on RB-2.6, uses the command below (see [Deprecated: building with qmake](#deprecated-building-with-qmake) for `config.pri`).

```Shell
cd $NATRON_SRC
cp config-macports.pri config.pri
/opt/local/libexec/qt5/bin/qmake -r CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa \
    CONFIG+=openmp CONFIG+=python3 CONFIG+=sdk_no_version_check CLANG_MP_VERSION=$CLANG
QT_MAC_SDK_NO_VERSION_CHECK=1 make -j8
```

`CLANG_MP_VERSION` selects the MacPorts clang used with `CONFIG+=openmp` (see `config-macports.pri`).

The OCIO configuration comes from the OpenColorIO-Configs download (see [Download OpenColorIO-Configs](#download-opencolorio-configs) below). To run a development build, or the [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests) suite (`runTests.sh <path to NatronRenderer>`), set the environment that the packaged application normally provides:

```Shell
env FONTCONFIG_FILE=$NATRON_SRC/Gui/Resources/etc/fonts/fonts.conf \
    OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio \
    OFX_PLUGIN_PATH=~/OFX/Natron-release NATRON_PLUGIN_PATH=$NATRON_SRC/Gui/Resources/PyPlugs \
    build/App/Natron.app/Contents/MacOS/Natron
```

(with the qmake build, the application is `App/Natron.app`).

Clear the OFX plugin cache (`~/Library/Caches/INRIA/Natron`) after rebuilding plugins. The plugin reference pages of the documentation are generated with `tools/genStaticDocs.sh` (see `Documentation/README-Natron-documentation.md`).

#### Notes for older systems

On OS X 10.6 Snow Leopard, the `+universal` variant may be added to `/opt/local/etc/macports/variants.conf`. Older releases were built in an OS X 10.9 VM with Xcode 6.2 to target old systems. Due to the OS X Software License Agreement, OS X may only be virtualized on Mac hardware; see [OS X on OS X](https://web.archive.org/web/20190520141144/https://ntk.me/2012/09/07/os-x-on-os-x/).

To keep such a VM small, keep only macOS and Xcode in it and put `/opt` on a sparsebundle disk image stored on the host. Create it on the host with Disk Utility, using a name without spaces (MacPorts doesn't support paths with spaces), e.g. `opt`, with a maximum size of about 100 GB. In the guest, mount the host share (`open smb://<host-ip>/<user>`), attach the image with `sudo hdiutil attach -owners on /Volumes/<user>/path/to/opt.sparsebundle`, and link it: `sudo rmdir /opt; sudo ln -s /Volumes/opt /`.

Known workarounds on 10.6 (universal builds):

- `cmake` may have to be upgraded with the MacPorts clang: `sudo port -v upgrade cmake configure.compiler=macports-clang-11`.
- ImageMagick: `magick/resize.c` must be compiled without OpenMP for i386 ([LLVM bug 24038](https://bugs.llvm.org/show_bug.cgi?id=24038)). Rebuild that object by hand without `-fopenmp` in the port's work directory, then `sudo port -v upgrade ImageMagick`.
- `py27-shiboken`: remove `universal_variant no` from the upstream `devel/sparsehash` Portfile, then `sudo port -v install sparsehash +universal configure.compiler=macports-clang-11`.
- For libc++ on older systems, follow <https://trac.macports.org/wiki/LibcxxOnOlderSystems>, and copy `/usr/lib/libc++*` into `/Developer/SDKs/MacOSX10.6.sdk/usr/lib`.
- Universal FFmpeg builds may keep install names that point into the i386 destroot. Fix them with `install_name_tool -change <destroot path>/lib/libavXXX.dylib /opt/local/lib/libavXXX.dylib` on `bin/ffmpeg`, `bin/ffprobe` and the `libav*` libraries.

The libtool that comes with OS X 10.6 does not work well with clang-generated binaries. On that system you may have to substitute it with the libtool provided by MacPorts' `cctools` package: `sudo mv /usr/bin/libtool /usr/bin/libtool.orig; sudo mv /Developer/usr/bin/libtool /Developer/usr/bin/libtool.orig; sudo ln -s /opt/local/bin/libtool /usr/bin/libtool; sudo ln -s /opt/local/bin/libtool /Developer/usr/bin/libtool`

Qt 4 builds used `qt4-mac` with `py27-pyside`/`py27-shiboken`; the Python 2 PySide ports are no longer available in current MacPorts.

### Homebrew

*Community-maintained, not tested for Natron 2.6. The paths below assume an Intel Mac (`/usr/local`); on Apple Silicon, use `$(brew --prefix)` (`/opt/homebrew`) instead. Some formulae and options mentioned here no longer exist in current Homebrew.*

Install Homebrew from <http://brew.sh/>

#### Qt5

Install Qt5

```Shell
brew install qt@5 pyside@2
# compilation doesn't work with Qt6 linked
brew unlink qt
# Fix the pkg-config files
brew install gnu-sed
gsed -e s@libdir=lib@libdir=\${prefix}/lib@ -i /usr/local/opt/pyside\@2/lib/pkgconfig/pyside2.pc
gsed -e s@libdir=lib@libdir=\${prefix}/lib@ -i /usr/local/opt/pyside\@2/lib/pkgconfig/shiboken2.pc
```

#### Building with CMake (untested)

Pointing `CMAKE_PREFIX_PATH` at the formulae should work if `pyside@2` has CMake config files; otherwise also pass `-DPython3_EXECUTABLE` for the Python that `pyside@2` was built for, so that CMake finds PySide2 through it (`cmake/PySide2FromPython.cmake`). This is not tested; the supported way is [MacPorts](#on-macports).

```Shell
cmake -S . -B build -DCMAKE_PREFIX_PATH="$(brew --prefix expat);$(brew --prefix qt@5);$(brew --prefix pyside@2)"
cmake --build build -j$(sysctl -n hw.ncpu)
```

#### Python2 (optional)

Install Python 2.7 (yes, we know it's deprecated).
```Shell
brew uninstall python@2 || true
cp tools/homebrew/python@2.rb $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core/Formula/python@2.rb
brew install -s python@2
```

Do the following if the `homebrew.pth` file does not exist:

```Shell
mkdir -p ~/Library/Python/2.7/lib/python/site-packages
echo 'import site; site.addsitedir("/usr/local/lib/python2.7/site-packages")' >> ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth
sudo ln -s ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth /Library/Python/2.7/lib/python/site-packages/homebrew.pth
```

#### Qt4 (optional)

Qt 4 is not supported in Homebrew. Please enable the community-maintained recipe using:

```Shell
brew tap cartr/qt4
```

Optional: Patch the qt4 recipe to fix the stack overflow issue (see [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [Homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793)).

Patching a Homebrew recipe is explained in the [Homebrew FAQ](https://github.com/Homebrew/homebrew/blob/master/share/doc/homebrew/FAQ.md).

```Shell
brew edit cartr/qt4/qt@4
brew install cartr/qt4/qt@4 cartr/qt4/shiboken@1.2 cartr/qt4/pyside@1.2 cartr/qt4/pyside-tools@1.2
```

and before the line that starts with `head`, add the following code:

```
  patch :p0 do
    # bugreports.qt.io may go away, use our local copy.
    # url "https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff"
    # sha256 "477630235eb5ee34ed04e33f156625d1724da290e7a66b745611fb49d17f1347"
    url "https://raw.githubusercontent.com/NatronGitHub/Natron/ff4b3afd3a784f1517002bd82e2da441265385ad/tools/MacPorts/aqua/qt4-mac/files/patch-qthread-stacksize.diff"
    sha256 "a3363ff6460fb4cb4a2a311dbc0724fc5de39d22eb6fa1ec1680d6bd28d01ee4"
  end

  patch :p0 do
    url "https://raw.githubusercontent.com/NatronGitHub/Natron/ff4b3afd3a784f1517002bd82e2da441265385ad/tools/MacPorts/aqua/qt4-mac/files/patch-qt-custom-threadpool.diff"
    sha256 "470c8bf6fbcf01bd15210aad961a476abc73470e201ccb4d62a7c579452de371"
  end
```

Reinstall Qt4 from source (takes a while):
```Shell
brew reinstall -s cartr/qt4/qt@4
```

#### Other dependencies

Install libraries:

```Shell
brew install cairo expat
brew install gnu-sed gawk coreutils findutils
brew install cmake keychain sphinx-doc
/usr/local/opt/sphinx-doc/libexec/bin/pip3 install sphinx_rtd_theme
brew install pyside@1.2 pyside-tools@1.2
```

If compiling with Qt5:

```Shell
brew install pyside@2
```


 To install the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugin, you also need the following:

```Shell
brew install ilmbase openexr freetype fontconfig ffmpeg
brew unlink openimageio || true;
brew unlink opencolorio || true;
brew unlink seexpr || true;
cp tools/homebrew/opencolorio@1.rb $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core/Formula
cp tools/homebrew/seexpr@2.rb $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core/Formula
brew install -s opencolorio@1
brew unlink qt@4
brew install -s seexpr@2
brew link qt@4
brew install opencolorio openimageio seexpr
```

To install the [openfx-arena](https://github.com/NatronGitHub/openfx-arena) set of plugin, you also need the following:

```Shell
brew install librsvg poppler librevenge libcdr libzip sox
brew uninstall imagemagick
brew install imagemagick --with-hdri --with-librsvg --with-quantum-depth-32 --with-pango
```

also set the correct value for the pkg-config path (you can also put
this in your .bash_profile):

```Shell
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/curl/lib/pkgconfig
```

### Installing manually (outside of MacPorts or Homebrew) a patched Qt to avoid stack overflows

The following is not necessary if a patched Qt was already installed by the standard MacPorts or Homebrew procedures above.

See [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [Homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793).

```Shell
wget https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz
tar zxvf qt-everywhere-opensource-src-4.8.7.tar.gz
cd qt-everywhere-opensource-src-4.8.7
wget https://raw.githubusercontent.com/Homebrew/patches/480b7142c4e2ae07de6028f672695eb927a34875/qt/el-capitan.patch
patch -p1 < el-capitan.patch
wget https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff
patch -p0 < patch-qthread-stacksize.diff
```

Then, configure using:

```Shell
./configure -prefix /opt/qt4 -pch -system-zlib -qt-libtiff -qt-libpng -qt-libjpeg -confirm-license -opensource -nomake demos -nomake examples -nomake docs -cocoa -fast -release
```

* On OS X >= 10.9 add `-platform unsupported/macx-clang-libc++`
* On OS X < 10.9, to compile with clang add `-platform unsupported/macx-clang`
* If compiling with gcc/g++, make sure that `g++ --version`refers to Apple's gcc >= 4.2 or clang >= 318.0.61
* To use another openssl than the system (mainly for security reasons), add `-openssl-linked -I /usr/local/Cellar/openssl/1.0.2d_1/include -L /usr/local/Cellar/openssl/1.0.2d_1/lib` (where the path is changed to your openssl installation).

Then, compile using:

```Shell
make
```

And install (after making sure `/opt/qt4` is user-writable) using:

```Shell
make install
```


## macOS packaging tools

In addition to the compile-time dependencies above, building the distributable
`.dmg` with `tools/jenkins/launchBuildMain.sh` requires a few extra tools. They
are verified early by the build script (the `PKGOS = OSX` check in
`launchBuildMain.sh`), so a missing tool fails fast with a clear message instead
of after a multi-hour build:

- **dmgbuild** — writes the DMG window background and icon layout directly into
  the disk image's `.DS_Store`, without driving Finder (the old AppleScript
  approach needs GUI Automation / TCC permission and fails in headless or
  unprivileged sessions). With MacPorts, install the port and link the
  versioned script (the port only installs `dmgbuild-3.11`):

  ```Shell
  sudo port -v -N install py311-dmgbuild
  sudo ln -sfn dmgbuild-3.11 /opt/local/bin/dmgbuild
  ```

  Otherwise, install it with pip: `python3 -m pip install dmgbuild`.

- **oiiotool** — provided by the `openimageio` port (already installed as a
  Natron dependency); used to contrast-reduce the DMG background image.
- **identify** — provided by the `ImageMagick` port (already installed as a
  Natron dependency); used to read the background image dimensions.

If you add a new macOS build/packaging dependency, add it to the early check in
`launchBuildMain.sh` and document it here.

## Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz).
Place it at the root of Natron source tree:

```Shell
curl -k -L https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz | tar zxf -
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

## Building with CMake

CMake is the supported build. It builds with Qt 5 and PySide2 by default; `-DNATRON_QT6=ON` selects Qt 6 and PySide6 (not covered by the macOS CI; following the Python version policy, use Python 3.13 with it, i.e. MacPorts `python313` and `py313-pyside6`, and `-DPython3_EXECUTABLE=/opt/local/bin/python3.13`). Other options: `-DCMAKE_BUILD_TYPE=Debug|Release|RelWithDebInfo`, `-DNATRON_BUILD_TESTS=OFF` (skip the unit tests), `-DNATRON_SYSTEM_LIBS=ON` (use system copies of the libraries in `libs/`). `cmake --install build --prefix <dir>` installs Natron; `-DNATRON_INSTALL_LAYOUT=PORTABLE` installs a self-contained `Natron.app`, as the release scripts will (see `Documentation/source/maintainers/building.rst`).

### On MacPorts

MacPorts' `py311-pyside2` has no CMake config files: CMake then finds Shiboken2 and PySide2 through `Python3_EXECUTABLE` (see `cmake/PySide2FromPython.cmake`). Build with the system (Xcode) clang and the MacPorts libraries, from `$NATRON_SRC`, with Homebrew off the `PATH`:

```Shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++ \
    -DCMAKE_PREFIX_PATH="/opt/local/libexec/qt5;/opt/local" \
    -DPython3_EXECUTABLE=/opt/local/bin/python3.11
cmake --build build -j$(sysctl -n hw.ncpu)
(cd build && env OFX_PLUGIN_PATH=$HOME/OFX/Natron-release OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio ctest -V)
```

These are the macOS CI job's commands (`.github/workflows/ci.yml`), except that CI has no OFX plug-ins: it leaves `OFX_PLUGIN_PATH` unset and skips the tests that need them with `GTEST_FILTER='-BaseTest.*'` in the `ctest` environment.

The shiboken2 generator parses the headers with its own libclang (LLVM 18 for PySide2 5.15), which can't parse the libc++ headers of newer SDKs (`use of undeclared identifier '__builtin_clzg'`, seen with the macOS 27 SDK of Xcode 27). If that happens, point `SDKROOT` at an older SDK, e.g. `export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.sdk`. It must be exported in the environment of `cmake --build` too, not only at configure time: shiboken runs during the build, and without `SDKROOT` it picks the Xcode SDK headers again. The same applies to the qmake build and to `launchBuildMain.sh`.

## Deprecated: building with qmake

*The qmake build (for Qt4 and Qt5) is deprecated. It still works on RB-2.6, where the release scripts in `tools/jenkins` use it with `NATRON_BUILD_SYSTEM=qmake`, but it will be removed in a future release (first on RB-2.7, see [#1181](https://github.com/NatronGitHub/Natron/issues/1181)).*

### Add the config.pri file

You have to define the locations of the required libraries.
This is done by creating a .pri file that will tell the .pro where to find those libraries.
The only library to put in the config.pri file on unix systems is boost.
For all other libraries are found with PKGConfig.

- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

INCLUDEPATH is the path to the include files

LIBS is the path to the libs

If you installed libraries using MacPorts, use the following
config.pri:

```Shell
# copy and paste the following in a terminal
cp config-macports.pri config.pri
```

If you installed libraries using Homebrew, use the following
config.pri:

```Shell
# copy and paste the following in a terminal
cp config-homebrew.pri config.pri
```

Then check at the top of the `config.pri` file that the `HOMEBREW` variable is set to the homebrew installation prefix (usually `/opt/homebrew`).

### Build with Makefile

You can generate a makefile by opening a Terminal, setting the current
directory to the toplevel source directory, and typing

```Shell
qmake -r
```

then type

```Shell
make
```

This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

```Shell
qmake -r CONFIG+=debug
```

* You can also enable logging by adding CONFIG+=log

* You can also enable clang sanitizer by adding CONFIG+=sanitizer

#### Building with OpenMP support using clang

*Legacy (clang 9, Qt 4). For current macOS with MacPorts, see step 5 and "Developing with the MacPorts SDK" above.*

It is possible to build Natron using clang (version 3.8 is required,
version 9.0 is recommended) with OpenMP support on
MacPorts (or Homebrew for OS X 10.9 or later).  OpenMP brings speed improvements in the
tracker and in CImg-based plugins.

First, install clang 9.0. On OS X 10.9 and later with MacPorts, simply execute:

```Shell
sudo port -v install clang-9.0
```

Or with Homebrew:

```Shell
brew install llvm
```

On older systems, follow the procedure described in [Using libc++ on older systems](https://trac.macports.org/wiki/LibcxxOnOlderSystems), and install and set clang-9.0 as the default compiler in the end. Note that we noticed clang 3.9.1 generates wrong code with `-Os` when compiling openexr (later clang versions were not checked), so it is safer to also change `default configure.optflags      {-Os}` to `default configure.optflags      {-O2}` in `/opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` (type `sudo nano /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` to edit it).

On OS X 10.6, see also the libtool note in "Notes for older systems" above.

Then, configure using the following qmake command on MacPorts:

```Shell
/opt/local/libexec/qt4/bin/qmake QMAKE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_CC=clang-mp-9.0 QMAKE_OBJECTIVE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-9.0 -stdlib=libc++' QMAKE_LD='clang++-mp-9.0 -stdlib=libc++' -r CONFIG+=openmp CONFIG+=enable-osmesa CONFIG+=enable-cairo
```

Or on Homebrew with Qt4/PySide from cartr/qt4:

```Shell
QT_INSTALL_PREFIX=/usr/local
qmake=$QT_INSTALL_PREFIX/bin/qmake
env PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/expat/lib/pkgconfig $qmake -spec macx-xcode CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa CONFIG+=python3 CONFIG+=sdk_no_version_check CONFIG+=openmp -r
```

Or on Homebrew with Qt5/PySide2:

```Shell
QT_INSTALL_PREFIX=/usr/local/opt/qt@5
qmake=$QT_INSTALL_PREFIX/bin/qmake
env PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/expat/lib/pkgconfig:/usr/local/opt/qt@5/lib/pkgconfig:/usr/local/opt/pyside@2/lib/pkgconfig $qmake -spec macx-xcode CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa CONFIG+=python3 CONFIG+=sdk_no_version_check CONFIG+=openmp -r
```

To build the plugins, use the following command-line:

```Shell
make CXX='clang++-mp-9.0 -stdlib=libc++' OPENMP=1
```

Or, if you have MangledOSMesa32 installed in `OSMESA_PATH` and LLVM installed in `LLVM_PATH` (MangledOSMesa32 and LLVM build script is available from [osmesa-install](https://github.com/devernay/osmesa-install)):

```Shell
OSMESA_PATH=/opt/osmesa
LLVM_PATH=/opt/llvm
make CXX='clang++-mp-9.0 -stdlib=libc++' OPENMP=1 CXXFLAGS_MESA="-DHAVE_OSMESA" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -lMangledOSMesa32 `${LLVM_PATH}/bin/llvm-config --ldflags --libs engine mcjit mcdisassembler | tr '\n' ' '`" OSMESA_PATH="${OSMESA_PATH}"
```

### Build on Xcode

Follow the instruction of build but
add -spec macx-xcode to the qmake call command:

```Shell
qmake -r -spec macx-xcode
```

Then open the already provided Project-xcode.xcodeproj and compile the target "all"

#### Compiling plugins with Xcode

The source distributions of the plugin sets `openfx-io` and
`openfx-misc` contain Xcode projects, but these require setting a few
global variables in Xcode. These variables can be used to switch
between the system-installed version of a package and a custom install
(e.g. if you need to debug something that happens in OpenImageIO).

In Xcode Preferences, select "Locations", then "Source Trees", and add the following
variable names/values (Xcode may need to be restarted after setting these):
- `LOCAL`: `/usr/local` on Homebrew, `/opt/local` on MacPorts
- `BOOST_PATH`: `$(LOCAL)/include`
- `EXR_PATH`: `$(LOCAL)`
- `FFMPEG_PATH`: `$(LOCAL)`
- `OCIO_PATH`: `$(LOCAL)`
- `OIIO_PATH`: `$(LOCAL)`
- `OPENCV_PATH`: `$(LOCAL)`
- `SEEXPR_PATH`: `$(LOCAL)`

It is also recommended in Xcode Preferences, select "Locations", then
"Locations", to set the Derived Data location to be Relative, and in
the advanced settings to set the build location to Legacy (if not,
build files are somewhere under `~/Library/Developer/Xcode`.

#### Build on Xcode with openmp clang

*Legacy (clang 9).*

See instructions under "Using clang-omp with Xcode" at the following page https://clang-omp.github.io

On Macports clang now ships with openmp by default. To install it:
```
sudo port install clang-9.0
```

In your config.pri file, add the following lines and change the paths according to your installation of clang

```
openmp {
INCLUDEPATH += /opt/local/include/libomp
LIBS += -L/opt/local/lib/libomp -liomp5 # may also be -lomp

cc_setting.name = CC
cc_setting.value = /opt/local/bin/clang-mp-9.0
cxx_setting.name = CXX
cxx_setting.value = /opt/local/bin/clang++-mp-9.0 -stdlib=libc++
ld_setting.name = LD
ld_setting.value = /opt/local/bin/clang-mp-9.0
ldplusplus_setting.name = LDPLUSPLUS
ldplusplus_setting.value = /opt/local/bin/clang++-mp-9.0 -stdlib=libc++
QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
QMAKE_LFLAGS += "-B /usr/bin"
}
```

The qmake call should add CONFIG+=openmp

```
qmake -r -spec macx-xcode CONFIG+=debug CONFIG+=enable-osmesa LLVM_PATH=/opt/llvm OSMESA_PATH=/opt/osmesa CONFIG+=openmp QMAKE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_CC=clang-mp-9.0 QMAKE_OBJECTIVE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-9.0 -stdlib=libc++' QMAKE_LD='clang++-mp-9.0 -stdlib=libc++' -r CONFIG+=openmp CONFIG+=enable-osmesa CONFIG+=enable-cairo
```


Then you can just build and run using Xcode

#### Xcode caveats

Whenever the .pro files change, Xcode will try to launch qmake and
probably fail because it doesn't find the necessary binaries (qmake,
moc, pkg-config, python3-config, etc.). In this case, just open a
Terminal and relaunch the above command. This will rebuild the Xcode projects.

Alternatively, you can globally add the necessary directories
(`/usr/local/bin`on Homebrew, `/opt/local/bin` on MacPorts) to you
PATH (see <http://www.emacswiki.org/emacs/EmacsApp> for instructions).

On MacPorts, this would look like:

```Shell
launchctl setenv PATH /opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin
```

## Testing

With CMake, the unit tests are built with Natron (`NATRON_BUILD_TESTS`, on by default) and run with `ctest` (see [Building with CMake](#building-with-cmake)).

With the deprecated qmake build, they build with the same qmake as Natron (see "Developing with the MacPorts SDK"), from `$NATRON_SRC`:

```Shell
cd $NATRON_SRC/Tests
# the top-level build (see "Developing with the MacPorts SDK") also builds Tests;
# the tests need OFX plug-ins (e.g. openfx-io) and an OCIO config:
env OFX_PLUGIN_PATH=$HOME/OFX/Natron-release OCIO=$NATRON_SRC/OpenColorIO-Configs/blender/config.ocio ./Tests
# for coverage, re-run the top-level qmake with the same flags plus CONFIG+=coverage
```

The render regression tests are in [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests); `launchBuildMain.sh` runs them with `UNIT_TESTS=true`.

## Generating Python bindings

This is not required: with Qt 5, the bindings are generated during the build, and the generated files for Qt 4 are in the repository. You only need this with Qt 4, if you extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken-2.7 for an explanation of the command line arguments.

Historical, for Qt 4 only (Natron 2.5 and older), with the Python 3.10 it was written for: Shiboken 1.2.4 is not part of the Python 3.11 SDK, and `tools/jenkins/include/scripts/pkg/python3.sh` notes that Python 3.11 breaks it. On MacPorts with qt4-mac, py310-pyside, py310-shiboken:
```Shell
PYVER=3.10 # Set to the python version
QT_VERSION_MAJOR=4
rm Engine/Qt${QT_VERSION_MAJOR}/NatronEngine/* Gui/Qt${QT_VERSION_MAJOR}/NatronGui/*

shiboken-${PYVER} --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:/opt/local/include:/opt/local/include/PySide-${PYVER}  --typesystem-paths=/opt/local/share/PySide-${PYVER}/typesystems --output-directory=Engine/Qt${QT_VERSION_MAJOR} Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken-${PYVER} --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:/opt/local/include:/opt/local/include/PySide-${PYVER}  --typesystem-paths=/opt/local/share/PySide-${PYVER}/typesystems:Engine:Shiboken --output-directory=Gui/Qt${QT_VERSION_MAJOR} Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh Engine/Qt${QT_VERSION_MAJOR}/NatronEngine natronengine
tools/utils/runPostShiboken.sh Gui/Qt${QT_VERSION_MAJOR}/NatronGui natrongui
```

Building Natron with Qt5 should generate the Python bindings automatically, but in case it does not,
here are the commands to recreate the Python bindings:

On MacPorts with qt5, py311-pyside2:
```Shell
PYVER=3.11 # Set to the python version
PYTHON_HOME=$(python${PYVER}-config --prefix)
QT_VERSION_MAJOR=5
# Fix a missing link in the MacPorts package
[ ! -f ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER} ] && sudo ln -s shiboken2 ${PYTHON_HOME}/lib/python${PYVER}/site-packages/shiboken2_generator/shiboken2-${PYVER}

rm Engine/Qt${QT_VERSION_MAJOR}/NatronEngine/* Gui/Qt${QT_VERSION_MAJOR}/NatronGui/*
# ${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include
shiboken2-${PYVER} --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Engine:Global:libs/OpenFX/include:/opt/local/include:/opt/local/libexec/qt${QT_VERSION_MAJOR}/include:${PYTHON_HOME}/include/python${PYVER}:${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include --typesystem-paths=${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/typesystems --output-directory=Engine/Qt${QT_VERSION_MAJOR} Engine/PySide2_Engine_Python.h  Engine/typesystem_engine.xml

shiboken2-${PYVER} --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Engine:Global:libs/OpenFX/include:/opt/local/include:/opt/local/libexec/qt${QT_VERSION_MAJOR}/include:/opt/local/libexec/qt${QT_VERSION_MAJOR}/include/QtWidgets:${PYTHON_HOME}/include/python${PYVER}:${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/include --typesystem-paths=${PYTHON_HOME}/lib/python${PYVER}/site-packages/PySide2/typesystems:Engine:Shiboken --output-directory=Gui/Qt${QT_VERSION_MAJOR} Gui/PySide2_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken2.sh Engine/Qt${QT_VERSION_MAJOR}/NatronEngine natronengine
tools/utils/runPostShiboken2.sh Gui/Qt${QT_VERSION_MAJOR}/NatronGui natrongui
```

on HomeBrew with Qt5/PySide2/Shiboken2:
```Shell
PYVER=3.10 # Set to the python version
export PATH="/usr/local/opt/pyside@2/bin:$PATH"
QT_VERSION_MAJOR=5
rm Engine/Qt${QT_VERSION_MAJOR}/NatronEngine/* Gui/Qt${QT_VERSION_MAJOR}/NatronGui/*

shiboken2 --enable-parent-ctor-heuristic --use-isnull-as-nb_nonzero --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Global:Engine:libs/OpenFX/include:/usr/local/Frameworks/Python.framework/Versions/${PYVER}/include/python${PYVER}:/usr/local/include:/usr/local/opt/pyside@2/include/PySide2:/usr/local/opt/qt@${QT_VERSION_MAJOR}/include  --typesystem-paths=/usr/local/opt/pyside@2/share/PySide2/typesystems --output-directory=Engine/Qt${QT_VERSION_MAJOR} Engine/PySide2_Engine_Python.h  Engine/typesystem_engine.xml

shiboken2 --enable-parent-ctor-heuristic --use-isnull-as-nb_nonzero --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Global:Engine:Gui:libs/OpenFX/include:/usr/local/Frameworks/Python.framework/Versions/${PYVER}/include/python${PYVER}:/usr/local/include:/usr/local/opt/pyside@2/include/PySide2:/usr/local/opt/qt@${QT_VERSION_MAJOR}/include:/usr/local/opt/qt@5/include/QtWidgets  --typesystem-paths=/usr/local/opt/pyside@2/share/PySide2/typesystems:Engine --output-directory=Gui/Qt${QT_VERSION_MAJOR} Gui/PySide2_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken2.sh Engine/Qt${QT_VERSION_MAJOR}/NatronEngine natronengine
tools/utils/runPostShiboken2.sh Gui/Qt${QT_VERSION_MAJOR}/NatronGui natrongui
```

**Note**
Shiboken has a few glitches which needs fixing with some sed commands, run tools/utils/runPostShiboken.sh once shiboken is called

## OpenFX plugins

With MacPorts, build the plugins as in "Developing with the MacPorts SDK" above, or as `tools/jenkins/build-plugins.sh` does. Each plugin repository's `README.md` has its own build instructions, and its CI workflows in `.github/workflows` show a complete build.


You can install [TuttleOFX](http://www.tuttleofx.org/) using Homebrew:

```Shell
brew tap homebrew/science homebrew/x11 homebrew/python cbenhagen/video
brew install tuttleofx
```


Or have a look at the [instructions for building on MacPorts as well as precompiled universal binaries](http://devernay.free.fr/hacks/openfx/#OSXTuttleOFX).
