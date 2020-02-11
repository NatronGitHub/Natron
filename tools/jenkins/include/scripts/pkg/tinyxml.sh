#!/bin/bash

# Install tinyxml (used by OpenColorIO)
# see https://sourceforge.net/projects/tinyxml/files/tinyxml
TINYXML_VERSION=2.6.2
TINYXML_TAR="tinyxml_${TINYXML_VERSION//./_}.tar.gz"
TINYXML_SITE="http://sourceforge.net/projects/tinyxml/files/tinyxml/${TINYXML_VERSION}"
if download_step; then
    download "$TINYXML_SITE" "$TINYXML_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/tinyxml.pc" ] || [ "$(pkg-config --modversion tinyxml)" != "$TINYXML_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$TINYXML_TAR"
    pushd "tinyxml"
    # The first two patches are taken from the debian packaging of tinyxml.
    #   The first patch enforces use of stl strings, rather than a custom string type.
    #   The second patch is a fix for incorrect encoding of elements with special characters
    #   originally posted at https://sourceforge.net/p/tinyxml/patches/51/
    # The third patch adds a CMakeLists.txt file to build a shared library and provide an install target
    #   submitted upstream as https://sourceforge.net/p/tinyxml/patches/66/
    patch -Np1 -i "$INC_PATH/patches/tinyxml/enforce-use-stl.patch"
    patch -Np1 -i "$INC_PATH/patches/tinyxml/entity-encoding.patch"
    patch -Np1 -i "$INC_PATH/patches/tinyxml/tinyxml_CMakeLists.patch"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make install
    if [ ! -d "$SDK_HOME/lib/pkgconfig" ]; then
        mkdir -p "$SDK_HOME/lib/pkgconfig"
    fi
    cat > "$SDK_HOME/lib/pkgconfig/tinyxml.pc" <<EOF
prefix=${SDK_HOME}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: TinyXml
Description: Simple, small, C++ XML parser
Version: ${TINYXML_VERSION}
Libs: -L\${libdir} -ltinyxml
Cflags: -I\${includedir}
EOF
    popd
    popd
    rm -rf "tinyxml"
    end_build
fi
