#!/bin/bash

# Install dbus (for QtDBus)
# see https://www.linuxfromscratch.org/blfs/view/systemd/general/dbus.html
DBUS_VERSION=1.16.2
DBUS_TAR="dbus-${DBUS_VERSION}.tar.xz"
DBUS_SITE="https://dbus.freedesktop.org/releases/dbus"
if download_step; then
    download "$DBUS_SITE" "$DBUS_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/dbus-1.pc" ] || [ "$(pkg-config --modversion dbus-1)" != "$DBUS_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$DBUS_TAR"
    pushd "dbus-${DBUS_VERSION}"
    if [ -x configure ]; then
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-doxygen-docs --disable-xml-docs --disable-static --enable-shared
        make -j${MKJOBS}
        make install
    else
        # dbus >= 1.15 only builds with meson (or CMake)
        env CFLAGS="$BF" CXXFLAGS="$BF" meson setup build --prefix="$SDK_HOME" --libdir=lib --buildtype=release \
            -Ddoxygen_docs=disabled -Dducktype_docs=disabled -Dxml_docs=disabled -Dqt_help=disabled \
            -Dmodular_tests=disabled -Dinstalled_tests=false -Dsystemd=disabled -Dx11_autolaunch=disabled \
            -Dselinux=disabled -Dapparmor=disabled -Dlibaudit=disabled -Dvalgrind=disabled
        ninja -C build -j${MKJOBS}
        ninja -C build install
    fi
    popd
    rm -rf "dbus-${DBUS_VERSION}"
    end_build
fi
