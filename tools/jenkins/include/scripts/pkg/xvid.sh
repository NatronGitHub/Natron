#!/bin/bash

# xvid
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/xvid.html
XVID_VERSION=1.3.6
XVID_TAR="xvidcore-${XVID_VERSION}.tar.gz"
XVID_SITE="http://downloads.xvid.com/downloads"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libxvidcore.so" ]; }; }; then
    start_build
    download "$XVID_SITE" "$XVID_TAR"
    untar "$SRC_PATH/$XVID_TAR"
    pushd xvidcore/build/generic
    # Fix error during make install if reintalling or upgrading. 
    sed -i 's/^LN_S=@LN_S@/& -f -v/' platform.inc.in
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    # This command disables installing the static library. 
    sed -i '/libdir.*STATIC_LIB/ s/^/#/' Makefile
    make install
    chmod +x "$SDK_HOME/lib/libxvidcore.so".*.*
    popd
    rm -rf xvidcore
    end_build
fi
