#!/bin/bash

# Install glib
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
# We explicitly disable SElinux, see https://github.com/NatronGitHub/Natron/issues/265
# versions after 2.54.3 and before 2.56.2 do not compile on CentOS6.
# https://github.com/GNOME/glib/commit/0beb62f564072f3585762c9c55fe894485993b62#diff-6b790fb09bbee6ca6c8ee1a76c0f49be
#GLIB_VERSION=2.54.3 # last version before 2.56
GLIB_VERSION=2.58.3 # last version before meson
#GLIB_VERSION=2.66.7 # requires meson, but also a recent version of binutils
#if grep -F F_SETPIPE_SZ /usr/include/linux/fcntl.h &>/dev/null; then
#    GLIB_VERSION=2.56.1
#fi
GLIB_VERSION_SHORT=${GLIB_VERSION%.*}
GLIB_TAR="glib-${GLIB_VERSION}.tar.xz"
GLIB_SITE="https://ftp.gnome.org/pub/gnome/sources/glib/${GLIB_VERSION_SHORT}"
if download_step; then
    download "$GLIB_SITE" "$GLIB_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/glib-2.0.pc" ] || [ "$(pkg-config --modversion glib-2.0)" != "$GLIB_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GLIB_TAR"
    pushd "glib-${GLIB_VERSION}"
    # apply patches from http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
    if version_gt "$GLIB_VERSION" 2.59.0; then
	#download "http://www.linuxfromscratch.org/patches/blfs/svn" "glib-2.60.2-skip_warnings-1.patch"
	#patch -Np1 -i "$SRC_PATH/glib-2.60.2-skip_warnings-1.patch"
	patch -Np1 -i "$INC_PATH/patches/glib/glib-2.60.2-skip_warnings-1.patch"
	mkdir build
	cd    build
	meson --prefix=/usr      \
	      -Dman=true         \
	      -Dselinux=disabled \
	      ..
	ninja
    else
	patch -Np1 -i "$INC_PATH/patches/glib/glib-2.56.0-skip_warnings-1.patch"
	# do not apply: we do not build with meson yet
	#patch -Np1 -i "$INC_PATH/patches/glib/glib-2.54.2-meson_fixes-1.patch"
	env CFLAGS="$BF" CXXFLAGS="$BF" ./autogen.sh --prefix="$SDK_HOME" --disable-selinux --disable-gtk-doc-html --disable-static --enable-shared --with-pcre=system
    fi
    make -j${MKJOBS}
    make install
    popd
    rm -rf "glib-${GLIB_VERSION}"
    end_build
fi
