#!/bin/bash

# Install ruby (necessary for qtwebkit)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/ruby.html
RUBY_VERSION=3.2.2
RUBY_VERSION_SHORT=${RUBY_VERSION%.*}
RUBY_VERSION_PKG=${RUBY_VERSION_SHORT}.0
RUBY_TAR="ruby-${RUBY_VERSION}.tar.xz"
RUBY_SITE="http://cache.ruby-lang.org/pub/ruby/${RUBY_VERSION_SHORT}"
if download_step; then
    download "$RUBY_SITE" "$RUBY_TAR"
fi
if build_step && { force_build || { [ "${REBUILD_RUBY:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/pkgconfig/ruby"*.pc
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ruby-${RUBY_VERSION_SHORT}.pc" ] || [ "$(pkg-config --modversion "ruby-${RUBY_VERSION_SHORT}")" != "$RUBY_VERSION_PKG" ]; }; }; then
    start_build
    untar "$SRC_PATH/$RUBY_TAR"
    pushd "ruby-${RUBY_VERSION}"
    env CFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-install-doc --disable-install-rdoc --disable-static --enable-shared --without-valgrind
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ruby-${RUBY_VERSION}"
    end_build
fi
