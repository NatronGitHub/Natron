#!/bin/bash

# Install openexr
# see https://github.com/openexr/openexr/releases/
if [ -z ${EXR_VERSION+x} ]; then
#EXR_VERSION=2.5.7
EXR_VERSION=3.5.4
EXR_CXX11_CONFIGURE_FLAGS=
if version_gt "2.4.0" "${EXR_VERSION}"; then
    EXR_CXX11_CONFIGURE_FLAGS=--enable-cxxstd=11
else
    EXR_CXX11_CONFIGURE_FLAGS=-DCMAKE_CXX_STANDARD=17 # OpenEXR >= 3.3 requires C++17
fi
fi

EXR_TAR="openexr-${EXR_VERSION}.tar.gz"
EXR_SHA256=cedafe3171176d20baea4e887ceacc74cbe3a5a06fbb30d9f0a394a4416e5f7b

if download_step; then
    if version_gt "2.4.0" "${EXR_VERSION}"; then
        download "$EXR_SITE" "$EXR_TAR"
    else
        download_github AcademySoftwareFoundation openexr "${EXR_VERSION}" v "${EXR_TAR}" "$EXR_SHA256"
    fi
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/OpenEXR.pc" ] || [ "$(pkg-config --modversion OpenEXR)" != "$EXR_VERSION" ]; }; }; then    
    start_build
    untar "$SRC_PATH/$EXR_TAR"
    pushd "openexr-${EXR_VERSION}"
    if version_gt "2.4.0" "${EXR_VERSION}"; then
        patch -p0 -i "$INC_PATH/patches/OpenEXR/patch-IlmImf-Makefile.diff"
        env PATH="$SDK_HOME/bin:$PATH" ./bootstrap
        #mkdir build;cd build
        # cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/OpenEXR" -DCMAKE_LIBRARY_PATH="${SDK_HOME}/lib" -DCMAKE_SHARED_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_EXE_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_C_COMPILER="$SDK_HOME/gcc/bin/gcc" -DCMAKE_CXX_COMPILER="${SDK_HOME}/gcc/bin/g++"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --disable-debug --disable-dependency-tracking ${EXR_CXX11_CONFIGURE_FLAGS}
        make -j${MKJOBS}
        make install
    else
        mkdir build
        pushd build
        cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib ${EXR_CXX11_CONFIGURE_FLAGS} -DBUILD_TESTING=OFF -DOPENEXR_BUILD_EXAMPLES=OFF -DOPENEXR_INSTALL_EXAMPLES=OFF -DOPENEXR_BUILD_DOCS=OFF \
              -DOPENEXR_FORCE_INTERNAL_DEFLATE=ON -DOPENEXR_FORCE_INTERNAL_OPENJPH=ON -DOPENEXR_FORCE_INTERNAL_IMATH=OFF -DFETCHCONTENT_FULLY_DISCONNECTED=ON
        # (libdeflate and OpenJPH: the copies vendored in the OpenEXR tarball, never a download or a
        # system library; Imath: the SDK's, see ilmbase.sh)
        # compiling openexr 2.5 requires lots of memory, let's limit to 2 jobs
        j=${MKJOBS}
        if [ $j -gt 2 ]; then
            j=2
        fi
        make -j${j}
        make install
        popd
    fi
    popd
    rm -rf "openexr-${EXR_VERSION}"
    end_build
fi
