#!/bin/bash

# Install poppler-glib (without curl, nss3, qt4, qt5)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/poppler.html
POPPLER_VERSION=26.02.0
POPPLER_TAR="poppler-${POPPLER_VERSION}.tar.xz"
POPPLER_SITE="https://poppler.freedesktop.org"
if download_step; then
    download "$POPPLER_SITE" "$POPPLER_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_POPPLER:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/include/poppler" "$SDK_HOME/lib/"libpoppler* "$SDK_HOME/lib/pkgconfig/"*poppler* || true 
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/poppler-glib.pc" ] || [ "$(pkg-config --modversion poppler-glib)" != "$POPPLER_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$POPPLER_TAR"
    pushd "poppler-${POPPLER_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-cairo-output --enable-xpdf-headers --enable-libjpeg --enable-zlib --enable-poppler-glib --disable-poppler-qt4 --disable-poppler-qt5 --enable-build-type=release --enable-cmyk --enable-xpdf-headers
    #make -j${MKJOBS}
    #make install
    mkdir build
    pushd build
    # Natron-specific options: disable curl, nss3, qt5, qt4
    # ENABLE_NSS3=OFF does not disable NSS3 if include/library are found,
    # thus NSS3_FOUND=OFF
    natron_options=( -DENABLE_LIBCURL=OFF
                     -DNSS3_FOUND=OFF
                     -DENABLE_NSS3=OFF
                     -DENABLE_QT5=OFF
                     -DENABLE_QT4=OFF
                     -DENABLE_GTK_DOC=OFF
                     -DENABLE_QT6=OFF
                     -DENABLE_GPGME=OFF
                     -DENABLE_BOOST=OFF
                     -DENABLE_GOBJECT_INTROSPECTION=OFF
                     -DBUILD_GTK_TESTS=OFF -DBUILD_QT5_TESTS=OFF -DBUILD_QT6_TESTS=OFF -DBUILD_CPP_TESTS=OFF -DBUILD_MANUAL_TESTS=OFF
                     -DCMAKE_INSTALL_LIBDIR=lib ) # poppler >= 25 needs C++23 (GCC 14)
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_XPDF_HEADERS=ON -DENABLE_UNSTABLE_API_ABI_HEADERS=ON "${natron_options[@]}"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "poppler-${POPPLER_VERSION}"
    end_build
fi
