#!/bin/bash
# install libmad (for sox)
# The original libmad (0.15.1b, 2004) is no longer maintained: this is the Tenacity team's
# continuation (CMake, same API and soname libmad.so.0), as in MacPorts.
# see https://codeberg.org/tenacityteam/libmad/tags
LIBMAD_VERSION=0.16.5
LIBMAD_TAR="libmad-${LIBMAD_VERSION}.tar.gz"
# the Codeberg source archive of the tag, also on MacPorts' distfiles mirror (the same file)
LIBMAD_SHA256=e32320a010e3cc00bfa66e0cde523a969313cddfd81e07ec4e3ef06d10ad3fe0
if download_step; then
    download_urls "$LIBMAD_TAR" "$LIBMAD_SHA256" \
        "https://codeberg.org/tenacityteam/libmad/archive/${LIBMAD_VERSION}.tar.gz" \
        "https://distfiles.macports.org/libmad/${LIBMAD_VERSION}.tar.gz"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/mad.pc" ] || [ "$(pkg-config --modversion mad)" != "$LIBMAD_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$LIBMAD_TAR"
    pushd libmad # the archive's top directory has no version
    # CMAKE_INSTALL_LIBDIR: GNUInstallDirs would pick lib64 on Rocky Linux
    cmake -S . -B build -DCMAKE_C_FLAGS="$BF" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_INSTALL_LIBDIR=lib -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_SHARED_LIBS=ON -DEXAMPLE=OFF
    cmake --build build -j${MKJOBS}
    # (it also installs libmad.a, which its CMake config refers to; sox links libmad.so)
    cmake --install build
    popd
    rm -rf libmad
    end_build
fi
