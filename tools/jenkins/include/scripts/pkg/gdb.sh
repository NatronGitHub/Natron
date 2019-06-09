#!/bin/bash

# install gdb (requires xz, zlib ncurses, python2, texinfo)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/gdb.html
GDB_VERSION=8.3
GDB_TAR="gdb-${GDB_VERSION}.tar.xz" # when using the sdk during debug we get a conflict with native gdb, so bundle our own
GDB_SITE="ftp://ftp.gnu.org/gnu/gdb"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/gdb" ]; }; }; then
    start_build
    download "$GDB_SITE" "$GDB_TAR"
    untar "$SRC_PATH/$GDB_TAR"
    pushd "gdb-${GDB_VERSION}"

    if version_gt 8.3 "${GDB_VERSION}"; then
	# Patch issue with header ordering for TRAP_HWBKPT
	patch -Np1 -i "$INC_PATH"/patches/gdb/gdb-Fix-ia64-defining-TRAP_HWBKPT-before-including-g.patch
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-tui --with-system-readline --without-guile
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdb-${GDB_VERSION}"
    end_build
fi
