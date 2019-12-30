# Natron Windows SDK (DRAFT)

The following are instructions for installing the Natron SDK on Windows. Please follow the instructions exactly as described.

## Install MSYS2

First you need to download a MSYS2 snapshot, only ``20180531`` is supported.

* Download and extract http://repo.msys2.org/distrib/x86_64/msys2-base-x86_64-20180531.tar.xz, then move the ``msys64`` folder to ``C:\msys64-20180531``
* Run ``C:\msys64-20180531\mingw64.exe``
* When prompted close the terminal

Now start ``C:\msys64-20180531\mingw64.exe`` again and run the following command:

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

Note that some packages might not be available from MSYS2 when you read this, you can download all the packages in one archive here (TODO, ADD LINK) then copy them to ``/var/cache/pacman/pkg`` and run the command above again.

## Additional packages

Several packages needs to be added or updated. You can build them yourself or install from binary packages.

## Install

TODO link to packages here.

## Build

You will need to build and install the following packages (in the correct order):

```
CWD=`pwd`
PKGS="
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
mingw-w64-libraw-gpl2
mingw-w64-ffmpeg-gpl2
mingw-w64-imagemagick
mingw-w64-libarchive
mingw-w64-cmake
mingw-w64-poppler
mingw-w64-opencolorio-git
mingw-w64-openimageio2
mingw-w64-python2-sphinx
mingw-w64-python-sphinx_rtd_theme
mingw-w64-python-sphinx-alabaster-theme
mingw-w64-qt4
mingw-w64-seexpr-git
mingw-w64-shiboken-qt4
mingw-w64-pyside-qt4
"
for pkg in $PKGS; do
cd $CWD/$pkg
sh $CWD/makepkg.sh
pacman -U mingw-w64-*pkg.tar.xz
done
```
