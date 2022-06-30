Instructions for installing Natron from sources on macOS
=======================================================

These are step-by-step instructions to compile Natron on macOS.

OS X 10.6 (a.k.a. Snow Leopard) and newer are supported when building with MacPorts, and Homebrew can be used to compile it on the latest macOS (although a few Homebrew casks have to be downgraded).

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

```Shell
sudo port selfupdate
sudo port upgrade outdated
```

Then, you should add the "ports" provided by Natron. Edit as root the file `/opt/local/etc/macports/sources.conf` (as in `sudo nano /opt/local/etc/macports/sources.conf`) and add the following line at the beginning, with the path to the Natron sources (yes, there are three slashes after `file:`):

```
file:///Users/your_username/path_to_sources/Natron/tools/MacPorts
```

Then, create the index file:

```Shell
(cd /Users/your_username/path_to_sources/Natron/tools/MacPorts; portindex)
```

It is also recommended to add the  following line to `/opt/local/etc/macports/variants.conf`:

```
-x11 +no_x11 +bash_completion +no_gnome +quartz +natron
```

#### Notes for OS X 10.6 Snow Leopard

On Snow Leopard, the `+universal` variant may be added to `/opt/local/etc/macports/variants.conf`.

The libtool that comes with OS X 10.6 Snow Leopard does not work well with clang-generated binaries, so on this system you may have to substitute it with the libtool provided by MacPort's `cctools` package, using `sudo mv /usr/bin/libtool /usr/bin/libtool.orig; sudo mv /Developer/usr/bin/libtool /Developer/usr/bin/libtool.orig; sudo ln -s /opt/local/bin/libtool /usr/bin/libtool; sudo ln -s /opt/local/bin/libtool /Developer/usr/bin/libtool`

#### Install Ports

Install the required packages:

```Shell
sudo port -v -N install pkgconfig -universal gsed -universal gawk -universal coreutils -universal findutils -universal
sudo port -v -N install cmake -universal keychain -universal
sudo port -v -N install opencolorio -quartz -python27 -python310
sudo port -v -N install cairo -x11
sudo port -v -N install qt4-mac boost expat
sudo port -v -N install py27-pyside py310-sphinx py310-sphinx_rtd_theme
sudo port select --set python python27
sudo port select --set python2 python27
sudo port select --set python3 python310
sudo port select --set sphinx py310-sphinx
```

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

```Shell
sudo port -v -N install x264
sudo port -v -N install libvpx +highbitdepth
sudo port -v -N install ffmpeg +gpl2 +highbitdepth +natronmini
sudo port -v -N install libraw +gpl2
sudo port -v -N install openexr
sudo port -v -N install opencolorio -quartz -python27 -python310
sudo port -v -N install openimageio +natron
sudo port -v -N install seexpr211 seexpr
```

