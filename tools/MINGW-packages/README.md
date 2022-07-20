# Natron Windows SDK

The following are instructions for installing and using the Natron SDK on Windows.

# Setup MSYS2

First you need to download a MSYS2 snapshot, only **``20180531``** is supported.

* Download and extract [msys2-base-x86_64-20180531.tar.xz](http://repo.msys2.org/distrib/x86_64/msys2-base-x86_64-20180531.tar.xz) [[Mirror](https://sourceforge.net/projects/natron/files/MINGW-packages/msys2-base-x86_64-20180531.tar.xz/download)]
* Move the ``msys64`` folder to ``C:\msys64-20180531``
* Run ``C:\msys64-20180531\mingw64.exe`` (this will trigger a setup script, wait until done)
* When prompted close the terminal window

Start ``C:\msys64-20180531\mingw64.exe``, then run the following commands:

	$ sed -i 's/SigLevel    = Required DatabaseOptional/SigLevel = Never/' /etc/pacman.conf
	$ sed -i 's|#XferCommand = /usr/bin/wget --passive-ftp -c -O %o %u|XferCommand = /usr/bin/wget --no-check-certificate --passive-ftp -c -O %o %u|' /etc/pacman.conf
	$ echo "Server = https://downloads.sourceforge.net/project/natron/MINGW-packages/mingw64" > /etc/pacman.d/mirrorlist.mingw64
	$ echo "Server = https://downloads.sourceforge.net/project/natron/MINGW-packages/msys" > /etc/pacman.d/mirrorlist.msys
	$ pacman -Syu natron-sdk

This will install everything required to build/package Natron, plug-ins and dependencies. If some packages fail to download re-run the last command.

The SDK can be updated with the following command:

```
pacman -Syu
```

# Setup a development environment (Qt Creator)

Start Qt Creator and go to ``Tools`` => ``Options``.

## Debuggers

Go to the ``Kits`` menu and select ``Debuggers``, then add a new.

* ``Name`` : ``Natron GDB``
* ``Path`` : ``C:\msys64-20180531\mingw64\bin\gdb.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Compilers

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C``).

* ``Name`` : ``Natron MinGW C``
* ``Path`` : ``C:\msys64-20180531\mingw64\bin\gcc.exe``

Go to the ``Kits`` menu and select ``Compilers``, then add a new (``MINGW => C++``).

* ``Name`` : ``Natron MinGW C++``
* ``Path`` : ``C:\msys64-20180531\mingw64\bin\g++.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Qt

Go to the ``Kits`` menu and select ``Qt Versions``, then add a new.

* ``Name`` : ``Natron Qt 4.8.7``
* ``Path`` : ``C:\msys64-20180531\mingw64\bin\qmake.exe``

*It's recommended to click ``Apply`` after adding something to the Kit.*

## Kit

Go to the ``Kits`` menu and select ``Kits``, then add a new.

* ``Name`` : ``Natron MSYS2``
* ``Compiler`` => ``C`` : ``Natron MinGW C``
* ``Compiler`` => ``C++`` : ``Natron MinGW C++``
* ``Debugger`` : ``Natron GDB``
* ``Qt version`` : ``Natron Qt 4.8.7``

## Project

Clone the Natron source (using Git Bash or MSYS2):

```
git clone https://github.com/NatronGitHub/Natron
cd Natron
git submodule update -i --recursive
wget https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v2.4.tar.gz
tar xvf Natron-v2.4.tar.gz
mv OpenColorIO-Configs-Natron-v2.4 OpenColorIO-Configs
touch config.pri
```

Then start Qt Creator and open ``Project.pro`` from the Natron source folder and select the custom kit (``Natron MSYS2``) added previous. When Qt Creator is finished loading the project go to the ``Projects`` tab => ``Natron MSYS2`` => ``Build`` => ``Build Environment`` and add a new environment variable ``PYTHONHOME`` with the value ``C:\msys64-20180531\mingw64``.

You should now be able to build and run release and debug versions of Natron directly from Qt Creator.

# Setup a CI environment

With a working MSYS2 installation launch ``C:\msys64-20180531\mingw64.exe`` and clone the build tools:

```
git clone https://github.com/NatronGitHub/Natron NatronCI
```

Now create a build script:

```
#!/bin/bash
# Build Natron Windows Snapshot
#

export WORKSPACE=$HOME/NatronCI-workspace
export UNIT_TESTS=true
export BUILD_NAME=snapshots
export BUILD_NUMBER=RB-2.4
export GIT_URL=https://github.com/NatronGitHub/Natron.git
export SNAPSHOT_BRANCH=RB-2.4
export GIT_BRANCH=RB-2.4
export NATRON_LICENSE=GPL
export NATRON_DEV_STATUS=SNAPSHOT
export NATRON_BUILD_NUMBER=1
export COMPILE_TYPE=relwithdebinfo
export BITS=64
export DISABLE_BREAKPAD=1
export DISABLE_PORTABLE_ARCHIVE=0
export REMOTE_URL=NO_URL
export REMOTE_USER=NO_USER
export REMOTE_PREFIX=NO_PREFIX
export MKJOBS=4
export LOCALAPPDATA="/c/Users/USERNAME/AppData/Local"
export MSYSTEM=MINGW64
export NOUPDATE=1
export GIT_URL_IS_NATRON=1

if [ ! -d "$WORKSPACE" ]; then
    mkdir -p "$WORKSPACE"
fi

./launchBuildMain.sh
```

