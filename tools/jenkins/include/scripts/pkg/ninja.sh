#!/bin/bash

# Install ninja
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/ninja.html

NINJA_VERSION=1.10.2
NINJA_TAR="ninja-${NINJA_VERSION}.tar.gz"
NINJA_SITE="https://github.com/ninja-build/ninja/archive/v${NINJA_VERSION}"
if download_step; then
    download "$NINJA_SITE" "$NINJA_TAR"
fi
if build_step && { force_build || { [ ! -x "$SDK_HOME/bin/ninja" ]; }; }; then
    start_build
    untar "$SRC_PATH/$NINJA_TAR"
    pushd "ninja-${NINJA_VERSION}"
    python3 configure.py --bootstrap
    install -vm755 ninja "$SDK_HOME/bin/"
    #install -vDm644 misc/bash-completion "$SDK_HOME/share/bash-completion/completions/ninja"
    #install -vDm644 misc/zsh-completion  "$SDK_HOME/share/zsh/site-functions/_ninja"
    popd
    rm -rf "ninja-${NINJA_VERSION}"
    end_build
fi
