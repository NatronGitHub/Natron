#!/bin/bash

# Install git (requires curl and pcre)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/git.html
GIT_VERSION=2.56.1
GIT_TAR="git-${GIT_VERSION}.tar.xz"
GIT_SHA256=f0b0bf89264b9bd2e1c640363e7529643989da76281600473c50745cd34a607f
GIT_SITE="https://www.kernel.org/pub/software/scm/git"
if download_step; then
    download "$GIT_SITE" "$GIT_TAR" "$GIT_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/git" ] || [ "$("${SDK_HOME}/bin/git" --version)" != "git version $GIT_VERSION" ] ; }; }; then
    start_build
    untar "$SRC_PATH/$GIT_TAR"
    pushd "git-${GIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "git-${GIT_VERSION}"
    end_build
fi
