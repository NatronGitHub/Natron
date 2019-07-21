#!/bin/bash

# Install cmake
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/cmake.html
#
# cmake 3.10.1 does not compile with the system libuv
# Utilities/cmlibuv/src/unix/posix-poll.c: In function 'uv__platform_loop_init':
# Utilities/cmlibuv/src/unix/posix-poll.c:37:7: error: 'uv_loop_t {aka struct uv_loop_s}' has no member named 'poll_fds'
#   loop->poll_fds = NULL;
#       ^~
# Since there is no way to disable libuv alone (--no-system-libuv unavailable in ./bootstrap),
# we disable all system libs. Voilà!
CMAKE_VERSION=3.15.0
CMAKE_VERSION_SHORT=${CMAKE_VERSION%.*}
CMAKE_TAR="cmake-${CMAKE_VERSION}.tar.gz"
CMAKE_SITE="https://cmake.org/files/v${CMAKE_VERSION_SHORT}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/cmake" ] || [ $("$SDK_HOME/bin/cmake" --version | head -1 |awk '{print $3}') != "$CMAKE_VERSION" ]; }; }; then
    start_build
    download "$CMAKE_SITE" "$CMAKE_TAR"
    untar "$SRC_PATH/$CMAKE_TAR"
    pushd "cmake-${CMAKE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --no-system-libs --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cmake-${CMAKE_VERSION}"
    end_build
fi
