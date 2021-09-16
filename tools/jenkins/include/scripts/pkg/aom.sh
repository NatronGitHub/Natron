#!/bin/bash

# Install aom (Alliance for Open Media AV1)
# see https://aomedia.googlesource.com/aom/
AOM_VERSION=3.1.2
#AOM_TAR="aom-${AOM_VERSION}.tar.gz"
#AOM_SITE=""
AOM_GIT="https://aomedia.googlesource.com/aom.git"
AOM_COMMIT="v${AOM_VERSION}"
# if download_step; then
#     download "$AOM_SITE" "$AOM_TAR"
# fi
if build_step && { force_build  || { [ ! -s "$SDK_HOME/lib/pkgconfig/aom.pc" ] || [ "$(pkg-config --modversion aom)" != "$AOM_VERSION" ]; }; }; then
    start_build
    # untar "$SRC_PATH/$AOM_TAR"
    # pushd "aom-${AOM_VERSION}"
    git_clone_commit "$AOM_GIT" "$AOM_COMMIT"
    pushd aom
    mkdir _build
    pushd _build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  \
     -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_DOCS=off -DENABLE_EXAMPLES=off \
     -DENABLE_TESTDATA=off -DENABLE_TESTS=off -DENABLE_TOOLS=off -DBUILD_SHARED_LIBS=on
    make -j${MKJOBS}
    make install
    popd
    popd
    # rm -rf "aom-${AOM_VERSION}"
    rm -rf aom
    end_build
fi
