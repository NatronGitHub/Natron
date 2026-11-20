Instructions for installing Natron from sources on Microsoft Windows
====================================================================

# Installing with MSYS2

[MSYS2](https://sourceforge.net/projects/msys2/) is a unix-like toolchain that can be used to build Windows applications on Windows 8.1/10/11. It provides everything required by Natron and the OFX plug-ins.

Natron is built with **CMake** in an MSYS2 **UCRT64** shell, with MSYS2's own packages: see [Build](#build). The qmake build is deprecated: it still works on RB-2.6, but will be removed in a future release.

The [SDK instructions](tools/MINGW-packages/README.md) set up the environment for the OFX plug-ins and the release installers, which need Natron's own MSYS2 packages (`tools/MINGW-packages`). That package repository is built in UCRT64, like Natron (#1197). The installer scripts (`tools/jenkins`, `build_installer.yml`) still assume MINGW64 and qmake, so they can't run until they move to UCRT64 too, step 5 of [#1133](https://github.com/NatronGitHub/Natron/issues/1133). You don't need any of this to build Natron itself.

## Maintaining the Windows build

MSYS2 is a rolling distribution, so the Windows build sometimes breaks with no change in Natron. [tools/MINGW-packages/README.md](tools/MINGW-packages/README.md) describes the known cases ("Toolchain drift") and how to rebuild and publish the Natron package repository ("The package repository"). In short:

- Qt5's qmake is now `qmake-qt5`. `tools/jenkins/common.sh` finds it (`qmake-qt5`, `qmake5`, then `qmake`), and `launchBuildMain.sh` stops early if it doesn't work or is the qmake of another Qt major version. To use another qmake, set `NATRON_QMAKE`; a `QMAKE` inherited from the environment is ignored, with a warning.
- Natron's `natron_ffmpeg-gpl2` (FFmpeg 9.0.2, GPLv2) uses MSYS2's `libx264` (`libx264-165.dll`), the same one MSYS2's `libheif` uses: there is only one x264. It is bundled in GPL builds only. COMMERCIAL builds use `natron_ffmpeg-lgpl` (the same FFmpeg without the GPL libraries) and bundle `natron_libheif-lgpl`'s `libheif.dll` (MSYS2's libheif without x264 and x265, #1177) instead of MSYS2's, and `build-Windows-installer.sh` fails if `tools/jenkins/check-commercial-bundle.sh` finds a GPL component in them (#1302).
- The installer bundles `libpyside2`/`libshiboken2` for Qt5 and `libpyside6`/`libshiboken6` for Qt6.
- The bundled Python standard library is listed in `tools/jenkins/zip-python-mingw.sh`. Modules removed from the stdlib (e.g. `lib2to3` and `msilib` in Python 3.13) are skipped, and modules that became packages (`pathlib` and `sysconfig` in 3.13, `string` in 3.14) must be listed there.
- Natron's package repository is built in MSYS2 UCRT64 and contains only what MSYS2 doesn't provide in the form Natron needs (see "Which packages Natron builds" in [tools/MINGW-packages/README.md](tools/MINGW-packages/README.md)). The Breakpad tools are no longer built: releases ship no crash reporter until #1183.
- Natron's package repository also has a `freetype` (2.15.0-1.1, MSYS2's 2.15.0-1 with the upstream fix 05f343a for rotated mirrored text, #1287) that replaces MSYS2's. pacman never takes it from `[natron]` by itself: `natron_repo_install_overrides` (`tools/MINGW-packages/natron_repo_common.sh`) installs it as long as it is newer than MSYS2's. Delete it once MSYS2 ships the fix (see "Packages that replace MSYS2's" in [tools/MINGW-packages/README.md](tools/MINGW-packages/README.md)).
- The installer workflow downloads `natron_package_repo-<version>.zip` (`<version>` from `tools/MINGW-packages/windows_pacman_repo_version.txt`) from the `windows-mingw-package-repo` release of the repository it runs from, then from NatronGitHub/Natron, and builds the packages locally if neither has it. After an MSYS2 toolchain change (e.g. the `__emutls_v.*` symbols removed when GCC moved to native TLS, or new sonames of libx265 or libbluray), every package must be rebuilt, the zip published, and `windows_pacman_repo_version.txt` bumped.

### OpenFX

Natron uses the OpenFX API, before building you should make sure it is up to date.

For that, go under Natron and type

```
git submodule update -i --recursive
```

### Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.1.tar.gz).
Place it at the root of Natron repository.

***note:*** *If it is name something like: `OpenColorIO-Configs-Natron-v2.0` rename it to `OpenColorIO-Configs`*

### config.pri

***note:*** *`config.pri` is only used by the deprecated qmake build. The CMake build doesn't need it, so you can skip this section.*

The `config.pri` is used to define the locations of the dependencies. It is probably the most
confusing part of the build process.

Create a `config.pri` file next to the `Project.pro` that will tell the .pro file
where to find those libraries.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

You can find more examples specific to distributions below.

INCLUDEPATH is the path to the include files

LIBS is the path to the libs

    ----- copy and paste the following in a terminal -----
    cat > config.pri << EOF
    boost: LIBS += -lboost_serialization
    expat: LIBS += -lexpat
    expat: PKGCONFIG -= expat
    cairo: PKGCONFIG -= cairo
    EOF
    ----- end -----

***note:*** *the last line for cairo is only necessary if the package for cairo in your distribution
is lower than version 1.12 (as it is on Ubuntu 12.04 LTS for example).*

### Nodes

Natron's nodes are contained in separate repositories. To use the default nodes, you must also build the following repositories:

    https://github.com/NatronGitHub/openfx-misc
    https://github.com/NatronGitHub/openfx-io


You'll find installation instructions in the README of both these repositories. Both openfx-misc and openfx-io have submodules as well.

Plugins must be installed in /usr/OFX/Plugins on Linux
Or in a directory named "Plugins" located in the parent directory where the binary lies, e.g.:


    bin/
        Natron
    Plugins/
        IO.ofx.bundle


## Build

Natron is built with CMake and Ninja, in an MSYS2 **UCRT64** shell. The Windows CI job (`.github/workflows/ci.yml`) installs these packages:

    pacman -S --needed git unzip \
      mingw-w64-ucrt-x86_64-cc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja \
      mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-wget \
      mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-cairo mingw-w64-ucrt-x86_64-fontconfig \
      mingw-w64-ucrt-x86_64-expat mingw-w64-ucrt-x86_64-python \
      mingw-w64-ucrt-x86_64-qt5-base mingw-w64-ucrt-x86_64-pyside2 mingw-w64-ucrt-x86_64-shiboken2 \
      mingw-w64-ucrt-x86_64-python-qtpy \
      mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-llvm-tools

(shiboken2 needs clang and llvm-tools. The release installer scripts don't work yet: see the SDK paragraph above and #1133.) Then, from the Natron source directory:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
    cmake --build build

Useful options (add them to the first `cmake` command): `-DCMAKE_BUILD_TYPE=Debug` or `Release`, `-DNATRON_QT6=ON` (Qt 6 and PySide6), `-DNATRON_BUILD_TESTS=OFF` (skip the unit tests), `-DNATRON_SYSTEM_LIBS=ON` (use system copies of the libraries in `libs/`).

To run the unit tests, which need the OpenFX IO plug-ins and an OpenColorIO config:

    (cd build && PYTHONLEGACYWINDOWSDLLLOADING=1 PYTHONHOME=$MINGW_PREFIX OFX_PLUGIN_PATH=/path/to/Plugins OCIO=$PWD/../OpenColorIO-Configs/blender/config.ocio ctest -V)

`PYTHONLEGACYWINDOWSDLLLOADING=1` makes MSYS2's Python find its DLLs through `PATH` again, which the tests need (#1147).

`cmake --install build --prefix <dir>` installs Natron; `-DNATRON_INSTALL_LAYOUT=PORTABLE` gives the self-contained layout that the release scripts will use once they move to CMake. See the Maintainer Guide (`Documentation/source/maintainers/building.rst`).

### Deprecated: building with qmake

*The qmake build is deprecated. It still works on RB-2.6, where the release scripts in `tools/jenkins` still use it, but it will be removed in a future release (first on RB-2.7, see [#1181](https://github.com/NatronGitHub/Natron/issues/1181)). It needs a [config.pri](#configpri) file.*

To build, go into the Natron directory and type:

    qmake -r
    make

If everything has been installed and configured correctly, it should build without errors.

If you want to build in DEBUG mode change the qmake call to this line:

    qmake -r CONFIG+=debug

Some debug options are available for developers of Natron and you can see them in the
global.pri file. To enable an option just add `CONFIG+=<option>` in the qmake call.

# Installing with Visual Studio

Building Natron with Visual Studio (MSVC) is not supported or tested: use MSYS2 as described above. The old Visual Studio instructions were removed, since they used Qt 4, Python 3.4, the deprecated qmake build, and the copy of expat in `libs/OpenFX/HostSupport`, which openfx is removing (openfx#143). Whether to add an MSVC build is discussed in [#1133](https://github.com/NatronGitHub/Natron/issues/1133); contributions are welcome.
