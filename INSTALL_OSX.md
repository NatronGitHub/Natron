Instructions for installing Natron from sources on OS X
=======================================================

These are step-by-step instructions to compile Natron on OS X.

OS X 10.6 (a.k.a. Snow Leopard) and newer are supported when building with MacPorts, and Homebrew can be used to compile it on the latest OS X.

## Official Natron binaries

The official Natron and plugins binaries are built using the section about MacPorts in these instructions to prepare the system, and the `launchBuildMain.sh` build script found in the `tools/jenkins` directory. The script takes care of everything, from checking out sources, to compiling and packaging.

These binaries are built on an OS X 10.9 (Mavericks) virtual machine with [Xcode 6.2](https://developer.apple.com/devcenter/download.action?path=/Developer_Tools/Xcode_6.2/Xcode_6.2.dmg). Note that [Mavericks can not be downloaded anymore from the 10.14 (High Sierra) App Store](https://www.macworld.co.uk/how-to/mac-software/download-old-os-x-3629363/), so you will need to use a Mac with an older system (up to 10.13), or look for [alternatives](https://applehint.com/t/download-all-macos-x-10-4-10-14-original/376).

The build system is based on [MacPorts](https://www.macports.org) with the custom ports found in the `tools/MacPorts` directory in the sources. We are not using [Homebrew](https://brew.sh), because MacPorts is easier to customize.

Everything is compiled using the latest version of [Clang](https://clang.llvm.org) rather than the version bundled with Xcode, in order to have full OpenMP support.


## Building on a VM (Virtual Machine)

Natron can be built on a virtual machine, in order to target older versions of OS X / macOS.  Due to the OS X Software License Agreement, OS X may only be virtualized on Mac hardware. The excellent blog post [OS X on OS X](https://ntk.me/2012/09/07/os-x-on-os-x/) ([archive](https://web.archive.org/web/20190520141144/https://ntk.me/2012/09/07/os-x-on-os-x/)) gives all the instructions to install an OS X virtual machine.


## Install Dependencies

In order to have Natron compiling, first you need to install the required libraries.

There are two exclusive options: using MacPorts or using Homebrew.

Homebrew is easier to set up than MacPorts, but cannot build universal binaries.

### MacPorts

#### Basic Setup

You need an up to date MacPorts version. Just download it and install it from <http://www.macports.org>, and execute the following commands in a terminal:

    sudo port selfupdate
    sudo port upgrade outdated

Then, you should add the "ports" provided by Natron. Edit as root the file `/opt/local/etc/macports/sources.conf` (as in `sudo nano /opt/local/etc/macports/sources.conf`) and add the following line at the beginning, with the path to the Natron sources (yes, there are three slashes after `file:`):

    file:///Users/your_username/path_to_sources/Natron/tools/MacPorts

Then, create the index file:

    (cd /Users/your_username/path_to_sources/Natron/tools/MacPorts; portindex)

It is also recommended to add the  following line to `/opt/local/etc/macports/variants.conf`:

    -x11 +no_x11 +bash_completion +no_gnome +quartz +natron

#### Notes for OS X 10.6 Snow Leopard

On older systems such as 10.6, follow the procedure described in "[https://trac.macports.org/wiki/LibcxxOnOlderSystems](Using libc++ on older system)", and install and set clang-8.0 as the default compiler in the end.
This means that `/opt/local/etc/macports/macports.conf` should contain the following:
```
cxx_stdlib         libc++
buildfromsource    always
delete_la_files    yes
default_compilers  macports-clang-8.0 macports-clang-7.0 macports-clang-6.0 macports-clang-5.0 macports-clang-3.7 macports-clang-3.4 llvm-gcc-4.2 clang gcc-4.2 apple-gcc-4.2 gcc-4.0
```

Also add the following to `/opt/local/etc/macports/variants.conf`:

    -llvm34 -llvm37 -llvm39 -llvm40 -llvm50 -llvm60 -llvm70 +llvm80
    -ld64_97 -ld64_127 -ld64_236

make sure you have the right versions of cctools and ld64:

    port -N install ld64-latest@274.2 +llvm80 cctools@921_2 +llvm80

The libtool that comes with OS X 10.6 Snow Leopard does not work well with clang-generated binaries, so on this system you may have to `sudo mv /usr/bin/libtool /usr/bin/libtool.orig; sudo mv /Developer/usr/bin/libtool /Developer/usr/bin/libtool.orig; sudo ln -s /opt/local/bin/libtool /usr/bin/libtool; sudo ln -s /opt/local/bin/libtool /Developer/usr/bin/libtool`

#### Install Ports

Now, use libjpeg-turbo instead of jpeg:

    sudo port -f uninstall jpeg
    sudo port -v -N install libjpeg-turbo

And finally install the required packages:

    sudo port -v -N install clang-8.0
    sudo port -v -N install opencolorio -quartz -python27
    sudo port -v -N install qt4-mac boost cairo expat
    sudo port -v -N install gsed gawk coreutils findutils
    sudo port -v -N install cmake keychain
    sudo port -v -N install py27-pyside py37-sphinx py37-sphinx_rtd_theme
    sudo port select --set python python27
    sudo port select --set python2 python27
    sudo port select --set python3 python37
    sudo port select --set sphinx py37-sphinx

Create the file /opt/local/lib/pkgconfig/glu.pc containing GLU
configuration, for example using the following comands:

```Shell
sudo -s
cat > /opt/local/lib/pkgconfig/glu.pc << EOF
 prefix=/usr
 exec_prefix=\${prefix}
 libdir=\${exec_prefix}/lib
 includedir=\${prefix}/include


Name: glu
 Version: 2.0
 Description: glu
 Requires:
 Libs:
 Cflags: -I\${includedir}
EOF
```

If you intend to build the [openfx-io](https://github.com/NatronGitHub/openfx-io) plugins too, you will need these additional packages:

    sudo port -v -N install x264
    sudo port -v -N install libvpx +highbitdepth
    sudo port -v -N install ffmpeg +gpl2 +highbitdepth +natronmini
    sudo port -v -N install libraw +gpl2
    sudo port -v -N install openexr
    sudo port -v -N install opencolorio -quartz -python27
    sudo port -v -N install openimageio +natron
    sudo port -v -N install seexpr

and for [openfx-arena](https://github.com/NatronGitHub/openfx-arena) (note that it installs a version of ImageMagick without support for many image I/O libraries):

    sudo port -v -N install librsvg
    sudo port -v -N install ImageMagick +natron
    sudo port -v -N install poppler
    sudo port -v -N install librevenge
    sudo port -v -N install libcdr-0.1
    sudo port -v -N install libzip

and for [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic):

    sudo port -v -N install fftw-3

### Homebrew

Install homebrew from <http://brew.sh/>

Qt 4 is not supported in homebrew. Please enable the community-maintained recipe using:

    brew tap cartr/qt4
    brew tap-pin cartr/qt4
    brew install qt@4 shiboken@1.2

Patch the qt4 recipe to fix the stack overflow issue (see [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793)).

Patching a homebrew recipe is explained in the [homebrew FAQ](https://github.com/Homebrew/homebrew/blob/master/share/doc/homebrew/FAQ.md).

    brew edit qt@4

and before the line that starts with `head`, add the following code:

      patch :p0 do
        url "https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff"
        sha256 "477630235eb5ee34ed04e33f156625d1724da290e7a66b745611fb49d17f1347"
      end


Install libraries:

    brew tap homebrew/python
    brew tap homebrew/science
    brew install cairo expat
    brew install gnu-sed gawk coreutils findutils
    brew install cmake keychain sphinx-doc
    /usr/local/opt/sphinx-doc/libexec/bin/pip3 install sphinx_rtd_theme
    brew install --build-from-source qt --with-mysql

On macOS Sierra, install the sierra-compatible recipe (to be used only in Sierra, since this builds Qt from sources and takes a while):

    brew install --build-from-source cartr/qt4/qt --with-mysql

Then install pyside (the boneyard tap is for pyside, which does not yet build with Qt5 and was thus removed from the homebrew core):

    brew install pyside@1.2 pyside-tools@1.2

The last command above will take a while, since it builds from sources, and should finally tell you do do the following if the `homebrew.pth` file does not exist:

    mkdir -p ~/Library/Python/2.7/lib/python/site-packages
    echo 'import site; site.addsitedir("/usr/local/lib/python2.7/site-packages")' >> ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth
    sudo ln -s ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth /Library/Python/2.7/lib/python/site-packages/homebrew.pth

 To install the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugin, you also need the following:

    brew install ilmbase openexr freetype fontconfig ffmpeg opencolorio openimageio seexpr

To install the [openfx-arena](https://github.com/NatronGitHub/openfx-arena) set of plugin, you also need the following:

    brew install librsvg poppler librevenge libcdr libzip
    brew uninstall imagemagick
    brew install imagemagick --with-hdri --with-librsvg --with-quantum-depth-32 --with-pango

To install the [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic) set of plugin, you also need the following:

    brew install fftw

also set the correct value for the pkg-config path (you can also put
this in your .bash_profile):

    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig

### Installing manually (outside of MacPorts or homebrew) a patched Qt to avoid stack overflows

The following is not necessary if a patched Qt was already installed by the standard MacPorts or homebrew procedures above.

See [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793).

    wget https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz
    tar zxvf qt-everywhere-opensource-src-4.8.7.tar.gz
    cd qt-everywhere-opensource-src-4.8.7
    wget https://raw.githubusercontent.com/Homebrew/patches/480b7142c4e2ae07de6028f672695eb927a34875/qt/el-capitan.patch
    patch -p1 < el-capitan.patch
    wget https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff
    patch -p0 < patch-qthread-stacksize.diff

Then, configure using:

    ./configure -prefix /opt/qt4 -pch -system-zlib -qt-libtiff -qt-libpng -qt-libjpeg -confirm-license -opensource -nomake demos -nomake examples -nomake docs -cocoa -fast -release

* On OS X >= 10.9 add `-platform unsupported/macx-clang-libc++`
* On OS X < 10.9, to compile with clang add `-platform unsupported/macx-clang`
* If compiling with gcc/g++, make sure that `g++ --version`refers to Apple's gcc >= 4.2 or clang >= 318.0.61
* To use another openssl than the system (mainly for security reasons), add `-openssl-linked -I /usr/local/Cellar/openssl/1.0.2d_1/include -L /usr/local/Cellar/openssl/1.0.2d_1/lib` (where the path is changed to your openssl installation).

Then, compile using:

    make

And install (after making sure `/opt/qt4` is user-writable) using:

    make install


## Checkout sources

    git clone https://github.com/NatronGitHub/Natron.git
    cd Natron

If you want to compile the bleeding edge version, use the master
branch:

    git checkout master

Update the submodules:

    git submodule update -i --recursive

### Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.3.tar.gz).
Place it at the root of Natron source tree:

    curl -k -L https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.3.tar.gz | tar zxf -
    mv OpenColorIO-Configs-Natron-v2.1 OpenColorIO-Configs


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
boost-serialization-lib: LIBS += -lboost_serialization-mt
boost: LIBS += -L/opt/local/lib -lboost_thread-mt -lboost_system-mt
macx:openmp {
  QMAKE_CC=/opt/local/bin/clang-mp-8.0
  QMAKE_CXX=/opt/local/bin/clang++-mp-8.0
  QMAKE_OBJECTIVE_CC=$$QMAKE_CC -stdlib=libc++
  QMAKE_LINK=$$QMAKE_CXX

  INCLUDEPATH += /opt/local/include/libomp
  LIBS += -L/opt/local/lib/libomp -liomp5
  cc_setting.name = CC
  cc_setting.value = /opt/local/bin/clang-mp-8.0
  cxx_setting.name = CXX
  cxx_setting.value = /opt/local/bin/clang++-mp-8.0
  ld_setting.name = LD
  ld_setting.value = /opt/local/bin/clang-mp-8.0
  ldplusplus_setting.name = LDPLUSPLUS
  ldplusplus_setting.value = /opt/local/bin/clang++-mp-7.0
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
  QMAKE_FLAGS = "-B /usr/bin"

  # clang (as of 5.0) does not yet support index-while-building functionality
  # present in Xcode 9, and Xcode 9's clang does not yet support OpenMP
  compiler_index_store_enable_setting.name = COMPILER_INDEX_STORE_ENABLE
  compiler_index_store_enable_setting.value = NO
  QMAKE_MAC_XCODE_SETTINGS += compiler_index_store_enable_setting
}
EOF
```

If you installed libraries using Homebrew, use the following
config.pri:

```Shell
 # copy and paste the following in a terminal
cat > config.pri << EOF
boost: INCLUDEPATH += /usr/local/include
boost-serialization-lib: LIBS += -lboost_serialization-mt
boost: LIBS += -L/usr/local/lib -lboost_thread-mt -lboost_system-mt
expat: PKGCONFIG -= expat
expat: INCLUDEPATH += /usr/local/opt/expat/include
expat: LIBS += -L/usr/local/opt/expat/lib -lexpat
macx:openmp {
  QMAKE_CC=/usr/local/opt/llvm/bin/clang
  QMAKE_CXX=/usr/local/opt/llvm/bin/clang++
  QMAKE_OBJECTIVE_CC=$$QMAKE_CC -stdlib=libc++
  QMAKE_LINK=$$QMAKE_CXX

  LIBS += -L/usr/local/opt/llvm/lib -lomp
  cc_setting.name = CC
  cc_setting.value = /usr/local/opt/llvm/bin/clang
  cxx_setting.name = CXX
  cxx_setting.value = /usr/local/opt/llvm/bin/clang++
  ld_setting.name = LD
  ld_setting.value = /usr/local/opt/llvm/bin/clang
  ldplusplus_setting.name = LDPLUSPLUS
  ldplusplus_setting.value = /usr/local/opt/llvm/bin/clang++
  QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
  QMAKE_FLAGS = "-B /usr/bin"

  # clang (as of 5.0) does not yet support index-while-building functionality
  # present in Xcode 9, and Xcode 9's clang does not yet support OpenMP
  compiler_index_store_enable_setting.name = COMPILER_INDEX_STORE_ENABLE
  compiler_index_store_enable_setting.value = NO
  QMAKE_MAC_XCODE_SETTINGS += compiler_index_store_enable_setting
}
EOF
```

## Build with Makefile

You can generate a makefile by opening a Terminal, setting the current
directory to the toplevel source directory, and typing

    qmake -r

then type

    make

This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

    qmake -r CONFIG+=debug

* You can also enable logging by adding CONFIG+=log

* You can also enable clang sanitizer by adding CONFIG+=sanitizer

### Building with OpenMP support using clang

It is possible to build Natron using clang (version 3.8 is required,
version 8.0 is recommended) with OpenMP support on
MacPorts (or homebrew for OS X 10.9 or later).  OpenMP brings speed improvements in the
tracker and in CImg-based plugins.

First, install clang 8.0. On OS X 10.9 and later with MacPorts, simply execute:

    sudo port -v install clang-8.0

Or with homebrew:

    brew install llvm

On older systems, follow the procedure described in "[https://trac.macports.org/wiki/LibcxxOnOlderSystems](Using libc++ on older system)", and install and set clang-8.0 as the default compiler in the end. Note that we noticed clang 3.9.1 generates wrong code with `-Os` when compiling openexr (later clang versions were not checked), so it is safer to also change `default configure.optflags      {-Os}` to `default configure.optflags      {-O2}` in `/opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` (type `sudo nano /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` to edit it).

The libtool that comes with OS X 10.6 does not work well with clang-generated binaries, and you may have to `sudo mv /usr/bin/libtool /usr/bin/libtool.orig; sudo mv /Developer/usr/bin/libtool /Developer/usr/bin/libtool.orig; sudo ln -s /opt/local/bin/libtool /usr/bin/libtool; sudo ln -s /opt/local/bin/libtool /Developer/usr/bin/libtool`

Then, configure using the following qmake command:

    /opt/local/libexec/qt4/bin/qmake QMAKE_CXX='clang++-mp-8.0 -stdlib=libc++' QMAKE_CC=clang-mp-8.0 QMAKE_OBJECTIVE_CXX='clang++-mp-8.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-8.0 -stdlib=libc++' QMAKE_LD='clang++-mp-8.0 -stdlib=libc++' -r CONFIG+=openmp CONFIG+=enable-osmesa CONFIG+=enable-cairo

To build the plugins, use the following command-line:

    make CXX='clang++-mp-8.0 -stdlib=libc++' OPENMP=1

Or, if you have MangledOSMesa32 installed in `OSMESA_PATH` and LLVM installed in `LLVM_PATH` (MangledOSMesa32 and LLVM build script is available from [https://github.com/devernay/osmesa-install](github:devernay/osmesa-install) :

    OSMESA_PATH=/opt/osmesa
    LLVM_PATH=/opt/llvm
    make CXX='clang++-mp-8.0 -stdlib=libc++' OPENMP=1 CXXFLAGS_MESA="-DHAVE_OSMESA" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -lMangledOSMesa32 `${LLVM_PATH}/bin/llvm-config --ldflags --libs engine mcjit mcdisassembler | tr '\n' ' '`" OSMESA_PATH="${OSMESA_PATH}"


## Build on Xcode

Follow the instruction of build but
add -spec macx-xcode to the qmake call command:

    qmake -r -spec macx-xcode

Then open the already provided Project-xcode.xcodeproj and compile the target "all"

### Compiling plugins with Xcode

The source distributions of the plugin sets `openfx-io` and
`openfx-misc` contain Xcode projects, but these require setting a few
global variables in Xcode. These variables can be used to switch
between the system-installed version of a package and a custom install
(e.g. if you need to debug something that happens in OpenImageIO).

In Xcode Preferences, select "Locations", then "Source Trees", and add the following
variable names/values (Xcode may need to be restarted after setting these):
- `LOCAL`: `/usr/local` on Homebrew, `/opt/local` on MacPorts
- `BOOST_PATH`: `$(LOCAL)/include`
- `EXR_PATH`: `$(LOCAL)`
- `FFMPEG_PATH`: `$(LOCAL)`
- `OCIO_PATH`: `$(LOCAL)`
- `OIIO_PATH`: `$(LOCAL)`
- `OPENCV_PATH`: `$(LOCAL)`
- `SEEXPR_PATH`: `$(LOCAL)`

It is also recommended in Xcode Preferences, select "Locations", then
"Locations", to set the Derived Data location to be Relative, and in
the advanced settings to set the build location to Legacy (if not,
build files are somewhere under `~/Library/Developer/Xcode`.

### Build on Xcode with openmp clang

See instructions under "Using clang-omp with Xcode" at the following page https://clang-omp.github.io

On Macports clang now ships with openmp by default. To install it:
```
sudo port install clang-8.0
```

In your config.pri file, add the following lines and change the paths according to your installation of clang

```
openmp {
INCLUDEPATH += /opt/local/include/libomp
LIBS += -L/opt/local/lib/libomp -liomp5 # may also be -lomp

cc_setting.name = CC
cc_setting.value = /opt/local/bin/clang-mp-8.0
cxx_setting.name = CXX
cxx_setting.value = /opt/local/bin/clang++-mp-8.0 -stdlib=libc++
ld_setting.name = LD
ld_setting.value = /opt/local/bin/clang-mp-8.0
ldplusplus_setting.name = LDPLUSPLUS
ldplusplus_setting.value = /opt/local/bin/clang++-mp-8.0 -stdlib=libc++
QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
QMAKE_LFLAGS += "-B /usr/bin"
}
```

The qmake call should add CONFIG+=openmp

qmake -r -spec macx-xcode CONFIG+=debug CONFIG+=enable-osmesa LLVM_PATH=/opt/llvm OSMESA_PATH=/opt/osmesa CONFIG+=openmp


Then you can just build and run using Xcode

### Xcode caveats

Whenever the .pro files change, Xcode will try to launch qmake and
probably fail because it doesn't find the necessary binaries (qmake,
moc, pkg-config, python3-config, etc.). In this case, just open a
Terminal and relaunch the above command. This will rebuild the Xcode projects.

Alternatively, you can globally add the necessary directories
(`/usr/local/bin`on Homebrew, `/opt/local/bin` on MacPorts) to you
PATH (see <http://www.emacswiki.org/emacs/EmacsApp> for instructions).

On MacPorts, this would look like:

    launchctl setenv PATH /opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin

## Testing

    (cd Tests && qmake -r CONFIG+=debug CONFIG+=coverage && make -j4 && ./Tests)

## Generating Python bindings

This is not required as generated files are already in the repository. You would need to run it if you were to extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken-2.7 for an explanation of the command line arguments.

On MacPorts:
```Shell
rm Engine/NatronEngine/* Gui/NatronGui/*

shiboken-2.7 --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:/opt/local/include:/opt/local/include/PySide-2.7  --typesystem-paths=/opt/local/share/PySide-2.7/typesystems --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken-2.7 --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:/opt/local/include:/opt/local/include/PySide-2.7  --typesystem-paths=/opt/local/share/PySide-2.7/typesystems:Engine --output-directory=Gui Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh
```

on HomeBrew:
```Shell
rm Engine/NatronEngine/* Gui/NatronGui/*

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:/usr/local/include:/usr/local/include/PySide  --typesystem-paths=/usr/local/share/PySide/typesystems --output-directory=Engine Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:/usr/local/include:/usr/local/include/PySide  --typesystem-paths=/usr/local/share/PySide/typesystems:Engine --output-directory=Gui Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh
```

**Note**
Shiboken has a few glitches which needs fixing with some sed commands, run tools/utils/runPostShiboken.sh once shiboken is called

## OpenFX plugins

Instructions to build the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugins can also be found in the [tools/packageOSX.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/packageOSX.sh) script if you are using MacPorts, or in the .travis.yml file in their respective github repositories if you are using homebrew ([openfx-misc/.travis.yml](https://github.com/NatronGitHub/openfx-misc/blob/master/.travis.yml), [openfx-io/.travis.yml](https://github.com/NatronGitHub/openfx-io/blob/master/.travis.yml).


You can install [TuttleOFX](http://www.tuttleofx.org/) using homebrew:

    brew tap homebrew/science homebrew/x11 homebrew/python cbenhagen/video
    brew install tuttleofx


Or have a look at the [instructions for building on MacPorts as well as precompiled universal binaries](http://devernay.free.fr/hacks/openfx/#OSXTuttleOFX).
