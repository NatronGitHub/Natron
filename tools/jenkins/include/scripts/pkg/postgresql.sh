#!/bin/bash

# Install PostgreSQL (for the Qt postgresql plugin and the python postgresql adapter)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/postgresql.html
POSTGRESQL_VERSION=15.19
POSTGRESQL_TAR="postgresql-${POSTGRESQL_VERSION}.tar.bz2"
POSTGRESQL_SHA256=e1a64a87a46b825b88c082e4518161a47aab53c45694964f8ba1df28f7859f89
POSTGRESQL_SITE="https://ftp.postgresql.org/pub/source/v${POSTGRESQL_VERSION}"
if download_step; then
    download "$POSTGRESQL_SITE" "$POSTGRESQL_TAR" "$POSTGRESQL_SHA256"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpq.pc" ] || [ "$(pkg-config --modversion libpq)" != "$POSTGRESQL_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$POSTGRESQL_TAR"
    pushd "postgresql-${POSTGRESQL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --enable-thread-safety
    make -j${MKJOBS}
    make install
    popd
    rm -rf "postgresql-${POSTGRESQL_VERSION}"
    end_build
fi
