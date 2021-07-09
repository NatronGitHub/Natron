#!/bin/bash

# Install openexr
# see https://github.com/openexr/openexr/releases/
# Note: upgrading to 2.4.1-2.5.1 requires rewriting the download and build scripts
# (openexr is now a single package).
EXR_VERSION=2.5.7
EXR_CXX11_CONFIGURE_FLAGS=
if version_gt "2.4.0" "${EXR_VERSION}"; then
    EXR_CXX11_CONFIGURE_FLAGS=--enable-cxxstd=11
else
    EXR_CXX11_CONFIGURE_FLAGS=-DOPENEXR_CXX_STANDARD=11
fi

if version_gt "2.4.0" "${EXR_VERSION}"; then # ilmbase is not a separate build, starting from 2.4.0

EXR_ILM_TAR="ilmbase-${EXR_VERSION}.tar.gz"
EXR_SITE="https://github.com/openexr/openexr/releases/download/v${EXR_VERSION}"

if download_step; then
    download "$EXR_SITE" "$EXR_ILM_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_EXR:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libI* "$SDK_HOME"/lib/libHalf* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{OpenEXR,IlmBase}.pc || true
    rm -f "$SDK_HOME"/bin/exr* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/IlmBase.pc" ] || [ "$(pkg-config --modversion IlmBase)" != "$EXR_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$EXR_ILM_TAR"
    pushd "ilmbase-${EXR_VERSION}"
    if version_gt "2.3.0" "${EXR_VERSION}"; then
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-threadpool_release_lock_single_thread.patch"
	# https://github.com/lgritz/openexr/tree/lg-register
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-register.patch"
	# https://github.com/lgritz/openexr/tree/lg-cpp11
	patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-c++11.patch"
    fi
    #  mkdir build; cd build
    #cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/OpenEXR" -DCMAKE_SHARED_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_C_COMPILER="$SDK_HOME/gcc/bin/gcc" -DCMAKE_CXX_COMPILER="${SDK_HOME}/gcc/bin/g++" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static ${EXR_CXX11_CONFIGURE_FLAGS}
    make -j${MKJOBS}
    make install
    cp IlmBase.pc "$SDK_HOME/lib/pkgconfig/"
    popd
    rm -rf "ilmbase-${EXR_VERSION}"
    end_build
fi

fi