and for [openfx-arena](https://github.com/NatronGitHub/openfx-arena) (note that it installs a version of ImageMagick without support for many image I/O libraries):

```Shell
sudo port -v -N install librsvg -quartz
sudo port -v -N install ImageMagick +natron -x11
sudo port -v -N install poppler
sudo port -v -N install librevenge
sudo port -v -N install libcdr-0.1
sudo port -v -N install libzip
sudo sed -i -e s/Zstd::Zstd/zstd/ /opt/local/lib/pkgconfig/libzip.pc
sudo port -v -N install sox
```

and for [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic):

```Shell
sudo port -v -N install fftw-3
```

### Homebrew

Install Homebrew from <http://brew.sh/>

#### Qt5

Install Qt5

```Shell
brew install qt@5 pyside@2
# compilation doesn't work with Qt6 linked)
brew unlink qt
# Fix the pkg-config files
brew install gnu-sed
gsed -e s@libdir=lib@libdir=\${prefix}/lib@ -i /usr/local/opt/pyside\@2/lib/pkgconfig/pyside2.pc
gsed -e s@libdir=lib@libdir=\${prefix}/lib@ -i /usr/local/opt/pyside\@2/lib/pkgconfig/shiboken2.pc
```

#### Python2 (optional)

Install Python 2.7 (yes, we know it's deprecated).
```Shell
brew uninstall python@2 || true
cd $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core
git checkout 3a877e3525d93cfeb076fc57579bdd589defc585 Formula/python@2.rb;
brew install python@2
```

#### Qt4 (optional)

Qt 4 is not supported in Homebrew. Please enable the community-maintained recipe using:

```Shell
brew tap cartr/qt4
brew install cartr/qt4/qt@4 cartr/qt4/shiboken@1.2 cartr/qt4/pyside@1.2
```

Patch the qt4 recipe to fix the stack overflow issue (see [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [Homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793)).

Patching a Homebrew recipe is explained in the [Homebrew FAQ](https://github.com/Homebrew/homebrew/blob/master/share/doc/homebrew/FAQ.md).

```Shell
brew edit cartr/qt4/qt@4
```

and before the line that starts with `head`, add the following code:

```
  patch :p0 do
    # bugreports.qt.io may go away, use our local copy.
    # url "https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff"
    # sha256 "477630235eb5ee34ed04e33f156625d1724da290e7a66b745611fb49d17f1347"
    url "https://raw.githubusercontent.com/NatronGitHub/Natron/ff4b3afd3a784f1517002bd82e2da441265385ad/tools/MacPorts/aqua/qt4-mac/files/patch-qthread-stacksize.diff"
    sha256 "a3363ff6460fb4cb4a2a311dbc0724fc5de39d22eb6fa1ec1680d6bd28d01ee4"
  end

  patch :p0 do
    url "https://raw.githubusercontent.com/NatronGitHub/Natron/ff4b3afd3a784f1517002bd82e2da441265385ad/tools/MacPorts/aqua/qt4-mac/files/patch-qt-custom-threadpool.diff"
    sha256 "470c8bf6fbcf01bd15210aad961a476abc73470e201ccb4d62a7c579452de371"
  end
```

#### Other dependencies

Install libraries:

```Shell
brew install cairo expat
brew install gnu-sed gawk coreutils findutils
brew install cmake keychain sphinx-doc
/usr/local/opt/sphinx-doc/libexec/bin/pip3 install sphinx_rtd_theme
brew install --build-from-source qt --with-mysql
```

On macOS Sierra, install the sierra-compatible recipe (to be used only in Sierra, since this builds Qt from sources and takes a while):

```Shell
brew install --build-from-source cartr/qt4/qt --with-mysql
```

Then install pyside (the boneyard tap is for pyside, which does not yet build with Qt5 and was thus removed from the Homebrew core):

```Shell
brew install pyside@1.2 pyside-tools@1.2
```

The last command above will take a while, since it builds from sources, and should finally tell you do do the following if the `homebrew.pth` file does not exist:

```Shell
mkdir -p ~/Library/Python/2.7/lib/python/site-packages
echo 'import site; site.addsitedir("/usr/local/lib/python2.7/site-packages")' >> ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth
sudo ln -s ~/Library/Python/2.7/lib/python/site-packages/homebrew.pth /Library/Python/2.7/lib/python/site-packages/homebrew.pth
```

 To install the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugin, you also need the following:

```Shell
brew install ilmbase openexr freetype fontconfig ffmpeg
brew unlink openimageio || true;
brew unlink opencolorio || true;
brew unlink seexpr || true;
cd $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core
# fetch OCIO 1.1.1 instead of 2.x (and corresponding OIIO version, which builds against it)
# 5f40d55f0ebf04ad15d244cc8edf597afc971bc8 Sun Nov 15 07:35:00 2020 +0000
git checkout 5f40d55f0ebf04ad15d244cc8edf597afc971bc8 Formula/opencolorio.rb;
# f772cb9a399726fd5f3ba859c8f315988afb3d60 Sun Nov 15 19:32:25 2020 +0000
git checkout f772cb9a399726fd5f3ba859c8f315988afb3d60 Formula/openimageio.rb;
# openfx-io still requires SeExpr 2.11, but Homebrew updated it to 3.0.1 on July 6, 2020
git checkout 4abcbc52a544c293f548b0373867d90d4587fd73 Formula/seexpr.rb
brew install opencolorio;
brew link opencolorio;
brew install openimageio;
brew link openimageio;
brew install seexpr
brew link seexpr
git checkout master Formula/seexpr.rb
```

If you ever need to get back to using the latest version of seexpr:

```Shell
rm -rf /usr/local/Cellar/seexpr/2.11
brew link seexpr
```

To install the [openfx-arena](https://github.com/NatronGitHub/openfx-arena) set of plugin, you also need the following:

```Shell
brew install librsvg poppler librevenge libcdr libzip sox
brew uninstall imagemagick
brew install imagemagick --with-hdri --with-librsvg --with-quantum-depth-32 --with-pango
```

To install the [openfx-gmic](https://github.com/NatronGitHub/openfx-gmic) set of plugin, you also need the following:

```Shell
brew install fftw
```

also set the correct value for the pkg-config path (you can also put
this in your .bash_profile):

```Shell
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/curl/lib/pkgconfig
```

### Installing manually (outside of MacPorts or Homebrew) a patched Qt to avoid stack overflows

The following is not necessary if a patched Qt was already installed by the standard MacPorts or Homebrew procedures above.

See [QTBUG-49607](https://bugreports.qt.io/browse/QTBUG-49607), [Homebrew issue #46307](https://github.com/Homebrew/homebrew/issues/46307), [MacPorts ticket 49793](http://trac.macports.org/ticket/49793).

```Shell
wget https://download.qt.io/official_releases/qt/4.8/4.8.7/qt-everywhere-opensource-src-4.8.7.tar.gz
tar zxvf qt-everywhere-opensource-src-4.8.7.tar.gz
cd qt-everywhere-opensource-src-4.8.7
wget https://raw.githubusercontent.com/Homebrew/patches/480b7142c4e2ae07de6028f672695eb927a34875/qt/el-capitan.patch
patch -p1 < el-capitan.patch
wget https://bugreports.qt.io/secure/attachment/52520/patch-qthread-stacksize.diff
patch -p0 < patch-qthread-stacksize.diff
```

Then, configure using:

```Shell
./configure -prefix /opt/qt4 -pch -system-zlib -qt-libtiff -qt-libpng -qt-libjpeg -confirm-license -opensource -nomake demos -nomake examples -nomake docs -cocoa -fast -release
```

* On OS X >= 10.9 add `-platform unsupported/macx-clang-libc++`
* On OS X < 10.9, to compile with clang add `-platform unsupported/macx-clang`
* If compiling with gcc/g++, make sure that `g++ --version`refers to Apple's gcc >= 4.2 or clang >= 318.0.61
* To use another openssl than the system (mainly for security reasons), add `-openssl-linked -I /usr/local/Cellar/openssl/1.0.2d_1/include -L /usr/local/Cellar/openssl/1.0.2d_1/lib` (where the path is changed to your openssl installation).

Then, compile using:

```Shell
make
```

And install (after making sure `/opt/qt4` is user-writable) using:

```Shell
make install
```


## Checkout sources

```Shell
git clone https://github.com/NatronGitHub/Natron.git
cd Natron
```

If you want to compile the bleeding edge version, use the master
branch:

```Shell
git checkout master
```

Update the submodules:

```Shell
git submodule update -i --recursive
```

### Download OpenColorIO-Configs

In the past, OCIO configs were a submodule, though due to the size of the repository, we have chosen instead
to make a tarball release and let you download it [here](https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz).
Place it at the root of Natron source tree:

```Shell
curl -k -L https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.5.tar.gz | tar zxf -
mv OpenColorIO-Configs-Natron-v2.5 OpenColorIO-Configs
```


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
cp config-macports.pri config.pri
```

If you installed libraries using Homebrew, use the following
config.pri:

```Shell
# copy and paste the following in a terminal
cp config-homebrew.pri config.pri
```

Then check at the top of the `config.pri` file that the `HOMEBREW` variable is set to the homebrew installation prefix (usually `/opt/homebrew`).

## Build with Makefile

You can generate a makefile by opening a Terminal, setting the current
directory to the toplevel source directory, and typing

```Shell
qmake -r
```

then type

```Shell
make
```

This will create all binaries in all the subprojects folders.

If you want to build in DEBUG mode change the qmake call to this line:

```Shell
qmake -r CONFIG+=debug
```

* You can also enable logging by adding CONFIG+=log

* You can also enable clang sanitizer by adding CONFIG+=sanitizer

### Building with OpenMP support using clang

It is possible to build Natron using clang (version 3.8 is required,
version 9.0 is recommended) with OpenMP support on
MacPorts (or Homebrew for OS X 10.9 or later).  OpenMP brings speed improvements in the
tracker and in CImg-based plugins.

First, install clang 9.0. On OS X 10.9 and later with MacPorts, simply execute:

```Shell
sudo port -v install clang-9.0
```

Or with Homebrew:

```Shell
brew install llvm
```

On older systems, follow the procedure described in "[https://trac.macports.org/wiki/LibcxxOnOlderSystems](Using libc++ on older system)", and install and set clang-9.0 as the default compiler in the end. Note that we noticed clang 3.9.1 generates wrong code with `-Os` when compiling openexr (later clang versions were not checked), so it is safer to also change `default configure.optflags      {-Os}` to `default configure.optflags      {-O2}` in `/opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` (type `sudo nano /opt/local/libexec/macports/lib/port1.0/portconfigure.tcl` to edit it).

The libtool that comes with OS X 10.6 does not work well with clang-generated binaries, and you may have to `sudo mv /usr/bin/libtool /usr/bin/libtool.orig; sudo mv /Developer/usr/bin/libtool /Developer/usr/bin/libtool.orig; sudo ln -s /opt/local/bin/libtool /usr/bin/libtool; sudo ln -s /opt/local/bin/libtool /Developer/usr/bin/libtool`

Then, configure using the following qmake command on MacPorts:

```Shell
/opt/local/libexec/qt4/bin/qmake QMAKE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_CC=clang-mp-9.0 QMAKE_OBJECTIVE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-9.0 -stdlib=libc++' QMAKE_LD='clang++-mp-9.0 -stdlib=libc++' -r CONFIG+=openmp CONFIG+=enable-osmesa CONFIG+=enable-cairo
```

Or on Homebrew with Qt4/PySide from cartr/qt4:

```Shell
QT_INSTALL_PREFIX=/usr/local
qmake=$QT_INSTALL_PREFIX/bin/qmake
env PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/expat/lib/pkgconfig $qmake -spec macx-xcode CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa CONFIG+=python3 CONFIG+=sdk_no_version_check CONFIG+=openmp -r
```

Or on Homebrew with Qt5/PySide2:

```Shell
QT_INSTALL_PREFIX=/usr/local/opt/qt@5
qmake=$QT_INSTALL_PREFIX/bin/qmake
env PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/opt/X11/lib/pkgconfig:/usr/local/opt/cairo/lib/pkgconfig:/usr/local/opt/icu4c/lib/pkgconfig:/usr/local/opt/libffi/lib/pkgconfig:/usr/local/opt/libxml2/lib/pkgconfig:/usr/local/opt/expat/lib/pkgconfig:/usr/local/opt/qt@5/lib/pkgconfig:/usr/local/opt/pyside@2/lib/pkgconfig $qmake -spec macx-xcode CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa CONFIG+=python3 CONFIG+=sdk_no_version_check CONFIG+=openmp -r
```

To build the plugins, use the following command-line:

```Shell
make CXX='clang++-mp-9.0 -stdlib=libc++' OPENMP=1
```

Or, if you have MangledOSMesa32 installed in `OSMESA_PATH` and LLVM installed in `LLVM_PATH` (MangledOSMesa32 and LLVM build script is available from [https://github.com/devernay/osmesa-install](github:devernay/osmesa-install) :

```Shell
OSMESA_PATH=/opt/osmesa
LLVM_PATH=/opt/llvm
make CXX='clang++-mp-9.0 -stdlib=libc++' OPENMP=1 CXXFLAGS_MESA="-DHAVE_OSMESA" LDFLAGS_MESA="-L${OSMESA_PATH}/lib -lMangledOSMesa32 `${LLVM_PATH}/bin/llvm-config --ldflags --libs engine mcjit mcdisassembler | tr '\n' ' '`" OSMESA_PATH="${OSMESA_PATH}"
```

## Build on Xcode

Follow the instruction of build but
add -spec macx-xcode to the qmake call command:

```Shell
qmake -r -spec macx-xcode
```

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
sudo port install clang-9.0
```

In your config.pri file, add the following lines and change the paths according to your installation of clang

```
openmp {
INCLUDEPATH += /opt/local/include/libomp
LIBS += -L/opt/local/lib/libomp -liomp5 # may also be -lomp

cc_setting.name = CC
cc_setting.value = /opt/local/bin/clang-mp-9.0
cxx_setting.name = CXX
cxx_setting.value = /opt/local/bin/clang++-mp-9.0 -stdlib=libc++
ld_setting.name = LD
ld_setting.value = /opt/local/bin/clang-mp-9.0
ldplusplus_setting.name = LDPLUSPLUS
ldplusplus_setting.value = /opt/local/bin/clang++-mp-9.0 -stdlib=libc++
QMAKE_MAC_XCODE_SETTINGS += cc_setting cxx_setting ld_setting ldplusplus_setting
QMAKE_LFLAGS += "-B /usr/bin"
}
```

The qmake call should add CONFIG+=openmp

```
qmake -r -spec macx-xcode CONFIG+=debug CONFIG+=enable-osmesa LLVM_PATH=/opt/llvm OSMESA_PATH=/opt/osmesa CONFIG+=openmp QMAKE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_CC=clang-mp-9.0 QMAKE_OBJECTIVE_CXX='clang++-mp-9.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-9.0 -stdlib=libc++' QMAKE_LD='clang++-mp-9.0 -stdlib=libc++' -r CONFIG+=openmp CONFIG+=enable-osmesa CONFIG+=enable-cairo
```


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

```Shell
launchctl setenv PATH /opt/local/bin:/opt/local/sbin:/usr/bin:/bin:/usr/sbin:/sbin
```

## Testing

```Shell
(cd Tests && qmake -r CONFIG+=debug CONFIG+=coverage && make -j4 && ./Tests)
```

## Generating Python bindings

This is not required as file generation occurs during build with Qt5 and generated files are already in the repository for Qt4. You would need to run it if you were both under Qt4 and either extend or modify the Python bindings via the
typesystem.xml file. See the documentation of shiboken-2.7 for an explanation of the command line arguments.

On MacPorts with qt4-mac, py310-pyside, py310-shiboken:
```Shell
PYV=3.10 # Set to the python version
QT=4
rm Engine/Qt${QT}/NatronEngine/* Gui/Qt${QT}/NatronGui/*

shiboken-${PYV} --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Global:/opt/local/include:/opt/local/include/PySide-${PYV}  --typesystem-paths=/opt/local/share/PySide-${PYV}/typesystems --output-directory=Engine/Qt${QT} Engine/Pyside_Engine_Python.h  Engine/typesystem_engine.xml

shiboken-${PYV} --avoid-protected-hack --enable-pyside-extensions --include-paths=../Engine:../Gui:../Global:/opt/local/include:/opt/local/include/PySide-${PYV}  --typesystem-paths=/opt/local/share/PySide-${PYV}/typesystems:Engine:Shiboken --output-directory=Gui/Qt${QT} Gui/Pyside_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken.sh Engine/Qt${QT}/NatronEngine natronengine
tools/utils/runPostShiboken.sh Gui/Qt${QT}/NatronGui natrongui
```

Building Natron with Qt5 should generate the Python bindings automatically, but in case it does not,
here are the commands to recreate the Python bindings:

On MacPorts with qt5, py310-pyside2:
```Shell
PYV=3.10 # Set to the python version
PYTHON_PREFIX=$(python${PYV}-config --prefix)
QT=5
# Fix a missing link in the MacPorts package
[ ! -f ${PYTHON_PREFIX}/lib/python${PYV}/site-packages/shiboken2_generator/shiboken2-${PYV} ] && sudo ln -s shiboken2 ${PYTHON_PREFIX}/lib/python${PYV}/site-packages/shiboken2_generator/shiboken2-${PYV}

rm Engine/Qt${QT}/NatronEngine/* Gui/Qt${QT}/NatronGui/*
# ${PYTHON_PREFIX}/lib/python${PYV}/site-packages/PySide2/include
shiboken2-${PYV} --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Engine:Global:libs/OpenFX/include:/opt/local/include:/opt/local/libexec/qt${QT}/include:${PYTHON_PREFIX}/include/python${PYV}:${PYTHON_PREFIX}/lib/python${PYV}/site-packages/PySide2/include --typesystem-paths=${PYTHON_PREFIX}/lib/python${PYV}/site-packages/PySide2/typesystems --output-directory=Engine/Qt${QT} Engine/Pyside2_Engine_Python.h  Engine/typesystem_engine.xml

shiboken2-${PYV} --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Engine:Global:libs/OpenFX/include:/opt/local/include:/opt/local/libexec/qt${QT}/include:/opt/local/libexec/qt${QT}/include/QtWidgets:${PYTHON_PREFIX}/include/python${PYV}:${PYTHON_PREFIX}/lib/python${PYV}/site-packages/PySide2/include --typesystem-paths=${PYTHON_PREFIX}/lib/python${PYV}/site-packages/PySide2/typesystems:Engine:Shiboken --output-directory=Gui/Qt${QT} Gui/Pyside2_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken2.sh Engine/Qt${QT}/NatronEngine natronengine
tools/utils/runPostShiboken2.sh Gui/Qt${QT}/NatronGui natrongui
```

on HomeBrew with Qt5/PySide2/Shiboken2:
```Shell
PYV=3.10 # Set to the python version
export PATH="/usr/local/opt/pyside@2/bin:$PATH"
QT=5
rm Engine/Qt${QT}/NatronEngine/* Gui/Qt${QT}/NatronGui/*

shiboken2 --enable-parent-ctor-heuristic --use-isnull-as-nb_nonzero --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Global:Engine:libs/OpenFX/include:/usr/local/Frameworks/Python.framework/Versions/${PYV}/include/python${PYV}:/usr/local/include:/usr/local/opt/pyside@2/include/PySide2:/usr/local/opt/qt@${QT}/include  --typesystem-paths=/usr/local/opt/pyside@2/share/PySide2/typesystems --output-directory=Engine/Qt${QT} Engine/PySide2_Engine_Python.h  Engine/typesystem_engine.xml

shiboken2 --enable-parent-ctor-heuristic --use-isnull-as-nb_nonzero --avoid-protected-hack --enable-pyside-extensions --include-paths=.:Global:Engine:Gui:libs/OpenFX/include:/usr/local/Frameworks/Python.framework/Versions/${PYV}/include/python${PYV}:/usr/local/include:/usr/local/opt/pyside@2/include/PySide2:/usr/local/opt/qt@${QT}/include:/usr/local/opt/qt@5/include/QtWidgets  --typesystem-paths=/usr/local/opt/pyside@2/share/PySide2/typesystems:Engine --output-directory=Gui/Qt${QT} Gui/PySide2_Gui_Python.h  Gui/typesystem_natronGui.xml

tools/utils/runPostShiboken2.sh Engine/Qt${QT}/NatronEngine natronengine
tools/utils/runPostShiboken2.sh Gui/Qt${QT}/NatronGui natrongui
```

**Note**
Shiboken has a few glitches which needs fixing with some sed commands, run tools/utils/runPostShiboken.sh once shiboken is called

## OpenFX plugins

Instructions to build the [openfx-io](https://github.com/NatronGitHub/openfx-io) and [openfx-misc](https://github.com/NatronGitHub/openfx-misc) sets of plugins can also be found in the [tools/packageOSX.sh](https://github.com/NatronGitHub/Natron/blob/master/tools/packageOSX.sh) script if you are using MacPorts, or in the .travis.yml file in their respective github repositories if you are using Homebrew ([openfx-misc/.travis.yml](https://github.com/NatronGitHub/openfx-misc/blob/master/.travis.yml), [openfx-io/.travis.yml](https://github.com/NatronGitHub/openfx-io/blob/master/.travis.yml).


You can install [TuttleOFX](http://www.tuttleofx.org/) using Homebrew:

```Shell
brew tap homebrew/science homebrew/x11 homebrew/python cbenhagen/video
brew install tuttleofx
```


Or have a look at the [instructions for building on MacPorts as well as precompiled universal binaries](http://devernay.free.fr/hacks/openfx/#OSXTuttleOFX).
