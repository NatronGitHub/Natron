#!/bin/bash

# Install libxml2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libxml2.html
LIBXML2_VERSION=2.10.4
LIBXML2_VERSION_SHORT=${LIBXML2_VERSION%.*}

LIBXML2_TAR="libxml2-${LIBXML2_VERSION}.tar.xz"
LIBXML2_SITE="https://download.gnome.org/sources/libxml2/${LIBXML2_VERSION_SHORT}"
LIBXML2_ICU=1 # set to 1 if libxml2 should be compiled with ICU support. This implies things for Qt4 and QtWebkit.
if download_step; then
    download "$LIBXML2_SITE" "$LIBXML2_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_XML:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/libxml2 "$SDK_HOME"/lib/libxml* "$SDK_HOME"/lib/pkgconfig/libxml* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libxml-2.0.pc" ] || [ "$(pkg-config --modversion libxml-2.0)" != "$LIBXML2_VERSION" ]; }; }; then
    REBUILD_XSLT=1
    start_build
    untar "$SRC_PATH/$LIBXML2_TAR"
    pushd "libxml2-${LIBXML2_VERSION}"
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        icu_flag=--with-icu
    else
        icu_flag=--without-icu
    fi
    #  First fix a problem generating the Python3 module with Python-3.9.0 and later: 
    $GSED -i '/if Py/{s/Py/(Py/;s/)/))/}' python/{types.c,libxml.c}
    #  To ensure that the Python module can be built by Python-3.9.0, run: 
    $GSED -i '/_PyVerify_fd/,+1d' python/types.c
    #  If you are going to run the tests, disable one test that prevents the tests from completing: 
    $GSED -i 's/test.test/#&/' python/tests/tstLastError.py
    # If, and only if, you are using ICU-68.1, fix a build breakage caused by that version by running the following command: 
    # see https://www.mail-archive.com/blfs-dev@lists.linuxfromscratch.org/msg10387.html
    if [ "${ICU_VERSION:-}" = 68.1 ] || [ "${ICU_VERSION:-}" = 68.2 ]; then
        $GSED -i 's/ TRUE/ true/' encoding.c
    fi
    # note: python3 module is necessary for itstool
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-history $icu_flag PYTHON="$SDK_HOME/bin/python3" --with-lzma --with-threads
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libxml2-${LIBXML2_VERSION}"
    end_build
fi
