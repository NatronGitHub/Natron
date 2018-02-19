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


# Libraries

***note:*** *The scripts `tools/travis/install_dependencies.sh` and
`tools/travis/build.sh` respectively install the correct dependencies
and build Natron and the standard set of plugins on Ubuntu
12.04. These scripts should always be up-to-date. You can use them as a reference*

## Install libraries

In order to have Natron compiling, first you need to install the required libraries.

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
PYSIDE_PKG_CONFIG_PATH = $$system($$PYTHON_CONFIG --prefix)/lib/pkgconfig:$$(PKG_CONFIG_PATH)
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
Or in a directory named "Plugins" located in the parent directory where the binary lies, e.g:


    bin/
        Natron
    Plugins/
        IO.ofx.bundle


# Build

To build, go into the Natron directory and type:

    qmake -r
    make

If everything has been installed and configured correctly, it should build without errors.

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
        PKGCONFIG -= pyside
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtCore
        INCLUDEPATH += $$system(pkg-config --variable=includedir pyside)/QtGui
        INCLUDEPATH += $$system(pkg-config --variable=includedir QtGui)
        LIBS += -lpyside-python2.7
}
shiboken {
        PKGCONFIG -= shiboken
        INCLUDEPATH += $$system(pkg-config --variable=includedir shiboken)
        LIBS += -lshiboken-python2.7
}
```

