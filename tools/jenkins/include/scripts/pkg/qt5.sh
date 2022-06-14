#!/bin/bash

# Install Qt5
# see https://www.linuxfromscratch.org/blfs/view/svn/x/qt5.html
# Note: This is excluding qtwebengine, which is available separately (for security reasons):
# https://www.linuxfromscratch.org/blfs/view/svn/x/qtwebengine.html
QT5_VERSION=5.15.4
QT5_VERSION_SHORT=${QT5_VERSION%.*}
QT5_TAR="qt-everywhere-opensource-src-${QT5_VERSION}.tar.xz"
QT5_SITE=" https://download.qt.io/archive/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}/single"
#  Now that Qt5 updates are restricted to commercial customers, upstream patches for the
# various modules are being curated at kde. Patches for the modules required by packages
# in BLFS have been aggregated for the non-modular Qt5 build we use.
# Required patch: https://www.linuxfromscratch.org/patches/blfs/svn/qt-everywhere-src-5.15.2-kf5.15-2.patch
# Details of the kde curation can be found at https://dot.kde.org/2021/04/06/announcing-kdes-qt-5-patch-collection
# and https://community.kde.org/Qt5PatchCollection.
QT5_PATCH="qt-everywhere-opensource-src-5.15.4-kf5-1.patch"
QT5_PATCH_SITE="https://www.linuxfromscratch.org/patches/blfs/svn"

if download_step; then
    download "$QT5_SITE" "$QT5_TAR"
    download "$QT5_PATCH_SITE" "$QT5_PATCH"
fi
if build_step && { force_build || { [ "${REBUILD_QT5:-}" = "1" ]; }; }; then
    rm -rf "$QT5PREFIX"
fi
if dobuild; then
    export QTDIR="$QT5PREFIX"
fi
if build_step && { force_build || { [ ! -s "$QT5PREFIX/lib/pkgconfig/Qt5Core.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Qt5Core)" != "$QT5_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$QT5_TAR"
    pushd "qt-everywhere-src-${QT5_VERSION}"
    patch -Np1 -i "$SRC_PATH/$QT5_PATCH"
    # Install Qt5 by running the following commands: 
    env CFLAGS="$BF" CXXFLAGS="$BF" OPENSSL_LIBS="-L$SDK_HOME/lib -lssl -lcrypto" ./configure -prefix "$QT5PREFIX" \
            -sysconfdir /etc/xdg                      \
            -confirm-license                          \
            -opensource                               \
            -dbus-linked                              \
            -openssl-linked                           \
            -system-harfbuzz                          \
            -system-sqlite                            \
            -nomake examples                          \
            -no-rpath                                 \
            -syslog                                   \
            -skip qtwebengine
    make -j${MKJOBS}
    make install
    # Remove references to the build directory from installed library dependency (prl) files by running the following command as the root user: 
    find $QT5PREFIX/ -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;
    rm -rf "qt-everywhere-src-${QT5_VERSION}"
    end_build
fi
