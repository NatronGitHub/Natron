Developer installation on mac osx
=================================

This file is supposed to guide you step by step to have working (compiling) version of Natron on mac osx ( >= 10.6 ). 

## Checkout sources

	git clone https://github.com/MrKepzie/Natron.git
	cd Natron

If you want to compile the bleeding edge version, use the workshop
branch:

	git checkout workshop
	
Update the submodules:

	git submodule update -i --recursive 

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.

There are two exclusive options: using MacPorts or using Homebrew.

Homebrew is easier to set up than MacPorts, but cannot build universal binaries.

### MacPorts

You need an up to date macports version. Just download it and install it from <http://www.macports.org>, and execute the following commands in a terminal:

	sudo port selfupdate
	sudo port upgrade outdated
	sudo port install qt4-mac boost glew cairo expat

create the file /opt/local/lib/pkgconfig/glu.pc containing GLU
configuration, for example using the following comands:

```Shell
sudo -s
cat > /opt/local/lib/pkgconfig/glu.pc << EOF
 prefix=/usr
 exec_prefix=${prefix}
 libdir=${exec_prefix}/lib
 includedir=${prefix}/include


Name: glu
 Version: 2.0
 Description: glu
 Requires:
 Libs:
 Cflags: -I${includedir}
EOF
```

### Homebrew

Install homebrew from <http://brew.sh/>

Install libraries:

    brew tap homebrew/python
    brew tap homebrew/science
    brew install qt expat cairo glew

To install the openfx-io and openfx-misc sets of plugin, you also need the following:

    brew install ilmbase openexr freetype fontconfig ffmpeg opencolorio openimageio

also set the correct value for the pkg-config path (you can also put
this in your .bash_profile):
	
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig
	
## Add the config.pri file

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
cat > config.pri << EOF
boost: INCLUDEPATH += /opt/local/include
boost: LIBS += -L/opt/local/lib -lboost_serialization-mt
EOF
```

If you installed libraries using Homebrew, use the following
config.pri:

```Shell
 # copy and paste the following in a terminal
cat > config.pri << EOF
boost: INCLUDEPATH += /usr/local/include
boost: LIBS += -L/usr/local/lib -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt
expat: PKGCONFIG -= expat
expat: INCLUDEPATH += /usr/local/opt/expat/include
expat: LIBS += -L/usr/local/opt/expat/lib -lexpat
shiboken: PKGCONFIG -= shiboken
shiboken: INCLUDEPATH += /opt/local/include/shiboken-3.4
shiboken: LIBS += -L/opt/local/lib -lshiboken.cpython-34m
EOF
```

## Build with Makefile

You can generate a makefile by typing

	qmake -r Project.pro

then type

	make
	
This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

	qmake -r CONFIG+=debug Project.pro

* You can also enable logging by adding CONFIG+=log

* You can also enable clang sanitizer by adding CONFIG+=sanitizer

## Build on Xcode

Follow the instruction of build but 
add -spec macx-xcode to the qmake call command:

	qmake -r -spec macx-xcode Project.pro
	
Then open the already provided Project-xcode.xcodeproj and compile the target "all"

* If using Xcode to compile, and it doesn't find the necessary
binaries (qmake, moc, pkg-config, just execute this line from a
terminal and log in/out of your session (see
<http://www.emacswiki.org/emacs/EmacsApp> for other options):

launchctl setenv PATH /opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin

## Testing

	(cd Tests && qmake -r CONFIG+=debug CONFIG+=coverage && make -j4 && ./Tests)

## Generating Python bindings

This is not required as generated files are already in the repository. You would need to run it if you were to extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken-3.4 for an explanation of the command line arguments.

Alex, can you check if the homebrew line below this one works?

```Shell
shiboken-3.4 --include-paths=../Engine:../Global:/opt/local/include:/opt/local/include/PySide-3.4  --typesystem-paths=/opt/local/share/PySide-3.4/typesystems --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml
```

on HomeBrew:
```Shell
shiboken --include-paths=../Global:`pkg-config --variable=prefix QtCore`/include:`pkg-config --variable=includedir pyside`  --typesystem-paths=`pkg-config --variable=typesystemdir pyside` --output-directory=Engine Engine/Pyside_Engine_Python.h Engine/typesystem_engine.xml
 ```
 
## OpenFX plugins

Instructions to build the [openfx-io](https://github.com/MrKepzie/openfx-io) and [openfx-misc](https://github.com/devernay/openfx-misc) sets of plugins can also be found in the [tools/packageOSX.sh](https://github.com/MrKepzie/Natron/blob/workshop/tools/packageOSX.sh) script if you are using MacPorts, or in the .travis.yml file in their respective github repositories if you are using homebrew ([openfx-misc/.travis.yml](https://github.com/devernay/openfx-misc/blob/master/.travis.yml), [openfx-io/.travis.yml](https://github.com/MrKepzie/openfx-io/blob/master/.travis.yml).

Compiling [TuttleOFX](https://github.com/tuttleofx/TuttleOFX) is a bit trickier, but you can find [instructions for building on MacPorts as well as precompiled universal binaries](http://devernay.free.fr/hacks/openfx/#OSXTuttleOFX). Building on homebrew is also possible by following these two scripts: [install_dependencies.sh](https://github.com/tuttleofx/TuttleOFX/blob/develop/tools/travis/install_dependencies.sh) and [build.sh](https://github.com/tuttleofx/TuttleOFX/blob/develop/tools/travis/build.sh).
