#!/bin/bash

# Install static qt4 for installer
# see http://www.linuxfromscratch.org/blfs/view/7.9/x/qt4.html
QT4_VERSION=4.8.7
QT4_VERSION_SHORT=${QT4_VERSION%.*}
QT4_TAR="qt-everywhere-opensource-src-${QT4_VERSION}.tar.gz"
QT4_SITE="https://download.qt.io/archive/qt/${QT4_VERSION_SHORT}/${QT4_VERSION}"
QT4WEBKIT_VERSION=2.3.4
QT4WEBKIT_VERSION_SHORT=${QT4WEBKIT_VERSION%.*}
QT4WEBKIT_VERSION_PKG=4.10.4 # version from QtWebkit.pc
QT4WEBKIT_TAR="qtwebkit-${QT4WEBKIT_VERSION}.tar.gz"
QT4WEBKIT_SITE="https://download.kde.org/stable/qtwebkit-${QT4WEBKIT_VERSION_SHORT}/${QT4WEBKIT_VERSION}/src"
if build_step && { force_build || { [ ! -s "$SDK_HOME/installer/bin/qmake" ]; }; }; then
    start_build
    QTIFW_CONF=( "-no-multimedia" "-no-gif" "-qt-libpng" "-no-opengl" "-no-libmng" "-no-libtiff" "-no-libjpeg" "-static" "-openssl-linked" "-confirm-license" "-release" "-opensource" "-nomake" "demos" "-nomake" "docs" "-nomake" "examples" "-no-gtkstyle" "-no-webkit" "-no-avx" "-no-openvg" "-no-phonon" "-no-phonon-backend" "-I${SDK_HOME}/installer/include" "-L${SDK_HOME}/installer/lib" )

    download "$QT4_SITE" "$QT4_TAR"
    untar "$SRC_PATH/$QT4_TAR"
    pushd "qt-everywhere-opensource-src-${QT4_VERSION}"
    #patch -p0 -i "$INC_PATH"/patches/Qt/patch-qt-custom-threadpool.diff
    patch -p0 -i "$INC_PATH"/patches/Qt/qt4-kde-fix.diff
    # (25) avoid zombie processes; see also:
    # https://trac.macports.org/ticket/46608
    # https://codereview.qt-project.org/#/c/61294/
    # approved but abandoned.
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-src_corelib_io_qprocess_unix.cpp.diff    
    # (28) Better invalid fonttable handling
    # Qt commit 0a2f2382 on July 10, 2015 at 7:22:32 AM EDT.
    # not included in 4.8.7 release.
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-better-invalid-fonttable-handling.diff
    # (30) Backport of Qt5 patch to fix an issue with null bytes in QSetting strings (QTBUG-56124).
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-qsettings-null.diff
    # avoid conflict with newer versions of pcre, see eg https://github.com/LLNL/spack/pull/4270
    patch -p0 -i "$INC_PATH"/patches/Qt/qt4-pcre.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/qt-everywhere-opensource-src-4.8.7-gcc6.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/0001-Enable-building-with-C-11-and-C-14.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/qt4-selection-flags-static_cast.patch
    if version_gt "$OPENSSL_VERSION" 1.0.9999; then
        # OpenSSL 1.1 support from ArchLinux https://aur.archlinux.org/cgit/aur.git/tree/qt4-openssl-1.1.patch?h=qt4
        patch -p1 -i "$INC_PATH"/patches/Qt/qt4-openssl-1.1.patch
    fi

    ./configure -prefix "$SDK_HOME/installer" "${QTIFW_CONF[@]}" -v

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    env LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS}
    make install
    popd
    rm -rf "qt-everywhere-opensource-src-${QT4_VERSION}"
    end_build
fi
