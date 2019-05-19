#!/bin/bash

# Qt4 webkit (a version more recent than the one in Qt 4.8.7)
if build_step && { force_build || { [ ! -s "$QT4PREFIX/lib/pkgconfig/QtWebKit.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtWebKit)" != "$QT4WEBKIT_VERSION_PKG" ]; }; }; then
    start_build
    # install a more recent qtwebkit, see http://www.linuxfromscratch.org/blfs/view/7.9/x/qt4.html
    download "$QT4WEBKIT_SITE" "$QT4WEBKIT_TAR"
    mkdir "qtwebkit-${QT4WEBKIT_VERSION}"
    pushd "qtwebkit-${QT4WEBKIT_VERSION}"
    untar "$SRC_PATH/$QT4WEBKIT_TAR"
    patch -Np1 -i "$INC_PATH"/patches/Qt/qtwebkit-2.3.4-gcc5-1.patch
    # disable xslt if libxml2 is compiled with ICU support, or qtwebkit will not compile, see https://aur.archlinux.org/packages/qtwebkit
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        xslt_flag="--no-xslt"
    else
        xslt_flag=""
    fi
    QTDIR="$QT4PREFIX" PATH="$QT4PREFIX/bin:$PATH" \
         Tools/Scripts/build-webkit --qt            \
         --no-webkit2    \
         "$xslt_flag" \
         --prefix="$QT4PREFIX" --makeargs=-j${MKJOBS}
    make -C WebKitBuild/Release install -j${MKJOBS}
    popd
    rm -rf "qtwebkit-${QT4WEBKIT_VERSION}"
    end_build
fi

