Instructions for installing Natron from sources on GNU/Linux
============================================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on GNU/Linux.

1. [Libraries](#libraries)
    - [Qt4](#qt-486)
    - [Boost](#boost)
    - [Expat](#expat)
    - [Cairo](#cairo)
    - [Pyside](#pyside)
    - [Shiboken](#shiboken)
2. [Configuration](#configuration)
    - [OpenFX](#openfx)
    - [OpenColorIO-Configs](#download-opencolorio-configs)
    - [config.pri](#configpri)
    - [Nodes](#nodes)
3. [Build](#build)
4. [Distribution specific](#distribution-specific)
    - [Arch Linux](#arch-linux)
    - [Debian-based](#debian-based)
    - [CentOS7/Fedora](#centos7)


# Dependencies

The dependencies necessary to build and install Natron can either be built specifically for Natron, using the Natron SDK, or installed using packages from the Linux distribution.

## Installing the full Natron SDK

The Natron SDK is used for building the official Natron binaries. The script that builds the whole SDK and installs it in the default location (`/opt/Natron-sdk`, which must be user-writable) can be exectuted like this:

    cd tools/jenkins
	include/scripts/build-Linux-sdk.sh
	
It puts build logs and the list of files installed by each package in the directory `/opt/Natron-sdk/var/log/Natron-Linux-x86_64-SDK` or `/opt/Natron-sdk/var/log/Natron-Linux-i686-SDK`.

Some packages, especially Qt 4.8.7, have Natron-specific patches. Take a look at the SDK script to see which patches are applied to each packages, and what configuration options are used.

The SDK may be updated by pulling the last modifications to the script and re-executing it.


## Manually install dependencies

In order to have Natron compiling, first you need to install the required libraries.

***note:*** *The scripts `tools/travis/install_dependencies.sh` and
`tools/travis/build.sh` respectively install the correct dependencies
and build Natron and the standard set of plugins on Ubuntu
14.04. These scripts should always be up-to-date. You can use them as a reference*

### Qt 4.8.7

You'll need to install Qt4 libraries, usually you can get them from your package manager (depends on your distribution).

Alternatively you can download it directly from [Qt download](http://qt-project.org/downloads).

Please download `Qt 4.*`, Natron is known to be buggy when running with Qt 5.


### Boost

Natron requires `boost serialization` to compile.
You can download boost with your package manager.
Alternatively you can install boost from [boost download](http://www.boost.org/users/download/)

### Expat

You can download it with your package manager.
The package depends on your distribution.

### Cairo

Natron links statically to cairo. Sometimes you can find a static version in development packages.
If your distribution does not provide a static cairo package you will need to build it from source.

```
git clone git://anongit.freedesktop.org/git/cairo
cd cairo
./autogen.sh
make
sudo make install
```

### Pyside

Natron uses pyside for python 2  

### Shiboken

Natron uses shiboken for python 2


# Configuration

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

The `config.pri` is used to define the locations of the dependencies. It is probably the most
confusing part of the build process.

Create a `config.pri` file next to the `Project.pro` that will tell the .pro file
where to find those libraries.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

You can find more examples specific to distributions below.

INCLUDEPATH is the path to the include files

LIBS is the path to the libs
```
----- copy and paste the following in a terminal -----
cat > config.pri << EOF
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo
pyside: PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
pyside: PKGCONFIG += pyside
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
pyside: INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
EOF
----- end -----
```

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


# Build

To build, go into the Natron directory and type:

    qmake -r
    make

If everything has been installed and configured correctly, it should build without errors.
if you have both QT4 and QT5 installed qmake can generate errors, you can try

    QT_SELECT=4 qmake -r
    make

If you want to build in DEBUG mode change the qmake call to this line:

    qmake -r CONFIG+=debug

Some debug options are available for developers of Natron and you can see them in the
global.pri file. To enable an option just add `CONFIG+=<option>` in the qmake call.


# Distribution specific

## Arch Linux

On Arch Linux you can do the following:
```
sudo pacman -S qt4 expat boost python2-pyside python2-shiboken
```

Cairo has to be build from source, because Arch Linux does not provide a static version (as far as we know). It is fairly easy to do:

```
git clone git://anongit.freedesktop.org/git/cairo
cd cairo
./autogen.sh
make
sudo make install
```

It should be installed in `/usr/local/lib`

For the config.pri, use the following:

```pri
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo {
        # Building cairo from source (git clone, make, make install) is installed in /usr/local/lib
        PKGCONFIG -= cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
        LIBS += /usr/local/lib/libcairo.a
}
pyside {
        PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
        PKGCONFIG += pyside
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
}
shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken-py2)
        LIBS += -lshiboken-python2.7
}
```


## Debian-based

Installing dependencies using `apt-get` should work on
any Debian-based distribution (e.g. Ubuntu).

If your version of Ubuntu does not provide cairo 1.12 (required for rotoscoping), use the xorg-edger PPA:
```
sudo add-apt-repository -y ppa:xorg-edgers/ppa 
```
If your version of Ubuntu does not provide boost 1.49, the irie PPA can be used:
```
sudo add-apt-repository -y ppa:irie/boost 
```
Install the required packages:
```
sudo apt-get install libqt4-dev libboost-serialization-dev libboost-system-dev libexpat1-dev libcairo2-dev python-dev python-pyside libpyside-dev libshiboken-dev
```

For the config.pri use:

```
boost-serialization-lib: LIBS += -lboost_serialization
boost: LIBS += -lboost_thread -lboost_system
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo
```

for linux mint you will need to add:

```
pyside {
        PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
        PKGCONFIG += pyside
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(env PKG_CONFIG_PATH=$$PYSIDE_PKG_CONFIG_PATH pkg-config --variable=includedir pyside)/QtGui
}
```

## CentOS7

Instructions for CentOS and Fedora.

On CentOS you need the EPEL repository:

```
yum install epel-release
```

Install required packages:

```
yum install fontconfig-devel gcc-c++ expat-devel python-pyside-devel shiboken-devel qt-devel boost-devel pixman-devel cairo-devel
```

config.pri:
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

## Generating Python bindings

This is not required as generated files are already in the repository. You would need to run it if you were to extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken for an explanation of the command line arguments.

```Shell
SDK_PREFIX=/opt/Natron-sdk
PYSIDE_PREFIX=/opt/Natron-sdk/qt4
rm Engine/NatronEngine/* Gui/NatronGui/*

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide --typesystem-paths=$PYSIDE_PREFIX/share/PySide/typesystems --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide --typesystem-paths=$PYSIDE_PREFIX/share/PySide/typesystems:Engine --output-directory=Gui Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh
```

If using PySide2 for Qt5, the command-line would be:

```Shell
SDK_PREFIX=/opt/Natron-sdk
PYSIDE_PREFIX=/opt/Natron-sdk
rm Engine/NatronEngine/* Gui/NatronGui/*

shiboken2 --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide2 --typesystem-paths=$PYSIDE_PREFIX/lib/python2.7/site-packages/PySide2/typesystems --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken2 --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:$SDK_PREFIX/include:$PYSIDE_PREFIX/include/PySide2 --typesystem-paths=$PYSIDE_PREFIX/lib/python2.7/site-packages/PySide2/typesystems:Engine --output-directory=Gui Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh
```

**Note**
Shiboken has a few glitches which needs fixing with some sed commands, run tools/utils/runPostShiboken.sh once shiboken is called
