#!/bin/bash

# Install dbus (for QtDBus)
# see http://www.linuxfromscratch.org/lfs/view/systemd/chapter08/dbus.html
DBUS_VERSION=1.12.18
DBUS_TAR="dbus-${DBUS_VERSION}.tar.gz"
DBUS_SITE="https://dbus.freedesktop.org/releases/dbus"
if download_step; then
    download "$DBUS_SITE" "$DBUS_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/dbus-1.pc" ] || [ "$(pkg-config --modversion dbus-1)" != "$DBUS_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$DBUS_TAR"
    pushd "dbus-${DBUS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-doxygen-docs --disable-xml-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "dbus-${DBUS_VERSION}"
    end_build
fi
