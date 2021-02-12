#!/bin/bash

# Install gcc
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gcc.html
# Old Natron 2 version is 4.8.5
GCC_VERSION=9.3.0
#GCC_VERSION=8.1.0
#GCC_VERSION=7.3.0
#GCC_VERSION=5.4.0
#GCC_VERSION=4.9.4
GCC_TAR="gcc-${GCC_VERSION}.tar.gz"
GCC_SITE="http://ftpmirror.gnu.org/gcc/gcc-${GCC_VERSION}"

# Install gcc
MPC_VERSION=1.1.0
MPC_TAR="mpc-${MPC_VERSION}.tar.gz"
MPC_SITE="https://ftp.gnu.org/gnu/mpc"

# MPFR_VERSION=4.0.1 # gcc 8.2.0
MPFR_VERSION=4.0.2 # gcc 9.1.0-9.3.0
MPFR_TAR="mpfr-${MPFR_VERSION}.tar.bz2"
MPFR_SITE="http://www.mpfr.org/mpfr-${MPFR_VERSION}"

# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/gmp.html
GMP_VERSION=6.2.0 # 6.2.0 fails when buiding using GCC 4.4.7 (CentOS6): requires -std=gnu99 but adding it to CFLAGS during configure doesn't help
if [ "${DTS:-0}" -le 3 ] && [ "${CENTOS:-7}" -le 7 ]; then
    GMP_VERSION=6.1.2 # 6.2.0 fails when buiding gcc: requires -std=gnu99 but adding it to CFLAGS during configure doesn't help
fi
GMP_TAR="gmp-${GMP_VERSION}.tar.bz2"
GMP_SITE="https://gmplib.org/download/gmp"

ISL_VERSION=0.14.1
if version_gt "$GCC_VERSION" 4.8.5; then
    ISL_VERSION=0.19
fi
if version_gt "$GCC_VERSION" 8.2.0; then
    ISL_VERSION=0.21
fi
if version_gt "$GCC_VERSION" 9.2.0; then
    # in-tree build of ISL 0.22 fails in GCC 9.2.0 if the system's GCC doesn't support C++11, see
    # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=92484
    #ISL_VERSION=0.22.1
    true
fi
ISL_TAR="isl-${ISL_VERSION}.tar.bz2"
ISL_SITE="http://isl.gforge.inria.fr"

CLOOG_VERSION=0.18.4
CLOOG_TAR="cloog-${CLOOG_VERSION}.tar.gz"
CLOOG_SITE="http://www.bastoul.net/cloog/pages/download/count.php3?url=."

DEVTOOLSET=
# Enable devtoolset on RHEL/CentOS, if installed
# if [ -s /etc/scl/prefixes/devtoolset-8 ]; then
#     DEVTOOLSET="/usr/bin/scl enable devtoolset-8"
# elif [ -s /etc/scl/prefixes/devtoolset-7 ]; then
#     DEVTOOLSET="/usr/bin/scl enable devtoolset-7"
# elif [ -s /etc/scl/prefixes/devtoolset-6 ]; then
#     DEVTOOLSET="/usr/bin/scl enable devtoolset-6"
# elif [ -s /etc/scl/prefixes/devtoolset-4 ]; then
#     DEVTOOLSET="/usr/bin/scl enable devtoolset-4"
# elif [ -s /etc/scl/prefixes/devtoolset-3 ]; then
#     DEVTOOLSET="/usr/bin/scl enable devtoolset-3"
# fi

if download_step; then
    download "$GCC_SITE" "$GCC_TAR"
    download "$MPC_SITE" "$MPC_TAR"
    download "$MPFR_SITE" "$MPFR_TAR"
    download "$GMP_SITE" "$GMP_TAR"
    download "$ISL_SITE" "$ISL_TAR"
    download "$CLOOG_SITE" "$CLOOG_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/gcc-$GCC_VERSION/bin/gcc" ]; }; }; then
    start_build
    untar "$SRC_PATH/$GCC_TAR"
    pushd "gcc-$GCC_VERSION"
    untar "$SRC_PATH/$MPC_TAR"
    mv "mpc-$MPC_VERSION" mpc
    untar "$SRC_PATH/$MPFR_TAR"
    mv "mpfr-$MPFR_VERSION" mpfr
    untar "$SRC_PATH/$GMP_TAR"
    mv "gmp-$GMP_VERSION" gmp
    untar "$SRC_PATH/$ISL_TAR"
    mv "isl-$ISL_VERSION" isl
    untar "$SRC_PATH/$CLOOG_TAR"
    mv "cloog-$CLOOG_VERSION" cloog
    echo $DEVTOOLSET ./configure --prefix="$SDK_HOME/gcc-${GCC_VERSION}" --disable-multilib --enable-languages=c,c++
    if [ -z "$DEVTOOLSET" ]; then
        ./configure --prefix="$SDK_HOME/gcc-${GCC_VERSION}" --disable-multilib --enable-languages=c,c++
        make -j$MKJOBS
        make install
    else
        $DEVTOOLSET './configure --prefix=$SDK_HOME/gcc-${GCC_VERSION} --disable-multilib --enable-languages=c,c++'
        $DEVTOOLSET 'make -j$MKJOBS'
        $DEVTOOLSET 'make install'
    fi
    #make -k check
    popd #"gcc-$GCC_VERSION"
    rm -rf "gcc-$GCC_VERSION"
    ln -sfnv "gcc-${GCC_VERSION}" "$SDK_HOME/gcc"
    end_build
fi
