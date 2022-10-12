#!/bin/bash

# Install fftw (GPLv2, for openfx-gmic)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/fftw.html
FFTW_VERSION=3.3.10
FFTW_TAR="fftw-${FFTW_VERSION}.tar.gz"
FFTW_SITE="http://www.fftw.org"
if download_step; then
    download "$FFTW_SITE" "$FFTW_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fftw3.pc" ] || [ "$(pkg-config --modversion fftw3)" != "$FFTW_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$FFTW_TAR"
    pushd "fftw-${FFTW_VERSION}"
    # to compile fftw3f, add --enable-float --enable-sse
    # --enable-avx512 is 64-bit only and requires a modern assembler (Support in binutils (gas/objdump) available from v2.24, CentOS 6.4 has 2.20)
    # /tmp/ccQxpb5e.s:25: Error: bad register name `%zmm14'
    #enableavx512="--enable-avx512"
    #if [ "$BITS" = "32" ]; then
        enableavx512=
    #fi
    # --enable-avx2 requires a modern assembler (Support in binutils (gas/objdump) available from v2.21, CentOS 6.4 has 2.20):
    # /tmp/ccX8Quod.s:86: Error: no such instruction: `vpermpd $177,%ymm0,%ymm0'
    env CFLAGS="$BF -DFFTWCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DFFTWCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix="$SDK_HOME" --enable-shared --enable-threads --enable-sse2 --enable-avx $enableavx512 -enable-avx-128-fma
    make -j${MKJOBS}
    make install
    popd
    rm -rf "fftw-${FFTW_VERSION}"
    end_build
fi
