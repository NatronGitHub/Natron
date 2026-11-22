#!/bin/bash

# Install Qt5
# see https://www.linuxfromscratch.org/blfs/view/svn/x/qt5.html
# Note: This is excluding qtwebengine, which is available separately (for security reasons):
# https://www.linuxfromscratch.org/blfs/view/svn/x/qtwebengine.html
QT5_VERSION=5.15.19
QT5_VERSION_SHORT=${QT5_VERSION%.*}
QT5_TAR="qt-everywhere-opensource-src-${QT5_VERSION}.tar.xz"
QT5_SHA256=173c2326dae138bbb0d98921e9d911e55c00163d93a6db29f294b5e19ff306ae
QT5_SITE="https://download.qt.io/archive/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}/single"
# Until 5.15.9, the open-source Qt 5.15 releases lagged behind the commercial ones, and
# the BLFS aggregation of the KDE Qt5 patch collection was applied
# (qt-everywhere-opensource-src-5.15.9-kf5-1.patch, see
# https://community.kde.org/Qt5PatchCollection). 5.15.19 is the last open-source 5.15
# release: it contains those fixes, and BLFS has no patch for it.
QT5_PATCH=
QT5_PATCH_SITE="https://www.linuxfromscratch.org/patches/blfs/svn"

if download_step; then
    download "$QT5_SITE" "$QT5_TAR" "$QT5_SHA256"
    if [ -n "$QT5_PATCH" ]; then
        download "$QT5_PATCH_SITE" "$QT5_PATCH"
    fi
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
    if [ -n "$QT5_PATCH" ]; then
        patch -Np1 -i "$SRC_PATH/$QT5_PATCH"
    fi
    mkdir -pv qtbase/.git
    # -no-pch: the precompiled headers use several GB of disk space (the 40 GB podman VM was full)
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
            -nomake tests                             \
            -no-pch                                   \
            -no-rpath                                 \
            -syslog                                   \
            -xcb                                      \
            -xcb-xlib                                 \
            -xkbcommon                                \
            -skip qtwebengine                         \
            -skip qtlocation
    make -j${MKJOBS}
    make install
    # The X11 platform plug-in, which Natron needs: -xcb makes configure fail if its
    # dependencies are missing, but check that it was built and installed.
    if [ ! -s "$QT5PREFIX/plugins/platforms/libqxcb.so" ] || [ ! -s "$QT5PREFIX/lib/libQt5XcbQpa.so.5" ]; then
        (>&2 echo "Error: the Qt5 xcb platform plug-in ($QT5PREFIX/plugins/platforms/libqxcb.so) was not built.")
        exit 1
    fi
    # Remove references to the build directory from installed library dependency (prl) files by running the following command as the root user: 
    find $QT5PREFIX/ -name \*.prl -exec sed -i -e '/^QMAKE_PRL_BUILD_DIR/d' {} \;
    popd
    rm -rf "qt-everywhere-src-${QT5_VERSION}"
    end_build
fi
