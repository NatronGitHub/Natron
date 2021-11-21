#!/bin/bash

# install ncurses (required for gdb and readline)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter08/ncurses.html
NCURSES_VERSION=6.3
NCURSES_VERSION_PKG=${NCURSES_VERSION}.20211021
NCURSES_TAR="ncurses-${NCURSES_VERSION}.tar.gz"
NCURSES_SITE="ftp://ftp.gnu.org/gnu/ncurses"
if download_step; then
    download "$NCURSES_SITE" "$NCURSES_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ncurses.pc" ] || [ "$(pkg-config --modversion ncurses)" != "$NCURSES_VERSION_PKG" ]; }; }; then
    start_build
    untar "$SRC_PATH/$NCURSES_TAR"
    pushd "ncurses-${NCURSES_VERSION}"
    # Don't install a static library that is not handled by configure:
    sed -i '/LIBTOOL_INSTALL/d' c++/Makefile.in
    # see http://archive.linuxfromscratch.org/mail-archives/lfs-dev/2015-August/070322.html
    sed -i 's/.:space:./ \t/g' ncurses/base/MKlib_gen.sh
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --with-shared --without-debug --without-normal --enable-pc-files --with-pkg-config-libdir="$SDK_HOME/lib/pkgconfig" --enable-widec
    make -j${MKJOBS}
    make install
    #  Many applications still expect the linker to be able to find non-wide-character Ncurses libraries. Trick such applications into linking with wide-character libraries by means of symlinks and linker scripts: 
    for lib in ncurses form panel menu ; do
        rm -vf                    "$SDK_HOME/lib/lib${lib}.so"
        echo "INPUT(-l${lib}w)" > "$SDK_HOME/lib/lib${lib}.so"
        ln -sfv ${lib}w.pc        "$SDK_HOME/lib/pkgconfig/${lib}.pc"
    done
    # Finally, make sure that old applications that look for -lcurses at build time are still buildable:
    rm -vf                     "$SDK_HOME/lib/libcursesw.so"
    echo "INPUT(-lncursesw)" > "$SDK_HOME/lib/libcursesw.so"
    ln -sfv libncurses.so      "$SDK_HOME/lib/libcurses.so"
    ln -sfnv ncursesw "$SDK_HOME/include/ncurses"

    for f in curses.h form.h ncurses.h panel.h term.h termcap.h; do
        ln -sfv "ncursesw/$f" "$SDK_HOME/include/$f"
    done
    popd
    rm -rf "ncurses-${NCURSES_VERSION}"
    end_build
fi

