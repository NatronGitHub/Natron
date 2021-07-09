#!/bin/bash

# Install libmount (required by glib)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/util-linux.html

#if [ "${RHEL_MAJOR:-7}" -le 6 ]; then
#    # 2.31.1 fails to compile on centos 6
#    # this was fixed in 2.32
#    # see https://www.spinics.net/lists/util-linux-ng/msg15044.html
#    # and https://www.spinics.net/lists/util-linux-ng/msg13963.html
#    UTILLINUX_VERSION=2.30.2
#else
#    UTILLINUX_VERSION=2.31.1
#fi
UTILLINUX_VERSION=2.37
UTILLINUX_VERSION_NOZERO=${UTILLINUX_VERSION} # eg 2.36
UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION}  # eg 2.36
# if has minor version, uncomment the proper macros below
#UTILLINUX_VERSION_NOZERO=${UTILLINUX_VERSION%.0} # eg 2.35.2
#UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION%.*}  # eg 2.35.2
if [ "${CENTOS:-0}" = 6 ]; then
    UTILLINUX_VERSION=2.35.2 # 2.36 fails on CentOS6 with:
    # sys-utils/unshare.c:552:21: error: 'CLOCK_BOOTTIME' undeclared (first use in this function); did you mean 'OPT_BOOTTIME'?
    #UTILLINUX_VERSION_NOZERO=${UTILLINUX_VERSION} # eg 2.36
    #UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION}  # eg 2.36
    # if has minor version, uncomment the proper macros below
    UTILLINUX_VERSION_NOZERO=${UTILLINUX_VERSION%.0} # eg 2.35.2
    UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION%.*}  # eg 2.35.2
fi
UTILLINUX_TAR="util-linux-${UTILLINUX_VERSION_NOZERO}.tar.xz"
UTILLINUX_SITE="https://www.kernel.org/pub/linux/utils/util-linux/v${UTILLINUX_VERSION_SHORT}"
if download_step; then
    download "$UTILLINUX_SITE" "$UTILLINUX_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/mount.pc" ] || [ "$(pkg-config --modversion mount)" != "$UTILLINUX_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$UTILLINUX_TAR"
    pushd "util-linux-${UTILLINUX_VERSION_NOZERO}"
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
    rm -rf "util-linux-${UTILLINUX_VERSION_NOZERO}"
    end_build
fi
