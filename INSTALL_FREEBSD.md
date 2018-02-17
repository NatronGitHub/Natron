Instructions for installing Natron from sources on FreeBSD
==========================================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on FreeBSD. 

## Install libraries

In order to have Natron compiling, first you need to install the required libraries.

```
pkg install qt4 boost-all pyside-py27 expat cairo pkgconf
```

### Submodules

Go under Natron and type

    git submodule update -i --recursive


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

```pri
boost: LIBS += -lboost_serialization
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

###Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.1.tar.gz).
Place it at the root of Natron repository.

### Build

The <srcPath> must be absolute and <buildPath> must not be a subdir of <srcPath>

    mkdir <buildPath>
    cd <buildfolder>
    qmake -r CONFIG+=disable-breakpad <srcPath>/Project.pro
    make

If you want to build in DEBUG mode change the qmake call to this line:

    qmake -r CONFIG+=debug CONFIG+=disable-breakpad <srcPath>/Project.pro

Some debug options are available for developers of Natron and you can see them in the
global.pri file. To enable an option just add CONFIG+=<option> in the qmake call.

### Nodes

Natron's nodes are contained in separate repositories. To use the default nodes, you must also build the following repositories:

    https://github.com/NatronGitHub/openfx-misc
    https://github.com/NatronGitHub/openfx-io

You'll find installation instructions in the README of both these repositories. Both openfx-misc and openfx-io have submodules as well.

Plugins must be installed in /usr/OFX/Plugins on FreeBSD
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
