#!/bin/bash

# Install shared-mime-info (required by gdk-pixbuf)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/shared-mime-info.html
SHAREDMIMEINFO_VERSION=2.5.1
#SHAREDMIMEINFO_VERSION=2.1 # requires docbook-xml. Warning: SHAREDMIMEINFO_SITE changes with every release
SHAREDMIMEINFO_TAR="shared-mime-info-${SHAREDMIMEINFO_VERSION}.tar.bz2"
#SHAREDMIMEINFO_SITE="http://freedesktop.org/~hadess"
SHAREDMIMEINFO_SITE="https://gitlab.freedesktop.org/xdg/shared-mime-info/-/archive/${SHAREDMIMEINFO_VERSION}"
#SHAREDMIMEINFO_SITE="https://gitlab.freedesktop.org/xdg/shared-mime-info/-/archive/${SHAREDMIMEINFO_VERSION}" # 2.1
if download_step; then
    download "$SHAREDMIMEINFO_SITE" "$SHAREDMIMEINFO_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/share/pkgconfig/shared-mime-info.pc" ] || [ "$(pkg-config --modversion shared-mime-info)" != "$SHAREDMIMEINFO_VERSION" ]; }; }; then
    start_build
    untar "$SRC_PATH/$SHAREDMIMEINFO_TAR"
    pushd "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    if version_gt "2.0" "${SHAREDMIMEINFO_VERSION}"; then
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
        #make -j${MKJOBS}
        make
        make install
        #cp "$SDK_HOME/share/pkgconfig/shared-mime-info.pc" "$SDK_HOME/lib/pkgconfig/shared-mime-info.pc"
    else
        mkdir build
        cd build
        env CFLAGS="$BF" CXXFLAGS="$BF" meson setup --prefix="$SDK_HOME" -Dupdate-mimedb=true -Dbuild-spec=false -Dbuild-tests=false -Dbuild-translations=false ..
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja
        env CFLAGS="$BF" CXXFLAGS="$BF" ninja install
    fi
    popd
    rm -rf "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    end_build
fi
