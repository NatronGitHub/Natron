#!/bin/bash

# install bison (for SeExpr)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/bison.html
BISON_VERSION=3.5.3
BISON_TAR="bison-${BISON_VERSION}.tar.gz"
BISON_SITE="http://ftp.gnu.org/pub/gnu/bison"
if download_step; then
    download "$BISON_SITE" "$BISON_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/bison" ]; }; }; then
    start_build
    untar "$SRC_PATH/$BISON_TAR"
    pushd "bison-${BISON_VERSION}"
    # 2 patches for use with glibc 2.28 and up. (for bison 3.0.4)
    if version_gt 3.1.0 "$BISON_VERSION"; then
	patch -Np1 -i "$INC_PATH"/patches/bison/0001-fflush-adjust-to-glibc-2.28-libio.h-removal.patch
	patch -Np1 -i "$INC_PATH"/patches/bison/0002-fflush-be-more-paranoid-about-libio.h-change.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    # parallel build sometimes dies with:
    # make[1]: Entering directory `/tmp/bison-3.4.1'
    # make[1]: Entering directory `/tmp/bison-3.4.1'
    #   LEX      examples/c/reccalc/scan.stamp
    #   LEX      examples/c/reccalc/scan.stamp
    # make[1]: Leaving directory `/tmp/bison-3.4.1'
    # mv: cannot stat `examples/c/reccalc/scan.stamp.tmp': No such file or directory
    # make[1]: *** [examples/c/reccalc/scan.stamp] Error 1
    # make[1]: Leaving directory `/tmp/bison-3.4.1'
    # make: *** [examples/c/reccalc/scan.h] Error 2
    make # -j${MKJOBS}
    make install
    popd
    rm -rf "bison-${BISON_VERSION}"
    end_build
fi
