Instructions for installing Natron from sources on macOS
=======================================================

These are step-by-step instructions to compile Natron on macOS. This file is the reference for macOS installs. There are two ways to build:

- **In a VM, fully automated:** `tools/tart/README.md` builds the same MacPorts SDK in a clean macOS 15 (Sequoia, the distribution baseline) [tart](https://tart.run) VM, then builds the `.dmg` in a throw-away clone. Its provisioning script (`tools/tart/provision.sh`) runs the MacPorts steps below. tart only runs on **Apple Silicon Macs** and only produces arm64 builds; for Intel or older macOS versions, use the MacPorts steps below on an Intel Mac (see "Notes for older systems").
- **On your Mac:** follow the [MacPorts section](#macports-from-scratch) below, or use Homebrew for development builds.

The official macOS binaries are built with [MacPorts](https://www.macports.org) plus the custom ports in `tools/MacPorts`, using the `tools/jenkins/launchBuildMain.sh` script. That script does everything: checks out the sources, compiles the plugins and Natron, builds the `.dmg`, runs the tests and archives the result. The [MacPorts section](#macports-from-scratch) below describes how to prepare a Mac for it from scratch. On Apple Silicon it produces native arm64 binaries.

[Homebrew](https://brew.sh) can also be used for development builds (see [Homebrew](#homebrew)). It is not used for releases, because MacPorts is easier to customize.

The workflow around the release build is documented in the Maintainer Guide (`Documentation/source/maintainers/build-macos.rst`). That covers keeping MacPorts up to date, the overlay maintenance, signing, and the test suite.

## Checkout sources

The MacPorts overlay is part of the source tree, so check out the sources first:

```Shell
git clone https://github.com/NatronGitHub/Natron.git
cd Natron
git submodule update -i --recursive
```

In the rest of this document, `$NATRON_SRC` is the absolute path to this checkout. Set it in every shell you build from:

```Shell
export NATRON_SRC=$PWD   # in the Natron directory
```

## Install Dependencies

There are two exclusive options: MacPorts (used for official builds) or Homebrew.

### MacPorts from scratch

Run every step below from a terminal. Some `port install` steps take a long time (clang and Qt are built from source when no binary archive is available), so running them inside `caffeinate -i` (or `tmux`, installed in step 6) is a good idea.

If Homebrew is also installed, keep it out of your `PATH` while building with MacPorts. Otherwise build scripts may silently pick up Homebrew tools and hide missing MacPorts dependencies. These shell functions (bash/zsh, e.g. in `~/.bashrc`) switch between the two:

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
  [ -x /opt/local/bin/port ] && PATH="/opt/local/bin:/opt/local/sbin:$PATH"
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

Natron needs modified ports. Examples: the `+natron` variants of ImageMagick and openimageio and the `+natronmini` variant of ffmpeg; ffmpeg pinned to 8.1.2; libraw pinned to 0.18.13 with the GPL demosaic packs; libomp pinned to 11.1.0. MacPorts reads them from a copy of `tools/MacPorts` in `/opt/MacPorts-Natron`. A copy is used rather than the source tree itself, because MacPorts accesses the ports as an unprivileged user that usually can't read your home directory.

```Shell
sudo mkdir -p /opt/MacPorts-Natron
sudo chown $(id -un):staff /opt/MacPorts-Natron
cd $NATRON_SRC/tools/MacPorts
./sync.sh          # rsync to /opt/MacPorts-Natron and run portindex, which must report "Ports failed: 0"
```

Then register it **before** the default source. Add this line above the last line (`rsync://… [default]`) of `/opt/local/etc/macports/sources.conf`:

```
file:///opt/MacPorts-Natron
```

for example with:

```Shell
grep -q '^file:///opt/MacPorts-Natron' /opt/local/etc/macports/sources.conf ||
    sudo perl -0pi -e 's|^(rsync://\S+ \[default\])|file:///opt/MacPorts-Natron\n$1|m' /opt/local/etc/macports/sources.conf
```

Check it with `port file openimageio`, which should print a path under `/opt/MacPorts-Natron`. Re-run `./sync.sh` whenever the files in `tools/MacPorts` change.

If `./check.sh` (in the same directory) reports obsolete Portfiles after a `port selfupdate`, the overlay has to be reintegrated with upstream first. See `tools/MacPorts/skills/macports-natron-upgrade/SKILL.md`.

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

With recent macOS SDKs and Xcode versions (verified with Xcode 27.2), use an osmesa-install version that includes the `regex_impl-include-guard.patch` and `sancov-explicit-string.patch` LLVM patches, the HTTPS Mesa download URLs, and the Xcode ≥ 20 compiler detection fix. Older versions fail with `unknown type name 'regex_t'`, `chosen constructor is explicit`, FTP download errors, or `GLhandleARB` conversion errors in `glapi_dispatch.c`.

#### 5. Compiler

Ports and Natron are compiled with a MacPorts clang (rather than Apple's) for full OpenMP support. Install the version MacPorts uses by default on your OS:

| macOS | clang port |
|---|---|
| 26 and later | `clang-22` |
| 15 | `clang-18` |
| 10.7 to 14 | `clang-17` |
| 10.6 | `clang-11` |

```Shell
CLANG=22   # from the table above: 18 on macOS 15, 17 on 10.7 to 14
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
sudo ln -s . /opt/local/include/minizip-ng
sudo sed -i '' -e 's/^find_dependency(zstd/#find_dependency(zstd/' /opt/local/lib/cmake/minizip-ng/minizip-ng-config.cmake
sudo port -v -N install opencolorio  -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314
sudo port -v -N install opencolorio1 -quartz -python27 -python38 -python39 -python310 -python311 -python312 -python313 -python314

# openfx-misc
sudo port -v -N install seexpr seexpr211

# openfx-arena
sudo port -v -N install librsvg -quartz
sudo port -v -N install poppler librevenge libcdr-0.1 sox libzip
sudo sed -i '' -e 's/Zstd::Zstd/zstd/' /opt/local/lib/pkgconfig/libzip.pc
sudo port -v -N install ImageMagick +natron -x11

# openfx-gmic
sudo port -v -N install fftw-3 -gfortran fftw-3-single -gfortran

# openfx-io
sudo port -v -N install ffmpeg +gpl2 +highbitdepth +natronmini
sudo port -v -N install openimageio +freetype +natron +opencolorio +openjpeg +python310 -python312 -python313 -python314
```

#### 8. Python 3.10, Qt 5 and PySide2

```Shell
sudo port -v -N install py310-pip
sudo port -v -N install openblas
sudo port -v -N install py310-numpy -gfortran
sudo port -v -N install py310-sphinx py310-sphinx_rtd_theme
sudo port -v -N install qt5 qt5-qtwebengine
sudo port -v -N install py310-pyside2   # its shiboken2 builds against the clang/llvm-18 it depends on

sudo port select --set python python310
sudo port select --set python3 python310
sudo port select --set sphinx py310-sphinx
sudo pip-3.10 install qtpy==1.11.2

# links expected by the build scripts
PY=/opt/local/Library/Frameworks/Python.framework/Versions/3.10
sudo ln -sfn shiboken2 $PY/lib/python3.10/site-packages/shiboken2_generator/shiboken2-3.10
sudo ln -sfn shiboken2-3.10 /opt/local/bin/shiboken2
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

The `.dmg` ends up in `$HOME/Development/workspace/builds_archive/natron_github_RB2/1/`, named `Natron-<branch>-<date>-<commit>-macOS<version>-<arch>.dmg`. Use `UNIT_TESTS=true` to also run the render regression suite (Natron-Tests); this adds `-tests.txt` and `-unit_tests_failures.zip` next to the DMG. `BUILD_FROM=N` / `BUILD_TO=N` run only some of the six steps (checkout, plugins, Natron, installer, tests, archive).

For a release, build a tag instead of a branch:

```Shell
env NATRON_DEV_STATUS=STABLE RELEASE_TAG=2.6.0 NATRON_BUILD_NUMBER=1 NOUPDATE=1 WORKSPACE=$HOME/Development/workspace \
    DISABLE_BREAKPAD=1 NATRON_LICENSE=GPL GIT_URL=https://github.com/NatronGitHub/Natron.git \
    UNIT_TESTS=true BUILD_NAME=natron_github_RB2 BUILD_NUMBER=1 COMPILE_TYPE=release MKJOBS=8 \
    ./launchBuildMain.sh
```

Signing, notarization and the test suite are described in `Documentation/source/maintainers/build-macos.rst`.

#### 11. Keeping MacPorts up to date

```Shell
sudo port selfupdate
cd $NATRON_SRC/tools/MacPorts && ./check.sh   # overlay Portfiles whose upstream changed
```

If `check.sh` reports obsolete Portfiles, reintegrate upstream into the overlay first: see `tools/MacPorts/skills/macports-natron-upgrade/SKILL.md`. Some overlay ports are deliberately **pinned** to older versions, and their `Portfile` must be kept when reintegrating (only `Portfile.orig` follows upstream):

| port | pinned version | reason |
|---|---|---|
| `lang/libomp` | 11.1.0 | OpenMP regression affecting Natron |
| `graphics/libraw` | 0.18.13 | the GPL demosaic packs (`+gpl2`) only exist for 0.18 |
| `multimedia/ffmpeg` | 8.1.2 | FFmpeg 9 removed `AVCodec::pix_fmts`, which openfx-io uses |

Then publish the overlay and upgrade:

```Shell
./sync.sh
sudo port -v upgrade outdated
```

`upgrade outdated` can pull in new dependencies or variants. If it fails or picks up unwanted changes, upgrade port by port, or re-run the install steps above (see `Documentation/source/maintainers/build-macos.rst`).

MacPorts never downgrades a port. If a pinned port was installed in a newer version, uninstall it (`sudo port -f uninstall ffmpeg`) and install it again with the same variants. To reclaim disk space afterwards: `sudo port reclaim`, or delete the downloaded archives with `sudo rm -rf /opt/local/var/macports/software/*`.

#### Developing with the MacPorts SDK

`launchBuildMain.sh` builds everything from fresh checkouts. To work on Natron or the plugins directly, build them from your own checkouts.

The OpenFX plugins ([openfx-io](https://github.com/NatronGitHub/openfx-io), [openfx-misc](https://github.com/NatronGitHub/openfx-misc), [openfx-arena](https://github.com/NatronGitHub/openfx-arena), [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic)) build with `make` (`CONFIG=debug` or `release`, OpenMP with the MacPorts clang):

```Shell
mkdir -p ~/src && cd ~/src
for p in openfx-io openfx-misc openfx-arena openfx-gmic; do git clone --recursive https://github.com/NatronGitHub/$p.git; done
cd ~/src/openfx-misc
env SEEXPR_HOME=/opt/local OSMESA=1 OPENMP=1 CXX="clang++-mp-$CLANG -stdlib=libc++" \
    CXXFLAGS_EXTRA="-DGL_SILENCE_DEPRECATION -isysroot $(xcrun --sdk macosx --show-sdk-path) -std=c++14" \
    CONFIG=release make -j8
```

Bundles are written to `<Plugin>/Darwin-64-<config>/*.ofx.bundle`. To use them, symlink them into a directory and point `OFX_PLUGIN_PATH` at it (or use `/Library/OFX/Plugins`):

```Shell
mkdir -p ~/OFX/Natron-release && cd ~/OFX/Natron-release
ln -sf ~/src/openfx-io/IO/Darwin-64-release/IO.ofx.bundle ~/src/openfx-misc/*/Darwin-64-release/*.ofx.bundle .
```

Natron itself builds with qmake and the MacPorts Qt 5 (see [Building with qmake](#building-with-qmake-for-qt4-and-qt5) below for `config.pri`):

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
    App/Natron.app/Contents/MacOS/Natron
```

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

To install the [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic) set of plugin, you also need the following:

```Shell
brew install fftw
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
  versioned script (the port only installs `dmgbuild-3.10`):

  ```Shell
  sudo port -v -N install py310-dmgbuild
  sudo ln -s dmgbuild-3.10 /opt/local/bin/dmgbuild
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

## Building with qmake (for Qt4 and Qt5)

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

## Building with cmake (Qt5 only)

### On homebrew

```Shell
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/expat;/opt/homebrew/opt/qt@5;/opt/homebrew/opt/pyside@2" -DPYTHON_FRAMEWORK_LIBRARIES=/opt/homebrew/Frameworks/Python.framework/Versions/3.9/lib
make -j
```

## Testing

The unit tests build with the same qmake as Natron (see "Developing with the MacPorts SDK"), from `$NATRON_SRC`:

```Shell
cd $NATRON_SRC/Tests
/opt/local/libexec/qt5/bin/qmake -r CONFIG+=debug CONFIG+=coverage CLANG_MP_VERSION=$CLANG && make -j4 && ./Tests
```

The render regression tests are in [Natron-Tests](https://github.com/NatronGitHub/Natron-Tests); `launchBuildMain.sh` runs them with `UNIT_TESTS=true`.

## Generating Python bindings

This is not required: with Qt 5, the bindings are generated during the build, and the generated files for Qt 4 are in the repository. You only need this with Qt 4, if you extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken-2.7 for an explanation of the command line arguments.

On MacPorts with qt4-mac, py310-pyside, py310-shiboken:
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

On MacPorts with qt5, py310-pyside2:
```Shell
PYVER=3.10 # Set to the python version
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

Instructions to build the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugins can also be found in the [tools/packageOSX.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/packageOSX.sh) script if you are using MacPorts, or in the .travis.yml file in their respective github repositories if you are using Homebrew ([openfx-misc/.travis.yml](https://github.com/NatronGitHub/openfx-misc/blob/master/.travis.yml), [openfx-io/.travis.yml](https://github.com/NatronGitHub/openfx-io/blob/master/.travis.yml).


You can install [TuttleOFX](http://www.tuttleofx.org/) using Homebrew:

```Shell
brew tap homebrew/science homebrew/x11 homebrew/python cbenhagen/video
brew install tuttleofx
```


Or have a look at the [instructions for building on MacPorts as well as precompiled universal binaries](http://devernay.free.fr/hacks/openfx/#OSXTuttleOFX).
