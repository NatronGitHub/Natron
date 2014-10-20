Developer installation on mac osx
=================================

This file is supposed to guide you step by step to have working (compiling) version of Natron on mac osx ( >= 10.6 ). 
You need an up to date macports version. Just download it and install it from : 
(Macports website)[http://www.macports.org]

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.

### *Qt 4.8*

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads).
Alternatively you can get it from macports (recommended):

	sudo port install qt4-mac

### *boost*

You can download boost from 
(boost download)[http://www.boost.org/users/download/]
or with macports:

	sudo port install boost

For now only boost serialization is required.


###*OpenFX*

In Natron's source tree's root type:

	git submodule update -i --recursive

###*Expat*

With macports:

	sudo port install expat	

###*GLEW*

With macports:

	sudo port install glew
	
###*Cairo 1.12*

With macports:

	sudo port install cairo


Alternatively you can use the macports version.
	
We're done here for libraries.


create the file /opt/local/lib/pkgconfig/glu.pc containing GLU
configuration, for example using the following comands:

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

###Add the config.pri file

You have to define the locations of the required libraries.
This is done by creating a .pri file that will tell the .pro where to find those libraries.
The only library to put in the config.pri file on unix systems is boost.
For all other libraries are found with PKGConfig.

- create the config.pri file next to the Project.pro file.

You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

INCLUDEPATH is the path to the include files

LIBS is the path to the libs

----- copy and paste the following in a terminal -----
cat > config.pri << EOF
  INCLUDEPATH += /opt/local/include
  LIBS += -L/opt/local/lib -lboost_serialization-mt
EOF
----- end -----


###Generate Python bindings

shiboken-2.7 Engine/Pyside_Engine_Python.h --include-paths=Engine:/opt/local/include --typesystem-paths=/opt/local/share/PySide-2.7/typesystems:Engine --output-directory=Engine Engine/typesystem_engine.xml 

###Build with Makefile
You can generate a makefile by typing

	qmake -r Project.pro

then type

	make
	
This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

	qmake -r CONFIG+=debug Project.pro

*You can also enable logging by adding CONFIG+=log
*You can also enable clang sanitizer by adding CONFIG+=sanitizer

### Build on Xcode

Follow the instruction of build but 
add -spec macx-xcode to the qmake call command:

	qmake -r -spec macx-xcode Project.pro
	
Then open the already provided Project-xcode.xcodeproj and compile the target "all"

* If using Xcode to compile, and it doesn't find the necessary
binaries (qmake, moc, pkg-config, just execute this line from a
terminal and log in/out of your session (see
http://www.emacswiki.org/emacs/EmacsApp for other options):

launchctl setenv PATH /opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin



###Testing

	(cd Tests && qmake -r CONFIG+=debug CONFIG+=coverage && make -j4 && ./Tests)

