#!/bin/bash

# Install glib
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
# We explicitely disable SElinux, see https://github.com/NatronGitHub/Natron/issues/265
# versions 2.56, 2.58 and 2.60 do not compile on CentOS6.
# This will be fixed in 2.62: https://github.com/GNOME/glib/commit/0beb62f564072f3585762c9c55fe894485993b62#diff-6b790fb09bbee6ca6c8ee1a76c0f49be
GLIB_VERSION=2.54.3 
#if grep -F F_SETPIPE_SZ /usr/include/linux/fcntl.h &>/dev/null; then
#    GLIB_VERSION=2.56.1
#fi
GLIB_VERSION_SHORT=${GLIB_VERSION%.*}
GLIB_TAR="glib-${GLIB_VERSION}.tar.xz"
GLIB_SITE="https://ftp.gnome.org/pub/gnome/sources/glib/${GLIB_VERSION_SHORT}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/glib-2.0.pc" ] || [ "$(pkg-config --modversion glib-2.0)" != "$GLIB_VERSION" ]; }; }; then
    start_build
    download "$GLIB_SITE" "$GLIB_TAR"
    untar "$SRC_PATH/$GLIB_TAR"
    pushd "glib-${GLIB_VERSION}"
    # apply patches from http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
    patch -Np1 -i "$INC_PATH/patches/glib/glib-2.56.0-skip_warnings-1.patch"
    # do not apply: we do not build with meson yet
    #patch -Np1 -i "$INC_PATH/patches/glib/glib-2.54.2-meson_fixes-1.patch"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-selinux --disable-gtk-doc-html --disable-static --enable-shared --with-pcre=system
    make -j${MKJOBS}
    make install
    popd
    rm -rf "glib-${GLIB_VERSION}"
    end_build
fi
