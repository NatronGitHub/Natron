developer installation for linux
==================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on linux. 

##Install libraries

In order to have Natron compiling, first you need to install the required libraries.

###*Qt 4.8*

You'll need to install Qt libraries from [Qt download](http://qt-project.org/downloads).
Alternatively you can get it from apt-get (the package depends on your distribution).


###*boost*

You can download boost from 
(boost download)[http://www.boost.org/users/download/]
Alternatively you can install boost from apt-get. (The package depends on your distribution).
For now just boost serialization is required.

###*OpenFX*

Go under Natron and type

	git submodule update -i

###*Expat*
 (The package depends on your distribution).
	sudo apt-get install expat

###*GLEW*
 (The package depends on your distribution).
	sudo apt-get install libglew-dev

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
EOF
----- end -----

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

### OpenColorIO configs
Note that if you want Natron to find the OpenColorIO config files you will need to
place them in the appropriate location. In the repository they are located under
Gui/Resources/OpenColorIO-Configs
You must copy them to a directory named ../share/OpenColorIO-Configs relative to
Natron's binary.
