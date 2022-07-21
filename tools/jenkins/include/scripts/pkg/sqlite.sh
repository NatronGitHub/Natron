#!/bin/bash

# Install sqlite (required for webkit and QtSql SQLite module, optional for python2)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/sqlite.html
SQLITE_VERSION=3.39.1 # also set SQLITE_VERSION_INT and SQLITE_YEAR below
SQLITE_VERSION_INT=3390100
SQLITE_YEAR=2022
SQLITE_TAR="sqlite-autoconf-${SQLITE_VERSION_INT}.tar.gz"
SQLITE_SITE="https://sqlite.org/${SQLITE_YEAR}"
if download_step; then
    download "$SQLITE_SITE" "$SQLITE_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/sqlite3.pc" ] || [ "$(pkg-config --modversion sqlite3)" != "$SQLITE_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SQLITE_TAR"
    pushd "sqlite-autoconf-${SQLITE_VERSION_INT}"
    env CFLAGS="$BF \
            -DSQLITE_ENABLE_FTS3=1 \
            -DSQLITE_ENABLE_COLUMN_METADATA=1     \
            -DSQLITE_ENABLE_UNLOCK_NOTIFY=1       \
            -DSQLITE_SECURE_DELETE=1              \
            -DSQLITE_ENABLE_DBSTAT_VTAB=1" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "sqlite-autoconf-${SQLITE_VERSION_INT}"
    end_build
fi
