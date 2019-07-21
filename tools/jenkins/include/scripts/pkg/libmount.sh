#!/bin/bash

# Install libmount (required by glib)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/util-linux.html

#if [ "${RHEL_MAJOR:-7}" -le 6 ]; then
#    # 2.31.1 fails to compile on centos 6
#    # see https://www.spinics.net/lists/util-linux-ng/msg15044.html
#    # and https://www.spinics.net/lists/util-linux-ng/msg13963.html
#    UTILLINUX_VERSION=2.30.2
#else
#    UTILLINUX_VERSION=2.31.1
#fi
UTILLINUX_VERSION=2.34.0 # this was fixed in 2.32
UTILLINUX_VERSION_NOZERO=${UTILLINUX_VERSION%.0}
UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION%.*}
UTILLINUX_TAR="util-linux-${UTILLINUX_VERSION_NOZERO}.tar.xz"
UTILLINUX_SITE="https://www.kernel.org/pub/linux/utils/util-linux/v${UTILLINUX_VERSION_SHORT}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/mount.pc" ] || [ "$(pkg-config --modversion mount)" != "$UTILLINUX_VERSION" ]; }; }; then
    start_build
    download "$UTILLINUX_SITE" "$UTILLINUX_TAR"
    untar "$SRC_PATH/$UTILLINUX_TAR"
    pushd "util-linux-${UTILLINUX_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-chfn-chsh  \
            --disable-login      \
            --disable-nologin    \
            --disable-su         \
            --disable-setpriv    \
            --disable-runuser    \
            --disable-pylibmount \
            --disable-static     \
            --without-python     \
            --without-systemd    \
            --without-systemdsystemunitdir \
            --disable-wall \
            --disable-makeinstall-chown \
            --disable-makeinstall-setuid
    make -j${MKJOBS}
    make install
    popd
    rm -rf "util-linux-${UTILLINUX_VERSION}"
    end_build
fi
