#!/bin/bash

# Install Qt5
# see https://www.linuxfromscratch.org/blfs/view/svn/x/qt5.html
# Note: This is excluding qtwebengine, which is available separately (for security reasons):
# https://www.linuxfromscratch.org/blfs/view/svn/x/qtwebengine.html
QT5_VERSION=5.15.2
QT5_VERSION_SHORT=${QT5_VERSION%.*}
QT5_TAR="qt-everywhere-src-${QT5_VERSION}.tar.xz"
QT5_SITE=" https://download.qt.io/archive/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}/single"

if download_step; then
    download "$QT5_SITE" "$QT5_TAR"
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
    # First apply a patch to fix an Out Of Bounds read in QtSVG: 
    patch -Np1 -i "$INC_PATH"/patches/Qt/qt-everywhere-src-5.15.2-CVE-2021-3481-1.patch
    # Next fix some issues using gcc-11:
    sed -i '/utility/a #include <limits>'     qtbase/src/corelib/global/qglobal.h         &&
    sed -i '/string/a #include <limits>'      qtbase/src/corelib/global/qfloat16.h        &&
    sed -i '/qbytearray/a #include <limits>'  qtbase/src/corelib/text/qbytearraymatcher.h &&
    sed -i '/type_traits/a #include <limits>' qtdeclarative/src/qmldebug/qqmlprofilerevent_p.h
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
