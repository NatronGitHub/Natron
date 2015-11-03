Developer installation for GNU/Linux
====================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on GNU/Linux.

1. [Libraries](#libraries)
    - [Qt4](#qt-486)
    - [Boost](#boost)
    - [Expat](#expat)
    - [Glew](#glew)
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
    - [CentOS](#centos6-64-bit)


# Libraries

***note:*** *The scripts `tools/travis/install_dependencies.sh` and
`tools/travis/build.sh` respectively install the correct dependencies
and build Natron and the standard set of plugins on Ubuntu
12.04. These scripts should always be up-to-date. You can use them as a reference*

## Install libraries

In order to have Natron compiling, first you need to install the required libraries.

### Qt 4.8.6

You'll need to install Qt4 libraries, usually you can get them from your package manager (depends on your distribution).

Alternatively you can download it directly from [Qt download](http://qt-project.org/downloads).

Please download `Qt 4.*`, Natron is known to be buggy when running with Qt 5.


### Boost

Natron requires `boost serialzation` to compile.
You can download boost with your package manager.
Alternatively you can install boost from [boost download](http://www.boost.org/users/download/)

### Expat

You can download it with your package manager.
The package depends on your distribution.



### GLEW

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
to make a tarball release and let you download it [here](https://github.com/MrKepzie/OpenColorIO-Configs/archive/Natron-v2.0.tar.gz).
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

    https://github.com/devernay/openfx-misc
    https://github.com/MrKepzie/openfx-io


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
sudo pacman -S qt4 glew expat boost python2-pyside python2-shiboken
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
boost: LIBS += -lboost_serialization
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

```
sudo apt-get install expat
sudo apt-get install libglew-dev
```

On Ubuntu 12.04 LTS the package can be added with the following ppa:
```
sudo add-apt-repository -y ppa:xorg-edgers/ppa
sudo apt-get install libcairo2-dev
```

For the config.pri use:

```
boost: LIBS += -lboost_serialization
expat: LIBS += -lexpat
expat: PKGCONFIG -= expat
cairo: PKGCONFIG -= cairo
```

## CentOS6+ (64-bit)

### Install dependencies
```
yum -y install gcc-c++ wget libX*devel *GL*devel *xcb*devel xorg*devel libdrm-devel mesa*devel *glut*devel dbus-devel bison flex expat-devel libtool-ltdl-devel git make glibc-devel glibc-devel.i686
```

### Download SDK (third-party software)
```
wget http://downloads.natron.fr/Third_Party_Binaries/Natron-CY2015-Linux-x86_64-SDK.tar.xz
tar xvf Natron-CY2015-Linux-x86_64-SDK.tar.xz -C /opt/
```

### Download Natron and plugins
```
git clone https://github.com/MrKepzie/Natron
git clone https://github.com/MrKepzie/openfx-io
git clone https://github.com/devernay/openfx-misc
git clone https://github.com/olear/openfx-arena
for i in $(echo "Natron openfx-io openfx-misc openfx-arena");do cd $i ; git submodule update -i --recursive ; cd .. ; done
```

### Build Natron and plugins
```
export INSTALL_PATH=/opt/Natron-CY2015
export PKG_CONFIG_PATH=$INSTALL_PATH/lib/pkgconfig:$INSTALL_PATH/ffmpeg-gpl/lib/pkgconfig
export LD_LIBRARY_PATH=$INSTALL_PATH/gcc/lib64:$INSTALL_PATH/lib:$INSTALL_PATH/ffmpeg-gpl/lib
export PATH=$INSTALL_PATH/gcc/bin:$INSTALL_PATH/bin:$INSTALL_PATH/ffmpeg-gpl/bin:$PATH
export QTDIR=$INSTALL_PATH
export BOOST_ROOT=$INSTALL_PATH
export PYTHON_HOME=$INSTALL_PATH
export PYTHON_PATH=$INSTALL_PATH/lib/python2.7
export PYTHON_INCLUDE=$INSTALL_PATH/include/python2.7

cd Natron
wget https://raw.githubusercontent.com/MrKepzie/Natron/workshop/tools/linux/include/natron/config.pri
mkdir build && cd build
$INSTALL_PATH/bin/qmake -r CONFIG+=release ../Project.pro
make || exit 1
cd ../..

cd openfx-io
CPPFLAGS="-I${INSTALL_PATH}/include -I${INSTALL_PATH}/ffmpeg-gpl/include" LDFLAGS="-L${INSTALL_PATH}/lib -L${INSTALL_PATH}/ffmpeg-gpl/lib" make CONFIG=release || exit 1
cd ..

cd openfx-misc
CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" make CONFIG=release || exit 1
cd ..

cd openfx-arena
CPPFLAGS="-I${INSTALL_PATH}/include" LDFLAGS="-L${INSTALL_PATH}/lib" make USE_SVG=1 USE_PANGO=1 STATIC=1 CONFIG=release || exit 1
cd ..
```
