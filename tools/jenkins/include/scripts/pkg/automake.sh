#!/bin/bash

# Install automake
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/automake.html
AUTOMAKE_VERSION=1.16.5
AUTOMAKE_TAR="automake-${AUTOMAKE_VERSION}.tar.xz"
AUTOMAKE_SITE="https://ftp.gnu.org/gnu/automake"
if download_step; then
    download "$AUTOMAKE_SITE" "$AUTOMAKE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/automake" ] || [ "$("$SDK_HOME/bin/automake" --version | head -1 | awk '{print $4}')" != "$AUTOMAKE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$AUTOMAKE_TAR"
    pushd "automake-${AUTOMAKE_VERSION}"
    # The following patch is necessary if an old perl (<=5.6) is installed.
    # patch -p1 -i $INC_PATH/patches/automake/0001-automake-Don-t-rely-on-List-Util-to-provide-none.patch
    # autoreconf -fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "automake-${AUTOMAKE_VERSION}"
    end_build
fi
