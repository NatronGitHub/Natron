Instructions for installing Natron from sources on FreeBSD
==========================================================

This file is supposed to guide you step by step to have working (compiling) version of
Natron on FreeBSD. 

## Install libraries

In order to have Natron compiling, first you need to install the required libraries.

```
pkg install qt5 boost-all pyside2 expat cairo pkgconf
```

For the CMake build, also install `cmake` (`pkg install cmake`).

### Submodules

Go under Natron and type

```
git submodule update -i --recursive
```

### Add the config.pri file

*`config.pri` is only used by the deprecated qmake build (see [Deprecated: building with qmake](#deprecated-building-with-qmake)); the CMake build doesn't need it.*

You might need to define the locations of some of the required libraries.
This is done by creating a `.pri` file next to the `Project.pro` that will tell the `.pro` where to find those libraries.
The only library to put in the `config.pri` file on unix systems is boost.
For all other libraries are found with PKGConfig.

Then you can create the `config.pri` file next to the `Project.pro` file just in case you need to pass more configurations.
You can fill it with the following proposed code to point to the libraries.
Of course you need to provide valid paths that are valid on your system.

`INCLUDEPATH` is the path to the include files

`LIBS` is the path to the libs

The following `config.pri` adds `boost_serialization` to the libraries, `cairo`, `expat` and `fontconfig` to pkgconfig and removes static `cairo`.

```pri
boost: LIBS += -lboost_serialization
PKGCONFIG += expat
PKGCONFIG += fontconfig
cairo {
        PKGCONFIG += cairo
        LIBS -=  $$system(pkg-config --variable=libdir cairo)/libcairo.a
}
```

###Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.4.tar.gz).
Place it at the root of Natron repository.

### Build

Natron is built with CMake. From the Natron source directory:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j$(sysctl -n hw.ncpu)
```

*These commands are the ones used on Linux, macOS and Windows; they haven't been tested on FreeBSD, where there is no CI. Please report problems (or fixes) on GitHub.*

Useful options (add them to the first `cmake` command): `-DCMAKE_BUILD_TYPE=Debug` or `Release`, `-DNATRON_QT6=ON` (Qt 6 and PySide6), `-DNATRON_BUILD_TESTS=OFF` (skip the unit tests), `-DNATRON_SYSTEM_LIBS=ON` (use system copies of the libraries in `libs/`). `cmake --install build --prefix /usr/local` installs Natron.

### Deprecated: building with qmake

*The qmake build is deprecated and will be removed in a future release (first on RB-2.7, see [#1181](https://github.com/NatronGitHub/Natron/issues/1181)). It needs a [config.pri](#add-the-configpri-file) file.*

The `<srcPath>` must be absolute and `<buildPath>` must not be a subdir of `<srcPath>`

```
mkdir <buildPath>
cd <buildfolder>
qmake -r CONFIG+=disable-breakpad <srcPath>/Project.pro
make
```

If you want to build in DEBUG mode change the qmake call to this line:

```
qmake -r CONFIG+=debug CONFIG+=disable-breakpad <srcPath>/Project.pro
```

Some debug options are available for developers of Natron and you can see them in the `global.pri` file. To enable an option just add `CONFIG+=<option>` in the qmake call.

### Nodes

Natron's nodes are contained in separate repositories. To use the default nodes, you must also build the following repositories:

    https://github.com/NatronGitHub/openfx-misc
    https://github.com/NatronGitHub/openfx-io

You'll find installation instructions in the `README` of both these repositories. Both openfx-misc and openfx-io have submodules as well.

Plugins can be installed in `/usr/OFX/Plugins` on FreeBSD.
Or in a directory named "`Plugins`" located in the parent directory where the binary lies, e.g.:

```
bin/
    Natron
Plugins/
    IO.ofx.bundle
```
	
### OpenColorIO configs

Note that if you want Natron to find the OpenColorIO config files you will need to
place them in the appropriate location. In the repository they are located under
`Gui/Resources/OpenColorIO-Configs`.
You must copy them to a directory named `../share/OpenColorIO-Configs` relative to Natron's binary.
