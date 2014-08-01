developer installation for linux
==================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on linux. 

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.

###*Qt 4.8.6 or 5.3*

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads).
Alternatively you can get it from apt-get (the package depends on your distribution).
Please download one of the version mentioned above, Natron is known to be buggy when
running with Qt 5.0 or 5.1.


###*boost*

You can download boost from 
(boost download)[http://www.boost.org/users/download/]
Alternatively you can install boost from apt-get. (The package depends on your distribution).
For now just boost serialization is required.

###*OpenFX*

Go under Natron and type

	git submodule update -i --recursive

###*Expat*
 (The package depends on your distribution).
	sudo apt-get install expat

###*GLEW*
 (The package depends on your distribution).
	sudo apt-get install libglew-dev
	
###*Cairo 1.12*
(The package depends on your distribution).
On Ubuntu 12.04 LTS the package can be added with the following ppa:
sudo add-apt-repository -y ppa:xorg-edgers/ppa 
 	sudo apt-get install libcairo2-dev
	

We're done here for libraries.



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

### Build:

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
Gui/Resources/OpenColorIO-Configs
You must copy them to a directory named

	 ../share/OpenColorIO-Configs
	 
 relative to Natron's binary.
