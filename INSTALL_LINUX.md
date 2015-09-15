Developer installation for GNU/Linux
====================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on GNU/Linux.

Note that that installing dependencies using `apt-get` should work on
any Debian-based distribution (e.g. Ubuntu).

The scripts `tools/travis/install_dependencies.sh` and
`tools/travis/build.sh` respectively install the correct dependencies
and build Natron and the standard set of plugins on Ubuntu
12.04. These scripts should always be up-to-date.

The end of this document gives instructions for a other Linux
distributions.

## Install libraries

In order to have Natron compiling, first you need to install the required libraries.

### Qt 4.8.6

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads).
Alternatively you can get it from apt-get (the package depends on your distribution).
Please download one of the version mentioned above, Natron is known to be buggy when
running with Qt 5.


### Boost

You can download boost from 
[boost download](http://www.boost.org/users/download/)
Alternatively you can install boost from apt-get. (The package depends on your distribution).
For now just boost serialization is required.

### OpenFX

Go under Natron and type

    git submodule update -i --recursive

### Expat

 (The package depends on your distribution).

    sudo apt-get install expat

### GLEW

 (The package depends on your distribution).

    sudo apt-get install libglew-dev
	
### Cairo 1.12

(The package depends on your distribution).
On Ubuntu 12.04 LTS the package can be added with the following ppa:

    sudo add-apt-repository -y ppa:xorg-edgers/ppa 
    sudo apt-get install libcairo2-dev
	

We're done here for libraries.

###Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/MrKepzie/OpenColorIO-Configs/archive/Natron-v2.0.tar.gz).
Place it at the root of Natron repository.

###Add the config.pri file

You have to define the locations of some required libraries.
This is done by creating a .pri file next to the Project.pro that will tell the .pro
where to find those libraries.
The only library to put in the config.pri file on unix systems is boost.
For all other libraries are found with PKGConfig.


- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
 Of course you need to provide valid paths that are valid on your system.

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

Note that the last line for cairo is only if the package for cairo in your distribution
is lower than version 1.12 (as it is on Ubuntu 12.04 LTS for example).
If your distribution provide already better packages

### Build

The <srcPath> must be absolute and <buildPath> must not be a subdir of <srcPath>

    mkdir <buildPath>
    cd <buildfolder>
    qmake -r <srcPath>/Project.pro
    make

If you want to build in DEBUG mode change the qmake call to this line:

    qmake -r CONFIG+=debug <srcPath>/Project.pro

Some debug options are available for developers of Natron and you can see them in the
global.pri file. To enable an option just add CONFIG+=<option> in the qmake call.

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

	
### OpenColorIO configs

Note that if you want Natron to find the OpenColorIO config files you will need to
place them in the appropriate location. In the repository they are located under
`Gui/Resources/OpenColorIO-Configs`.
You must copy them to a directory named `../share/OpenColorIO-Configs` relative to Natron's binary.

# Installing and building on other Linux distributions

## ArchLinux

    pacman -S qt4 cairo glew python expat boost pixman ffmpeg opencolorio openimageio wget git cmake gcc make libxslt pkg-config
    wget https://raw.githubusercontent.com/olear/natron-linux/master/include/misc/build-natron-on-archlinux.sh

## CentOS6

### Add devtools-2
```
wget http://people.centos.org/tru/devtools-2/devtools-2.repo -O /etc/yum.repos.d/devtools-2.repo
```

### Install dependencies
```
yum -y install libxslt-devel pango-devel librsvg2-devel libxml2-devel devtoolset-2-toolchain gcc-c++ kernel-devel libX*devel fontconfig-devel freetype-devel zlib-devel *GL*devel *xcb*devel xorg*devel libdrm-devel mesa*devel *glut*devel dbus-devel bzip2-devel glib2-devel bison flex expat-devel libtool-ltdl-devel git
```

### Download SDK
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
scl enable devtoolset-2 bash
export INSTALL_PATH=/opt/Natron-CY2015
export PKG_CONFIG_PATH=$INSTALL_PATH/lib/pkgconfig:$INSTALL_PATH/ffmpeg-gpl/lib/pkgconfig
export LD_LIBRARY_PATH=$INSTALL_PATH/lib:$INSTALL_PATH/ffmpeg-gpl/lib
export PATH=/usr/local/bin:$INSTALL_PATH/bin:$INSTALL_PATH/ffmpeg-gpl/bin:$PATH
export QTDIR=$INSTALL_PATH
export BOOST_ROOT=$INSTALL_PATH
export PYTHON_HOME=$INSTALL_PATH
export PYTHON_PATH=$INSTALL_PATH/lib/python2.7
export PYTHON_INCLUDE=$INSTALL_PATH/include/python2.7
export OPENJPEG_HOME=$INSTALL_PATH
export THIRD_PARTY_TOOLS_HOME=$INSTALL_PATH

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
