#!/bin/bash

# Install OpenColorIO (uses yaml-cpp, tinyxml, lcms)
# see https://github.com/imageworks/OpenColorIO/releases
# We build a version more recent than 1.0.9, because 1.0.9 only supports yaml-cpp 0.3 and we
# installed yaml-cpp 0.5.
OCIO_BUILD_GIT=0 # set to 1 to build the git version instead of the release
# non-GIT version:
OCIO_VERSION=1.1.1
OCIO_TAR="OpenColorIO-${OCIO_VERSION}.tar.gz"
if download_step; then
    download_github imageworks OpenColorIO "$OCIO_VERSION" v "$OCIO_TAR"
fi
# if [ "$OCIO_BUILD_GIT" = 1 ]; then
#     ## GIT version:
#     OCIO_GIT="https://github.com/imageworks/OpenColorIO.git"
#     ## 7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa is October 25, 2017
#     OCIO_COMMIT="7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa"
# fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/OpenColorIO.pc" ] || [ "$(pkg-config --modversion OpenColorIO)" != "$OCIO_VERSION" ]; }; }; then
    start_build
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        git_clone_commit "$OCIO_GIT" "$OCIO_COMMIT"
        pushd OpenColorIO
    else
        untar "$SRC_PATH/$OCIO_TAR"
        pushd "OpenColorIO-${OCIO_VERSION}"
    fi
    # don't treat warnings as error (this should be for OCIO developers only)
    find . -name CMakeLists.txt -exec sed -e s/-Werror// -i {} \;
    if version_gt 0.7.0 "$YAMLCPP_VERSION"; then
	    # Patch issue with yaml-cpp 0.6 https://bugs.gentoo.org/651970
	    patch -Np1 -i "$INC_PATH"/patches/oopencolorio/opencolorio-yaml-cpp-0.6.patch
    fi
    # fix for compatibility with yaml-cpp 0.7.0
    $GSED -i 's/if(node\["ocio_profile_version"\] == NULL)/if(!node["ocio_profile_version"])/' src/core/OCIOYaml.cpp
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"   -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DOCIO_BUILD_JNIGLUE=OFF -DOCIO_BUILD_NUKE=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_STATIC_JNIGLUE=OFF -DOCIO_BUILD_TRUELIGHT=OFF -DUSE_EXTERNAL_LCMS=ON -DUSE_EXTERNAL_TINYXML=ON -DUSE_EXTERNAL_YAML=ON -DOCIO_BUILD_APPS=OFF -DOCIO_USE_BOOST_PTR=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        rm -rf OpenColorIO
    else
        rm -rf "OpenColorIO-${OCIO_VERSION}"
    fi
    end_build
fi
