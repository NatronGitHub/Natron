#!/bin/bash

# Install Berkeley DB (optional for python2 and python3)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/db.html
BERKELEYDB_VERSION=6.2.32
BERKELEYDB_TAR="db-${BERKELEYDB_VERSION}.tar.gz"
BERKELEYDB_SITE="http://download.oracle.com/berkeley-db"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/sqlite3.pc" ] || [ "$(pkg-config --modversion sqlite3)" != "$BERKELEYDB_VERSION" ]; }; }; then
    start_build
    download "$BERKELEYDB_SITE" "$BERKELEYDB_TAR"
    untar "$SRC_PATH/$BERKELEYDB_TAR"
    pushd "sqlite-autoconf-${BERKELEYDB_VERSION_INT}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" -enable-compat185 --enable-dbm --disable-static --enable-cxx
    make -j${MKJOBS}
    make install
    popd
    rm -rf "sqlite-autoconf-${BERKELEYDB_VERSION_INT}"
    end_build
fi
