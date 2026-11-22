Instructions for installing Natron from sources on GNU/Linux
============================================================

This file is supposed to guide you step by step to have working (compiling) version of Natron on GNU/Linux. Here's the gist of what you need to know:

* It's recommended to use Docker for the easiest hands-off installation method - see [here](#using-docker) for more details
* If you are on Arch Linux or Manjaro, see [this](#arch-linux) for relevant details
* If you are on Fedora or RHEL, see [here](#fedorarhel-based) for specific instructions
* If you are on Debian or a Debian-based system (such as Ubuntu, Linux Mint, KDE Neon, ElementaryOS etc.) see [here](#debian-based) for details
* If you are willing to try the complete installation process, the instructions are below
* Natron is built with **CMake** (see [Build](#build)). The qmake build is deprecated: it still works on RB-2.6, but will be removed in a future release (see [Deprecated: building with qmake](#deprecated-building-with-qmake))

0. [Using Docker](#using-docker)
1. [Dependencies](#dependencies)
  - [Installing the full Natron SDK](#installing-the-full-natron-sdk)
    - [Environment to use the Natron SDK](#environment-to-use-the-natron-sdk)
  - [Manually install dependencies](#manually-install-dependencies)
    - [Qt5](#qt-515)
    - [Qt4](#qt-48)
    - [Boost](#boost)
    - [Expat](#expat)
    - [Cairo](#cairo)
    - [Pyside2](#pyside2)
    - [Shiboken](#shiboken2)
    - [Pyside](#pyside)
    - [Shiboken](#shiboken)
    - [QtPy](#qtpy)
2. [Configuration](#configuration)
    - [OpenFX](#openfx)
    - [OpenColorIO-Configs](#download-opencolorio-configs)
    - [config.pri](#configpri) (qmake only, deprecated)
    - [Nodes](#nodes)
3. [Build](#build)
    - [Deprecated: building with qmake](#deprecated-building-with-qmake)
4. [Distribution specific](#distribution-specific)
    - [Arch Linux](#arch-linux)
    - [Debian-based](#debian-based)
    - [Fedora/RHEL-based](#fedorarhel-based)
5. [Generating Python bindings](#generating-python-bindings)

# Using Docker

If you have `docker` installed, the installation procedure is very simple. Simply create a directory called `builds`, and then run the following command:

```bash
docker run -it --rm --mount src="$(pwd)/builds",target=/home/builds_archive,type=bind natrongithub/natron-sdk:latest
```

Docker will automatically do the rest for you, and you should have a complete Natron binary in `./builds` (as a tgz archive).

# Dependencies

The dependencies necessary to build and install Natron can either be built specifically for Natron, using the Natron SDK, or installed using packages from the Linux distribution.

## Installing the full Natron SDK

The Natron SDK is used for building the official Natron binaries. The script that builds the whole SDK and installs it in the default location (`/opt/Natron-sdk`, which must be user-writable) can be exectuted like this:

```
cd tools/jenkins
include/scripts/build-Linux-sdk.sh
```

It puts build logs and the list of files installed by each package in the directory `/opt/Natron-sdk/var/log/Natron-Linux-<arch>-SDK`, where `<arch>` is `x86_64`, `aarch64` or `i686`.

The official Linux binaries are built with the Rocky Linux 9 SDK (`ROCKY=9`). It uses GCC 14 from `gcc-toolset-14`, Python 3.11, Qt 5.15.19, PySide2/Shiboken2 5.15.19 (built from source with EPEL's libclang 19), and librsvg 2.58 (built with Rust from `rust-toolset`). It doesn't build Qt 4 or the Qt installer framework (`WITH_QT4=false`): the Linux releases are portable archives. The Rocky Linux 8 SDK (`ROCKY=8`) is the same, except for libclang 18, librsvg 2.40 and Qt 4. Both build natively on x86_64 and aarch64: the Docker image is built by `[ROCKY=8] tools/docker/natron-sdk-rockylinux/build.sh [amd64|arm64]` (see `tools/docker/README.md`).

Some packages, especially Qt 4.8.7, have Natron-specific patches. Take a look at the SDK script to see which patches are applied to each packages, and what configuration options are used.

The SDK may be updated by pulling the last modifications to the script and re-executing it.

The release scripts (`tools/jenkins/launchBuildMain.sh`) build Natron with CMake, from the SDK; extra CMake options go in `NATRON_EXTRA_CMAKE_FLAGS`. On RB-2.6, `NATRON_BUILD_SYSTEM=qmake` selects the deprecated qmake build instead (see `Documentation/source/maintainers/building.rst`, "Release builds").

### Runtime requirements of the Linux binaries

The official Linux binaries are built on Rocky Linux 9 with the Natron SDK. They bundle the SDK libraries: Qt, Python, Boost, OpenImageIO, OpenColorIO, OpenEXR, FFmpeg, LibRaw, and so on. They never bundle the following system libraries, which must be installed on the system that runs Natron:

- glibc 2.34 or later: libc, libm, libdl, libpthread, librt and ld-linux.
- libstdc++ and libgcc_s from GCC 11 or later. On Rocky Linux 9 these are the system ones. Natron is compiled with a newer GCC, from `gcc-toolset`, but the libstdc++ symbols that are newer than GCC 11 are linked statically, from `libstdc++_nonshared.a`, so the binaries only need the system libstdc++.
- libgomp (OpenMP runtime), from GCC 11 or later. It is a separate package (`libgomp` on RHEL/Rocky, `libgomp1` on Debian/Ubuntu). Most systems have it, but minimal images such as `rockylinux:9-minimal` don't.
- libGL from GLVND (libGL, libEGL, libGLX, libGLdispatch, libOpenGL), and an OpenGL driver.
- The X11 client libraries, at least as recent as those of Rocky Linux 9.0 (libX11 1.7.0, libxcb 1.13.1, libxkbcommon 1.0.3): libX11, libX11-xcb, libXext, libXrender, libXau, libXdmcp and the other libX\* libraries, libxcb and its extensions, the xcb-util libraries (libxcb-icccm, libxcb-image, libxcb-keysyms, libxcb-render-util), libSM, libICE, libxkbcommon and libxkbcommon-x11. Qt's X11 platform plug-in (`libqxcb.so`) needs them.
- ALSA (libasound), for Qt Multimedia.

Other libraries are not required, but they are never bundled either: when the system has them, they are used from the system, because they must match its drivers or services. A headless render node doesn't need them:

- the rest of GLVND (libGLESv1_CM, libGLESv2), libdrm and libdrm\_\*, and Mesa's libgbm and libglapi: the system's OpenGL drivers use them, and an older bundled copy would break those drivers;
- systemd's libudev (it reads the running udev's database, whose format follows the system's udev) and libselinux;
- the GPU drivers' libraries: libcuda, NVIDIA's libnvidia-\* and libnvcuvid, and the OpenCL vendor drivers (ICDs: libamdocl\*, libigdrcl, libintelocl, libMesaOpenCL, libRusticlOpenCL, libpocl). The OpenCL ICD loader only opens the ICDs listed in `/etc/OpenCL/vendors/*.icd`, so a bundled copy would never be used. The ICD loader itself, libOpenCL.so.1 (ocl-icd or the Khronos OpenCL-ICD-Loader), is not in this list, since a default EL9 or Ubuntu install doesn't have it: it is bundled when a bundled file needs it (none does today).

These are the system libraries of openfx's `Support/include/linuxCheckBundle.sh`, which checks the OFX plug-in bundles, plus the X11 and ALSA libraries.

Everything else comes from the SDK and is bundled. In particular, Python's `crypt` module (deprecated since Python 3.11) is not included: it needs libcrypt.so.2, which Debian and Ubuntu don't have. libva (VA-API) is neither bundled nor needed: the SDK image has no libva-devel, so the SDK's FFmpeg is built without VA-API (and `checkBundle` would fail if a bundled file needed the system's libva). A bundled libva would only find the VA-API drivers at the paths of the distribution it was built on.

`tools/jenkins/build-Linux-installer.sh` checks this (`checkBundle`, on the portable tree and on the installer packages). It fails if:
- the directory doesn't exist or contains no ELF file;
- one of the system libraries above is bundled;
- an ELF file needs (DT_NEEDED) a library that is neither bundled nor one of the system libraries above;
- a bundled binary needs a symbol version of a system library that is newer than Rocky Linux 9.0 provides, as found by `objdump -p` (Version References): GLIBC_2.34, GLIBCXX_3.4.29, CXXABI_1.3.13, GCC_7.0.0 on x86_64 or GCC_11.0 on aarch64, GOMP_5.0.1 and OMP_5.0.1. Later EL9 updates of glibc define a few GLIBC_2.35 symbols, backported from glibc 2.35: a binary that needs them would not run on older EL9 systems;
- a bundled binary needs a version of these libraries without a number, such as GLIBC_PRIVATE or GLIBC_ABI_DT_RELR (glibc 2.36 and later), except libstdc++'s CXXABI_TM_1 and CXXABI_FLOAT128.

It then runs openfx's `Support/include/linuxCheckBundle.sh` (from `libs/OpenFX`, if it has it) on the OFX plug-in bundles of the portable tree, which also checks their rpaths.

### Environment to use the Natron SDK

Once the SDK is built, you should set your environment in the shell from which you execute or test Natron, to make sure that the Natron SDK is preferred over any other system library:

```
. path_to_Natron_sources/tools/utils/natron-sdk-setup-linux.sh
```

This must be done in every shell/terminal where you intend to compile and/or run Natron.

## Installing dependencies from Ubuntu

The Linux job of the [GitHub Actions CI](https://github.com/NatronGitHub/Natron/actions/workflows/ci.yml)
(`.github/workflows/ci.yml`) installs the dependencies and builds Natron and the tests on Ubuntu 24.04:
read it for the exact package list and build commands.
You can use it as a reference, but the resulting binaries are not guaranteed to be fully functional.

## Installing dependencies manually

### Qt 5.15

For Qt5 You'll need to install the qtbase libraries, usually you can get them from your package manager (which depends on your Linux distribution).

Alternatively you can build it from source using the tarballs from [Qt download](https://download.qt.io/archive/qt/5.15/5.15.4/submodules) or the [KDE fork](https://invent.kde.org/qt/qt/qtbase/-/tree/kde/5.15).

### Qt 4.8

In case you prefer to use Qt4 you'll have to build it from source as many distributions have already deprecated Qt 4, [Qt download](https://download.qt.io/archive/qt/4.8/4.8.7/) has a source archive.

### Boost

Natron requires `boost serialization` to compile.
You can download boost with your package manager.
Alternatively you can install boost from [boost download](http://www.boost.org/users/download/)

### Expat

You can download it with your package manager.
The package depends on your distribution.

### Cairo

You can download it with your package manager.

### PySide2

Natron uses pyside2 for Python 3 with Qt5.

### Shiboken2

Natron uses shiboken2 for Python 3 with Qt5, the generator binary (`shiboken2`) and headers are required too.

### PySide

Natron uses pyside for Python 2 with Qt4, notice that it's deprecated in many distros.

### Shiboken

Natron uses shiboken for Python 2 with Qt4, notice that it's deprecated in many distros.

### QtPy

Abstraction layer for PyQt5/PyQt4/PySide2/PySide. Preferably you can use your distribution package manager or install it via pip

```
pip install qtpy
```

# Configuration

### OpenFX

Natron uses the OpenFX API, before building you should make sure its submodule is up to date.

For that, go under Natron and type

```
git submodule update --init --recursive
```

### Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz).
Place it at the root of Natron repository.

***note:*** *If it is named something like: `OpenColorIO-Configs-Natron-v2.5` rename it to `OpenColorIO-Configs`*


```
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar -xvzf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

***note:*** In order to reclaim disk space, you may keep only the following subfolders : blender\*, natron, nuke-default

```
cd OpenColorIO-Configs && rm -v !("blender"|"blender-cycles"|"natron"|"nuke-default") -R
```

### config.pri

***note:*** *`config.pri` is only used by the deprecated qmake build. The CMake build finds the dependencies itself (with pkg-config and the CMake packages of Qt, Shiboken2 and PySide2), so you can skip this section.*

The `config.pri` is used to define the locations of the dependencies. It is probably the most
confusing part of the build process.

Create a `config.pri` file next to the `Project.pro` that will tell the .pro file
where to find those libraries.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

You can find more examples specific to distributions below.

`INCLUDEPATH` is the path to the include files.

`LIBS` is the path to the libs.

`PKGCONFIG` is the pkg-config.

An example configuration for a Qt4 build might be

```
----- copy and paste the following in a terminal -----
cat > config.pri << EOF
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
pyside: PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
pyside: PKGCONFIG += pyside
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
EOF
----- end -----
```

### Nodes

Natron's nodes are contained in separate repositories. To use the default nodes, you must also build the following repositories:

- [NatronGitHub/openfx-misc](https://github.com/NatronGitHub/openfx-misc)
- [NatronGitHub/openfx-io](https://github.com/NatronGitHub/openfx-io)


You'll find installation instructions in the README of both these repositories. Both openfx-misc and openfx-io have submodules as well.

Plugins can be installed in /usr/OFX/Plugins on Linux
Or in a directory named "Plugins" located in the parent directory where the binary lies, e.g.:

```
bin/
    Natron
Plugins/
    IO.ofx.bundle
```

# Build

Natron is built with CMake (3.21 or later). The distribution package is recent enough on Ubuntu 22.04 and later, Debian 12 and later, Fedora, and RHEL/Rocky/Alma 8.8 and later and 9 (`dnf install cmake`). On older systems, install a newer CMake with `pip install cmake` (in a virtual environment or with `--user`), or from https://cmake.org/download/ (or Kitware's APT repository on Ubuntu).

From the Natron source directory:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

This is what the Linux CI job does on Ubuntu 24.04 (see `.github/workflows/ci.yml`); see [Debian-based](#debian-based) for the packages it installs.

Useful options (add them to the first `cmake` command):

- `-DCMAKE_BUILD_TYPE=Debug`, `Release` or `RelWithDebInfo` (the default).
- `-DNATRON_QT6=ON` builds with Qt 6 and PySide6 instead of Qt 5 and PySide2, as C++20 (Qt 5 builds are C++17). See [Debian-based](#debian-based) and [Arch Linux](#arch-linux) for the Qt 6 packages.
- `-DNATRON_BUILD_TESTS=OFF` skips the unit tests.
- `-DNATRON_SYSTEM_LIBS=ON` uses the system copies of the libraries that Natron otherwise builds from `libs/`.
- `-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang` builds with Clang.

To run the unit tests, which need the OpenFX IO plug-ins and an OpenColorIO config:

```
(cd build && OFX_PLUGIN_PATH=/path/to/Plugins OCIO=$PWD/../OpenColorIO-Configs/blender/config.ocio ctest -V)
```

The tests open windows: on a machine without a display, run them under `xvfb-run` (package `xvfb`), as the CI job does: `xvfb-run --auto-servernum ctest -V`.

To install, run `cmake --install build --prefix /usr/local` (or another prefix). The default layout is the usual Unix one (`bin/`, `share/`); `-DNATRON_INSTALL_LAYOUT=PORTABLE` gives the self-contained layout that the release scripts will use once they move to CMake. See the Maintainer Guide (`Documentation/source/maintainers/building.rst`) for details.

## Deprecated: building with qmake

***note:*** *The qmake build is deprecated. It still works on RB-2.6, where the release scripts in `tools/jenkins` use it with `NATRON_BUILD_SYSTEM=qmake` (they build with CMake by default), but it will be removed in a future release (first on RB-2.7, see [#1181](https://github.com/NatronGitHub/Natron/issues/1181)). It needs a [config.pri](#configpri) file.*

To build, go into the Natron directory and type:

```
qmake -r
make
```

If everything has been installed and configured correctly, it should build without errors.
In case you have many versions of Qt installed qmake can generate errors, you can try for a Qt5 build

```
QT_SELECT=5 qmake -r
make
```

If you want to build in DEBUG mode change the qmake call to this line:

```
qmake -r CONFIG+=debug
```

In case for compiling with Clang:

```
qmake -r -spec linux-clang
```

Some debug options are available for developers of Natron and you can see them in the
`global.pri` file. To enable an option just add `CONFIG+=<option>` in the `qmake` call.


# Distribution specific

## Arch Linux

On Arch Linux, there are two tested methods of compiling Natron: using the AUR or via manual compiling.

### If using AUR

Simply run the command below:

```
yay -S natron-compositor
```

### If compiling manually

First, install build dependencies. You can install GCC, Expat and Boost directly from the Arch Linux official repositories, like so:

```
sudo pacman -S expat boost-libs gcc
```

You will also need additional Boost libraries, cairo, and Qt5 (provided by PySide2). They can be installed with the following command:

```
sudo pacman -S cmake boost cairo pyside2 python-pyqt
```

For a Qt 6 build (`-DNATRON_QT6=ON`), install `qt6-base pyside6 shiboken6 python-qtpy` instead of `pyside2 python-pyqt`.

Then, clone Natron's repo:

```
git clone https://github.com/NatronGitHub/Natron && cd Natron
```

Update submodules:

```
git submodule init
git submodule update --init --recursive
```

Build it with CMake, as described in [Build](#build):

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(nproc)
```

The rest of this section is for the deprecated qmake build. Make a build folder:

```
mkdir build && cd build
```

At this point, you might need the `config.pri` in case you have to pass some options to the build via the config file. On every operating system and distro this will be different, including for Arch Linux. First, make it by running this command:

```
touch ../config.pri
```

Now, open `../config.pri` with any editor and modify it to your preference. For example you can pass options like this:

```
CONFIG += custombuild
CONFIG += openmp
DEFINES += QT_NO_DEBUG_OUTPUT
```

In case of a Qt4 you should paste in these lines to the empty file. **A template `config.pri` is available [here](./build-configs/arch-linux/config.pri)**. Here are some recommended instructions to do so:

```
# These are the lines you should paste into your empty `config.pri`
boost: LIBS += -lboost_serialization
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo {
        PKGCONFIG += cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
}
pyside {
        PKGCONFIG -= pyside
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py2)
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py2)/QtCore
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside-py2)/QtGui
        INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
        LIBS += -lpyside-python2.7
}
shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken-py2)
        LIBS += -lshiboken-python2.7
}
```

You're now all set to compile. Use `qmake` to generate a Makefile for final compiling, like this:

```
qmake -r ../Project.pro PREFIX=/usr BUILD_USER_NAME="Arch Linux" CONFIG+=custombuild CONFIG+=openmp DEFINES+=QT_NO_DEBUG_OUTPUT QMAKE_CFLAGS_RELEASE="${CFLAGS}" QMAKE_CXXFLAGS_RELEASE="${CXXFLAGS}" QMAKE_LFLAGS_RELEASE="${LDFLAGS}"

# or in case you want to use Clang

qmake -r ../Project.pro -spec linux-clang PREFIX=/usr BUILD_USER_NAME="Arch Linux" CONFIG+=custombuild CONFIG+=openmp DEFINES+=QT_NO_DEBUG_OUTPUT QMAKE_CFLAGS_RELEASE="${CFLAGS}" QMAKE_CXXFLAGS_RELEASE="${CXXFLAGS}" QMAKE_LFLAGS_RELEASE="${LDFLAGS}"
```

Last, compile with `make`:

```
make
```

The binaries will be found in the `build/App` folder. In order to launch Natron after compiling, simply do `./App/Natron`, and you can then start using Natron!


## Debian-based

Installing dependencies using `apt-get` or `apt` should work on
any Debian-based distribution.

For Ubuntu 24.04 (Python 3.12, Qt 5.15), these are the packages that the Linux CI job installs (`.github/workflows/ci.yml`), plus `cmake`, which the GitHub runner already has:

```
sudo apt install build-essential cmake libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev \
  qt5-qmake qtbase5-dev qtdeclarative5-dev \
  python3-dev libshiboken2-dev libpyside2-dev python3-pyside2.qtwidgets python3-qtpy \
  libwayland-dev libwayland-client0 libwayland-egl1 \
  extra-cmake-modules clang
```

(`qt5-qmake` is only needed for the deprecated qmake build.)

For a Qt 6 build (`-DNATRON_QT6=ON`), replace the Qt 5 and PySide2 packages with the Qt 6 and PySide6 ones. Ubuntu 24.04 has no PySide6/Shiboken6 packages: they start with Ubuntu 26.04. On Ubuntu 26.04 (no `libboost-system-dev` there: Boost.System is header-only since Boost 1.89):

```
sudo apt install build-essential cmake libboost-serialization-dev libexpat1-dev libcairo2-dev \
  qt6-base-dev \
  python3-dev libshiboken6-dev shiboken6 libpyside6-dev python3-pyside6.qtwidgets python3-qtpy \
  libwayland-dev libwayland-client0 libwayland-egl1 \
  extra-cmake-modules clang
```

For Ubuntu 22.04 using Python 3.10 and Qt 5.15, install the required dependencies:

```
sudo apt install build-essential cmake libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev qt5-qmake qtbase5-dev python3-dev libshiboken2-dev libpyside2-dev python3-pyside2.qtwidgets python3-qtpy
```

For Debian 12, install the following packages instead:

```
sudo apt install build-essential cmake qtbase5-dev libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev python3-dev python3-pyside2.qtcore libpyside2-dev libshiboken2-dev
```

For most Debian/Ubuntu-based systems, install the required packages:

```
sudo apt install build-essential cmake qtbase5-dev libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev python3-dev python3-pyside2 libpyside2-dev libshiboken2-dev
```

For the Qt4 config.pri (deprecated qmake build) use:

```
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo

pyside: PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
pyside: PKGCONFIG += pyside
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
```

for Linux Mint you will need to add:

```
pyside {
        PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
        PKGCONFIG += pyside
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
}
```

Get Natron:

```
git clone https://github.com/NatronGitHub/Natron && cd Natron
git submodule update --init --recursive
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz
tar xzf Natron-v2.5.tar.gz
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```

Build:

```
mkdir ../build-Natron && cd ../build-Natron
cmake ../Natron
make -j8
make test
```

## Fedora/RHEL-based

Instructions for Fedora, Red Hat Enterprise Linux and derivatives. You can use either the dnf or yum package managers

On RHEL and derivative distributions you need the EPEL repository:

``` yum install epel-release ``` or ``` dnf install epel-release ```

Install required packages:

```
yum install cmake fontconfig-devel gcc-c++ expat-devel python-pyside2-devel shiboken2-devel qt5-qtbase-devel boost-devel pixman-devel cairo-devel
```
or
```
dnf install cmake fontconfig-devel gcc-c++ expat-devel python-pyside2-devel shiboken2-devel qt5-qtbase-devel boost-devel pixman-devel cairo-devel
```

Then build with CMake, as described in [Build](#build). For the deprecated qmake build, here is a Qt4 config.pri:
```pri
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
PKGCONFIG += expat
PKGCONFIG += fontconfig
cairo {
        PKGCONFIG += cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
}
pyside {
        PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
        PKGCONFIG += pyside
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
}
shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)
        LIBS += -lshiboken-python2.7
}
```

# Generating Python bindings

This is not required as file generation occurs during build with Qt5 and generated files are already in the repository for Qt4. You would need to run it if you were both under Qt4 and either extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken for an explanation of the command line arguments.

If using PySide for Qt4, the command-line would be:

```sh
SDK_PREFIX=/usr # /opt/Natron-sdk if using the Natron SDK
PYSIDE_PREFIX=/usr # /opt/Natron-sdk/qt4 if using the Natron SDK
QT=4
rm Engine/NatronEngine/* Gui/NatronGui/*

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide --typesystem-paths=$PYSIDE_PREFIX/share/PySide/typesystems --output-directory=Engine/Qt${QT} Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide --typesystem-paths=$PYSIDE_PREFIX/share/PySide/typesystems:Engine:Shiboken --output-directory=Gui/Qt${QT} Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh Engine/Qt${QT}/NatronEngine natronengine
tools/utils/runPostShiboken.sh Gui/Qt${QT}/NatronGui natrongui
```

**Note**
Shiboken has a few glitches which needs fixing with some sed commands, run `tools/utils/runPostShiboken.sh` for Qt4 once shiboken is called.
