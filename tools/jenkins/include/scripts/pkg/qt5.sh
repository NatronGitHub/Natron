#!/bin/bash

if build_step && { force_build || { [ "${REBUILD_QT5:-}" = "1" ]; }; }; then
    rm -rf "$QT5PREFIX"
fi
# Qt5
QT5_VERSION=5.6.1
QT5_VERSION_SHORT=${QT5_VERSION%.*}

# we use vfxplatform fork
QTBASE_GIT=https://github.com/autodesk-forks/qtbase

# qtdeclarative and qtx11extras from autodesk-forks don't compile: we extract the originals and apply the necessary patches instead.
QT_DECLARATIVE_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtdeclarative-opensource-src-${QT5_VERSION}-1.tar.gz"
#QT_DECLARATIVE_URL="https://github.com/autodesk-forks/qtdeclarative/archive/Maya2018.tar.gz"
QT_X11EXTRAS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtx11extras-opensource-src-${QT5_VERSION}-1.tar.gz"
#QT_X11EXTRAS_URL="https://github.com/autodesk-forks/qtx11extras/archive/Maya2017Update3.tar.gz"
QT_XMLPATTERNS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtxmlpatterns-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBVIEW_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebview-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBSOCKETS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebsockets-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBENGINE_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebengine-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBCHANNEL_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebchannel-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_TRANSLATIONS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qttranslations-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_SVG_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtsvg-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_TOOLS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qttools-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_SCRIPT_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtscript-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_QUICKCONTROLS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtquickcontrols-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_QUICKCONTROLS2_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtquickcontrols2-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_MULTIMEDIA_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtmultimedia-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_IMAGEFORMATS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtimageformats-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_GRAPHICALEFFECTS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtgraphicaleffects-opensource-src-${QT5_VERSION}-1.tar.gz"

QT_MODULES=( qtxmlpatterns qtwebview qtwebsockets qtwebengine qtwebchannel qttools qttranslations qtsvg qtscript qtquickcontrols qtquickcontrols2 qtmultimedia qtimageformats qtgraphicaleffects qtdeclarative qtx11extras )
QT_MODULES_URL=( "$QT_XMLPATTERNS_URL" "$QT_WEBVIEW_URL" "$QT_WEBSOCKETS_URL" "$QT_WEBENGINE_URL" "$QT_WEBCHANNEL_URL" "$QT_TOOLS_URL" "$QT_TRANSLATIONS_URL" "$QT_SVG_URL" "$QT_SCRIPT_URL" "$QT_QUICKCONTROLS_URL" "$QT_QUICKCONTROLS2_URL" "$QT_MULTIMEDIA_URL" "$QT_IMAGEFORMATS_URL" "$QT_GRAPHICALEFFECTS_URL" "$QT_DECLARATIVE_URL" "$QT_X11EXTRAS_URL" )

export QTDIR="$QT5PREFIX"
if build_step && { force_build || { [ ! -s "$QT5PREFIX/lib/pkgconfig/Qt5Core.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Qt5Core)" != "$QT5_VERSION" ]; }; }; then
    start_build

    # Install QtBase
    QT_CONF=( "-v" "-openssl-linked" "-opengl" "desktop" "-opensource" "-nomake" "examples" "-nomake" "tests" "-release" "-no-gtkstyle" "-confirm-license" "-no-c++11" "-shared" "-qt-xcb" )

    if [ ! -d "qtbase" ]; then
        git_clone_commit "$QTBASE_GIT" tags/Maya2018
        pushd qtbase
        git submodule update -i --recursive --remote
        patch -p0 -i "$INC_PATH/patches/Qt/qtbase-revert-1b58d9acc493111390b31f0bffd6b2a76baca91b.diff"
        patch -p0 -i "$INC_PATH/patches/Qt/qtbase-threadpool.diff"
        popd
    fi
    pushd qtbase

    QT_LD_FLAGS="-L${SDK_HOME}/lib"
    if [ "$BITS" = "32" ]; then
        QT_LD_FLAGS="$QT_LD_FLAGS -L${SDK_HOME}/gcc/lib"
    else
        QT_LD_FLAGS="$QT_LD_FLAGS -L${SDK_HOME}/gcc/lib64"
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" LDFLAGS="$QT_LD_FLAGS" ./configure -prefix "$QT5PREFIX" "${QT_CONF[@]}"
    LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS}
    make install
    QTBASE_PRIV=( QtCore QtDBus QtGui QtNetwork QtOpenGL QtPlatformSupport QtPrintSupport QtSql QtTest QtWidgets QtXml )
    for i in "${QTBASE_PRIV[@]}"; do
        (cd "$QT5PREFIX/include/$i"; ln -sfv "$QT5_VERSION/$i/private" .)
    done

    popd
    rm -rf qtbase
    end_build

    for (( i=0; i<${#QT_MODULES[@]}; i++ )); do
        module=${QT_MODULES[i]}
        moduleurl=${QT_MODULES_URL[i]}
        step="${module}-${QT5_VERSION}"
        start_build
        package="${module}-${QT5_VERSION}.tar.gz"
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** downloading $package"
            $WGET --no-check-certificate "$moduleurl" -O "$SRC_PATH/$package" || rm "$SRC_PATH/$package"
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** Error: unable to download $package"
            exit 1
        fi
        untar "$SRC_PATH/$package"
        dirs=(${module}-*)
        if [ ${#dirs[@]} -ne 1 ]; then
            (>&2 echo "Error: Qt5: more than one match for ${module}-*: ${dirs[*]}")
            exit 1
        fi
        dir=${dirs[0]}
        pushd "$dir"
        if [ "$module" = "qtdeclarative" ]; then
            patch -p1 -i "$INC_PATH/patches/Qt/qtdeclarative-5.6.1-backport_core_profile_qtdeclarative_5.6.1.patch"
        fi
        if [ "$module" = "qtx11extras" ]; then
            patch -p1 -i "$INC_PATH/patches/Qt/qtx11extras-5.6.1-isCompositingManagerRunning.patch"
            patch -p1 -i "$INC_PATH/patches/Qt/qtx11extras-5.6.1-qtc-wip-add-peekeventqueue-api.patch"
        fi
        "$QT5PREFIX/bin/qmake"
        make -j${MKJOBS}
        make install
        popd
        rm -rf "$dir"

        # Copy private headers to includes because Pyside2 needs them
        ALL_MODULES_INCLUDE=("$QT5PREFIX"/include/*)
        for m in "${ALL_MODULES_INCLUDE[@]}"; do
            (cd "$m"; ln -sfv "$QT5_VERSION"/*/private .)
        done

        end_build
    done
fi
