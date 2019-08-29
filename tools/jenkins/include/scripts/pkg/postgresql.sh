#!/bin/bash

# Install PostgreSQL (for the Qt postgresql plugin and the python postgresql adapter)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/postgresql.html
POSTGRESQL_VERSION=11.5
POSTGRESQL_TAR="postgresql-${POSTGRESQL_VERSION}.tar.bz2"
POSTGRESQL_SITE="http://ftp.postgresql.org/pub/source/v${POSTGRESQL_VERSION}"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libpq.pc" ] || [ "$(pkg-config --modversion libpq)" != "$POSTGRESQL_VERSION" ]; }; }; then
    start_build
    download "$POSTGRESQL_SITE" "$POSTGRESQL_TAR"
    untar "$SRC_PATH/$POSTGRESQL_TAR"
    pushd "postgresql-${POSTGRESQL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --enable-thread-safety
    make -j${MKJOBS}
    make install
    popd
    rm -rf "postgresql-${POSTGRESQL_VERSION}"
    end_build
fi
