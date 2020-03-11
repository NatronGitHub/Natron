#!/bin/bash

# x265
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x265.html
# and https://git.archlinux.org/svntogit/packages.git/tree/trunk/PKGBUILD?h=packages/x265
X265_VERSION=3.3
X265_TAR="x265_${X265_VERSION}.tar.gz"
X265_SITE="https://bitbucket.org/multicoreware/x265/downloads"
if download_step; then
    download "$X265_SITE" "$X265_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_X265:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libx265* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x265* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/x265.pc" ] || [ "$(pkg-config --modversion x265)" != "$X265_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$X265_TAR"
    pushd "x265_$X265_VERSION"
    for d in 8 $([[ "$ARCH" == "x86_64" ]] && echo "10 12"); do
        if [[ -d build-$d ]]; then
            rm -rf build-$d
        fi
        mkdir build-$d
    done
    if [[ "$ARCH" == "x86_64" ]]; then
        pushd build-12
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DHIGH_BIT_DEPTH='TRUE' \
              -DMAIN12='TRUE' \
              -DEXPORT_C_API='FALSE' \
              -DENABLE_CLI='FALSE' \
              -DENABLE_SHARED='FALSE'
        make -j${MKJOBS}
        popd
        
        pushd build-10
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DHIGH_BIT_DEPTH='TRUE' \
              -DEXPORT_C_API='FALSE' \
              -DENABLE_CLI='FALSE' \
              -DENABLE_SHARED='FALSE'
        make -j${MKJOBS}
        popd
        
        pushd build-8

        ln -sfv ../build-10/libx265.a libx265_main10.a
        ln -sfv ../build-12/libx265.a libx265_main12.a

        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DENABLE_SHARED='TRUE' \
              -DENABLE_HDR10_PLUS='TRUE' \
              -DEXTRA_LIB='x265_main10.a;x265_main12.a' \
              -DEXTRA_LINK_FLAGS='-L.' \
              -DLINKED_10BIT='TRUE' \
              -DLINKED_12BIT='TRUE'
        make -j${MKJOBS}
        # rename the 8bit library, then combine all three into libx265.a
        mv libx265.a libx265_main.a
        # On Linux, we use GNU ar to combine the static libraries together
        ar -M <<EOF
CREATE libx265.a
ADDLIB libx265_main.a
ADDLIB libx265_main10.a
ADDLIB libx265_main12.a
SAVE
END
EOF
        # Mac/BSD libtool
        #libtool -static -o libx265.a libx265_main.a libx265_main10.a libx265_main12.a 2>/dev/null
        popd
    else
        pushd build-8
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DENABLE_SHARED='TRUE'
        make -j${MKJOBS}
        popd
    fi
    pushd build-8
    make install
    popd
    popd
    rm -rf "x265_$X265_VERSION"
    end_build
fi
