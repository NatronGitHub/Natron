#!/bin/bash

# install flex (for SeExpr)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/flex.html
FLEX_VERSION=2.6.4
FLEX_TAR="flex-${FLEX_VERSION}.tar.gz"
FLEX_SITE="https://github.com/westes/flex/releases/download/v${FLEX_VERSION}"
if download_step; then
    download "$FLEX_SITE" "$FLEX_TAR"
    #download_github westes flex "${FLEX_VERSION}" v "${FLEX_TAR}"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/flex" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FLEX_TAR"
    pushd "flex-${FLEX_VERSION}"
    # First, fix a problem introduced with glibc-2.26:
    sed -i "/math.h/a #include <malloc.h>" src/flexdef.h
    HELP2MAN=true ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "flex-${FLEX_VERSION}"
    end_build
fi