and save to ``$HOME/NatronCI/tools/jenkins/build-snapshot.sh``. Remember to change ``LOCALAPPDATA`` to match your username, and ``MKJOBS`` to match your CPU. You can disable unit tests with ``UNIT_TESTS=false``.

Now run the script *(working internet connection needed)*:

```
cd $HOME/NatronCI/tools/jenkins
bash build-snapshot.sh
```

The result will be in ``$HOME/NatronCI-workspace``.

This was just an example, see ``launchBuildMain.sh`` for more information/options.

# Setup MSYS2 from scratch

*This information is for maintainers and is outdated. Test with the latest MSYS2 snapshot and update the list.*

Install a MSYS2 snapshot and the following packages:

```
pacman -S \
automake-wrapper \
wget \
tar \
dos2unix \
diffutils \
file \
gawk \
libreadline \
gettext \
grep \
make \
patch \
patchutils \
pkg-config \
sed \
unzip \
git \
bison \
flex \
gperf \
lndir \
m4 \
perl \
rsync \
zip \
autoconf \
m4 \
libtool \
scons \
yasm \
nasm \
python2-pip \
setconf \
texinfo \
mingw-w64-x86_64-binutils \
mingw-w64-x86_64-crt-git \
mingw-w64-x86_64-gcc \
mingw-w64-x86_64-gcc-ada \
mingw-w64-x86_64-gcc-fortran \
mingw-w64-x86_64-gcc-libgfortran \
mingw-w64-x86_64-gcc-libs \
mingw-w64-x86_64-gcc-objc \
mingw-w64-x86_64-gdb \
mingw-w64-x86_64-headers-git \
mingw-w64-x86_64-libmangle-git \
mingw-w64-x86_64-libwinpthread-git \
mingw-w64-x86_64-make \
mingw-w64-x86_64-pkg-config \
mingw-w64-x86_64-tools-git \
mingw-w64-x86_64-winpthreads-git \
mingw-w64-x86_64-winstorecompat-git \
mingw-w64-x86_64-pkg-config \
mingw-w64-x86_64-python2-sphinx \
mingw-w64-x86_64-python2-sphinx-alabaster-theme \
mingw-w64-x86_64-python2-sphinx_rtd_theme \
mingw-w64-x86_64-python2-packaging \
mingw-w64-x86_64-python2-pyparsing \
mingw-w64-x86_64-gsm \
mingw-w64-x86_64-cmake \
mingw-w64-x86_64-curl \
mingw-w64-x86_64-expat \
mingw-w64-x86_64-jsoncpp \
mingw-w64-x86_64-libarchive \
mingw-w64-x86_64-libbluray \
mingw-w64-x86_64-libuv \
mingw-w64-x86_64-libiconv \
mingw-w64-x86_64-libmng \
mingw-w64-x86_64-libxml2 \
mingw-w64-x86_64-libxslt \
mingw-w64-x86_64-zlib \
mingw-w64-x86_64-pcre \
mingw-w64-x86_64-qtbinpatcher \
mingw-w64-x86_64-sqlite3 \
mingw-w64-x86_64-pkg-config \
mingw-w64-x86_64-gdbm \
mingw-w64-x86_64-db \
mingw-w64-x86_64-python2 \
mingw-w64-x86_64-python2-mako \
mingw-w64-x86_64-boost \
mingw-w64-x86_64-libjpeg-turbo \
mingw-w64-x86_64-libpng \
mingw-w64-x86_64-libtiff \
mingw-w64-x86_64-giflib \
mingw-w64-x86_64-lcms2 \
mingw-w64-x86_64-glew \
mingw-w64-x86_64-pixman \
mingw-w64-x86_64-cairo \
mingw-w64-x86_64-openssl \
mingw-w64-x86_64-freetype \
mingw-w64-x86_64-fontconfig \
mingw-w64-x86_64-eigen3 \
mingw-w64-x86_64-pango \
mingw-w64-x86_64-librsvg \
mingw-w64-x86_64-libzip \
mingw-w64-x86_64-libcdr \
mingw-w64-x86_64-lame \
mingw-w64-x86_64-speex \
mingw-w64-x86_64-libtheora \
mingw-w64-x86_64-wavpack \
mingw-w64-x86_64-xvidcore \
mingw-w64-x86_64-tinyxml \
mingw-w64-x86_64-yaml-cpp \
mingw-w64-x86_64-firebird2-git \
mingw-w64-x86_64-libmariadbclient \
mingw-w64-x86_64-postgresql \
mingw-w64-x86_64-ruby \
mingw-w64-x86_64-dbus \
mingw-w64-x86_64-libass \
mingw-w64-x86_64-libcaca \
mingw-w64-x86_64-libmodplug \
mingw-w64-x86_64-opencore-amr \
mingw-w64-x86_64-opus \
mingw-w64-x86_64-rtmpdump-git \
mingw-w64-x86_64-openal \
mingw-w64-x86_64-opencore-amr \
mingw-w64-x86_64-celt \
mingw-w64-x86_64-gnutls \
mingw-w64-x86_64-fftw \
mingw-w64-x86_64-c-ares \
mingw-w64-x86_64-gdk-pixbuf2 \
mingw-w64-x86_64-gettext \
mingw-w64-x86_64-glib2 \
mingw-w64-x86_64-harfbuzz \
mingw-w64-x86_64-icu \
mingw-w64-x86_64-libvorbis \
mingw-w64-x86_64-ncurses \
mingw-w64-x86_64-nghttp2 \
mingw-w64-x86_64-nspr \
mingw-w64-x86_64-nss \
mingw-w64-x86_64-python3 \
mingw-w64-x86_64-rhash \
mingw-w64-x86_64-jasper \
mingw-w64-x86_64-ptex \
mingw-w64-x86_64-pugixml \
mingw-w64-x86_64-openh264 \
mingw-w64-x86_64-openjpeg2 \
mingw-w64-x86_64-libwebp \
mingw-w64-x86_64-libmfx \
mingw-w64-x86_64-srt \
mingw-w64-x86_64-x265 \
mingw-w64-x86_64-snappy \
mingw-w64-x86_64-libvpx \
mingw-w64-x86_64-l-smash \
mingw-w64-x86_64-poppler-data \
mingw-w64-x86_64-gtk-doc \
mingw-w64-x86_64-python3-setuptools \
mingw-w64-x86_64-meson \
mingw-w64-x86_64-gobject-introspection \
mingw-w64-x86_64-cppunit \
mingw-w64-x86_64-python2-colorama \
mingw-w64-x86_64-python2-docutils \
mingw-w64-x86_64-python2-imagesize \
mingw-w64-x86_64-python2-jinja \
mingw-w64-x86_64-python2-pygments \
mingw-w64-x86_64-python3-pygments \
mingw-w64-x86_64-python2-requests \
mingw-w64-x86_64-python2-setuptools \
mingw-w64-x86_64-python2-six \
mingw-w64-x86_64-python2-sqlalchemy \
mingw-w64-x86_64-python2-whoosh \
mingw-w64-x86_64-python2-nose \
mingw-w64-x86_64-python2-snowballstemmer \
mingw-w64-x86_64-python2-babel \
mingw-w64-x86_64-python2-sphinx-alabaster-theme \
mingw-w64-x86_64-python2-sphinx_rtd_theme \
mingw-w64-x86_64-python2-mock \
mingw-w64-x86_64-python2-enum34 \
mingw-w64-x86_64-python2-pytest \
mingw-w64-x86_64-python3-pytest \
mingw-w64-x86_64-jemalloc
```

