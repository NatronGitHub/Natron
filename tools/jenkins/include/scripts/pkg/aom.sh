#!/bin/bash

# Install aom (Alliance for Open Media AV1)
# see https://aomedia.googlesource.com/aom/
AOM_VERSION=3.15.1
#AOM_TAR="aom-${AOM_VERSION}.tar.gz"
#AOM_SITE=""
AOM_GIT="https://aomedia.googlesource.com/aom.git"
AOM_TAG="v${AOM_VERSION}"
# the commit of the tag (git ls-remote https://aomedia.googlesource.com/aom.git 'refs/tags/v3.15.1^{}'),
# checked after cloning (googlesource archives are not reproducible, so there is no tarball checksum)
AOM_COMMIT=44d0a57786f432d933ff64b653347c66f4d0fa1d
# if download_step; then
#     download "$AOM_SITE" "$AOM_TAR"
# fi
if build_step && { force_build  || { [ ! -s "$SDK_HOME/lib/pkgconfig/aom.pc" ] || [ "$(pkg-config --modversion aom)" != "$AOM_VERSION" ]; }; }; then
    start_build
    # untar "$SRC_PATH/$AOM_TAR"
    # pushd "aom-${AOM_VERSION}"
    git_clone_branch_or_tag "$AOM_GIT" "$AOM_TAG"
    pushd aom
    if [ "$(git rev-parse HEAD)" != "$AOM_COMMIT" ]; then
        (>&2 echo "Error: aom $AOM_TAG is $(git rev-parse HEAD), expected $AOM_COMMIT")
        exit 1
    fi
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
