#!/bin/bash

# Install glew
# see http://www.linuxfromscratch.org/blfs/view/systemd/x/glew.html
GLEW_VERSION=2.2.0
GLEW_TAR="glew-${GLEW_VERSION}.tgz"
GLEW_SITE="https://sourceforge.net/projects/glew/files/glew/${GLEW_VERSION}"
if download_step; then
    download "$GLEW_SITE" "$GLEW_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/glew.pc" ] || [ "$(pkg-config --modversion glew)" != "$GLEW_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GLEW_TAR"
    pushd "glew-${GLEW_VERSION}"
    if [ "$ARCH" = "i686" ]; then
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -march=i686 -mtune=i686' includedir=/usr/include GLEW_DEST= libdir=/usr/lib bindir=/usr/bin
    else
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -m64 -fPIC -mtune=generic' includedir=/usr/include GLEW_DEST= libdir=/usr/lib64 bindir=/usr/bin
    fi
    make install GLEW_DEST="$SDK_HOME" libdir=/lib bindir=/bin includedir=/include
    chmod +x "$SDK_HOME/lib/libGLEW.so.$GLEW_VERSION"
    popd
    rm -rf "glew-${GLEW_VERSION}"
    end_build
fi