Now build and install all the packages in this directory (tools/MINGW-packages) in the following order:

```
mingw-w64-zlib
mingw-w64-bzip2
mingw-w64-xz
mingw-w64-openssl10
mingw-w64-libzip
mingw-w64-libjpeg-turbo
mingw-w64-giflib
mingw-w64-libpng
mingw-w64-libtiff
mingw-w64-lcms2
mingw-w64-libwebp
mingw-w64-lzo2
mingw-w64-jasper
mingw-w64-ninja
mingw-w64-meson
mingw-w64-pixman
mingw-w64-cairo
mingw-w64-libdatrie
mingw-w64-libthai
mingw-w64-fribidi
mingw-w64-pango
mingw-w64-gdk-pixbuf2
mingw-w64-librsvg
mingw-w64-openh264
mingw-w64-x265
mingw-w64-x264-git
mingw-w64-openjpeg2
mingw-w64-libvpx
mingw-w64-librevenge
mingw-w64-libcdr
mingw-w64-libmng
mingw-w64-openexr
mingw-w64-aom
mingw-w64-srt
mingw-w64-sqlite3
mingw-w64-nspr
mingw-w64-nss
mingw-w64-nettle
mingw-w64-gnutls
mingw-w64-nghttp2
mingw-w64-curl
mingw-w64-snappy
mingw-w64-xvidcore
mingw-w64-dav1d
mingw-w64-rtmpdump-git
mingw-w64-libraw-gpl2
mingw-w64-ffmpeg-gpl2
mingw-w64-libarchive
mingw-w64-cmake
mingw-w64-poppler
mingw-w64-opencolorio-git
mingw-w64-python2-sphinx
mingw-w64-python-sphinx_rtd_theme
mingw-w64-python-sphinx-alabaster-theme
mingw-w64-qt4
mingw-w64-seexpr-git
mingw-w64-shiboken-qt4
mingw-w64-pyside-qt4
mingw-w64-libmad
mingw-w64-sox
mingw-w64-osmesa
mingw-w64-libde265
mingw-w64-libheif
mingw-w64-imagemagick
mingw-w64-openimageio
mingw-w64-breakdown
mingw-w64-natron-setup
mingw-w64-natron-sdk
```
```
cd <package_name>
../makepkg.sh
pacman -U <package(s)>
```
