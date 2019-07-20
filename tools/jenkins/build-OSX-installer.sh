#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
set -x # Print commands and their arguments as they are executed.
#set -v # Prints shell input lines as they are read.

echo "*** OSX installer..."

source common.sh
source manageBuildOptions.sh
source manageLog.sh

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

updateBuildOptions

DUMP_SYMS=/usr/local/bin/dump_syms
DSYMUTIL=dsymutil
if [ "$COMPILER" = "clang" ] || [ "$COMPILER" = "clang-omp" ]; then
    if [ -x /opt/local/bin/llvm-dsymutil-mp-3.9 ]; then
        DSYMUTIL=/opt/local/bin/llvm-dsymutil-mp-3.9
    fi
    if [ -x /opt/local/bin/llvm-dsymutil-mp-4.0 ]; then
        DSYMUTIL=/opt/local/bin/llvm-dsymutil-mp-4.0
    fi
    if [ -x /opt/local/bin/llvm-dsymutil-mp-5.0 ]; then
        DSYMUTIL=/opt/local/bin/llvm-dsymutil-mp-5.0
    fi
    if [ -x /opt/local/bin/llvm-dsymutil-mp-6.0 ]; then
        DSYMUTIL=/opt/local/bin/llvm-dsymutil-mp-6.0
    fi
    if [ -x /opt/local/bin/dsymutil-mp-7.0 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-7.0
    fi
    if [ -x /opt/local/bin/dsymutil-mp-8.0 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-8.0
    fi
    if [ -x /opt/local/bin/dsymutil-mp-9.0 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-9.0
    fi
fi

# the list of libs in /opt/local/lib/libgcc
#gcclibs="atomic.1 gcc_ext.10.4 gcc_ext.10.5 gcc_s.1 gfortran.3 gomp.1 itm.1 objc-gnu.4 quadmath.0 ssp.0 stdc++.6"
gcclibs="atomic.1 gcc_s.1 gfortran.3 gomp.1 itm.1 objc-gnu.4 quadmath.0 ssp.0 stdc++.6"
#  clang's OpenMP lib
omplibs="omp"


# tag symbols we want to keep with 'release'
BPAD_TAG=""
if [ "${NATRON_BUILD_CONFIG:-}" != "" ] && [ "${NATRON_BUILD_CONFIG:-}" != "SNAPSHOT" ]; then
    BPAD_TAG="-release"
    #if [ -f "$TMP_BINARIES_PATH/natron-fullversion.txt" ]; then
    #    GET_VER=`cat "$TMP_BINARIES_PATH/natron-fullversion.txt"`
    #    if [ "$GET_VER" != "" ]; then
    #        TAG=$GET_VER
    #    fi
    #fi
fi

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    echo "=====> NO $TMP_BINARIES_PATH, NOTHING TO DO HERE!"
    exit 1
fi

if [ "${DEBUG_SCRIPTS:-}" = "0" ]; then
if [ -d "${TMP_PORTABLE_DIR}.app" ]; then
    echo "${TMP_PORTABLE_DIR}.app already exists, removing it"
    rm -rf "${TMP_PORTABLE_DIR}.app"
fi
fi
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi
if [ ! -d "${TMP_PORTABLE_DIR}.app" ]; then
	mkdir -p "${TMP_PORTABLE_DIR}.app"
	mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/Frameworks"
	mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/Plugins"
	mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/MacOS"
	mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/Plugins/OFX/Natron"
fi

#mv "$TMP_BINARIES_PATH/Plugins/"*.ofx.bundle "${TMP_PORTABLE_DIR}.app/Contents/Plugins/OFX/Natron/" || true
if [ "${DEBUG_SCRIPTS:-}" = "1" ]; then
   if [ -d "$TMP_BINARIES_PATH/bin" ]; then
   	cp -r "$TMP_BINARIES_PATH/bin/"* "${TMP_PORTABLE_DIR}.app/Contents/MacOS/"
   fi
   if [ -d "$TMP_BINARIES_PATH/OFX/Plugins" ]; then
   	cp -r "$TMP_BINARIES_PATH/OFX/Plugins/"*.ofx.bundle "${TMP_PORTABLE_DIR}.app/Contents/Plugins/OFX/Natron/" || true
   fi
else
    mv "$TMP_BINARIES_PATH/bin/"* "${TMP_PORTABLE_DIR}.app/Contents/MacOS/"
    mv "$TMP_BINARIES_PATH/OFX/Plugins/"*.ofx.bundle "${TMP_PORTABLE_DIR}.app/Contents/Plugins/OFX/Natron/" || true
fi


if [ -d "$TMP_BINARIES_PATH/PyPlugs" ] && [ ! -d "${TMP_PORTABLE_DIR}.app/Contents/Plugins/PyPlugs" ]; then
	mv "$TMP_BINARIES_PATH/PyPlugs" "${TMP_PORTABLE_DIR}.app/Contents/Plugins/"
fi
if [ -d "$TMP_BINARIES_PATH/Resources" ] && [ ! -d "${TMP_PORTABLE_DIR}.app/Contents/Resources" ]; then
	mv "$TMP_BINARIES_PATH/Resources" "${TMP_PORTABLE_DIR}.app/Contents/"
fi
NATRON_BIN_VERSION=$NATRON_VERSION_SHORT
if [ "$NATRON_BIN_VERSION" = "" ]; then
    NATRON_BIN_VERSION="2.0"
fi

# move Info.plist and PkgInfo, see "copy Info.plist and PkgInfo" in build-natron.sh
if [ -f "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Info.plist" ]; then
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Info.plist" "${TMP_PORTABLE_DIR}.app/Contents/Info.plist"
else
    $GSED "s/_REPLACE_VERSION_/${NATRON_BIN_VERSION:-}/" < "$INC_PATH/natron/Info.plist" > "${TMP_PORTABLE_DIR}.app/Contents/Info.plist"
fi
if [ -f "${TMP_PORTABLE_DIR}.app/Contents/MacOS/PkgInfo" ]; then
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/PkgInfo" "${TMP_PORTABLE_DIR}.app/Contents/PkgInfo"
else
    echo "APPLNtrn" > "${TMP_PORTABLE_DIR}.app/Contents/PkgInfo"
fi

#Dump symbols for breakpad before stripping
for bin in IO Misc CImg Arena GMIC Shadertoy Extra Magick OCL; do
    ofx_binary="${TMP_PORTABLE_DIR}.app/Contents/Plugins/OFX/Natron"/${bin}.ofx.bundle/Contents/MacOS/${bin}.ofx

    #Strip binary
    if [ -x "$ofx_binary" ] && [ "$COMPILE_TYPE" != "debug" ]; then
        if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
            STATUS=0
            if [ "$BITS" = "64" ] || [ "$BITS" = "Universal" ]; then
                $DSYMUTIL --arch=x86_64 "$ofx_binary" || STATUS=$?
                if [ $STATUS == 0 ] && [ -d "${ofx_binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${ofx_binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-Mac-x86_64.sym" | head -10
                    rm -rf "${ofx_binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 $ofx_binary failed with exit status $STATUS"
                fi
            fi
            STATUS=0
            if [ "$BITS" = "32" ] || [ "$BITS" = "Universal" ]; then
                $DSYMUTIL --arch=i386 "$ofx_binary" || STATUS=$?
                if [ $STATUS == 0 ] && [ -d "${ofx_binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${ofx_binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-Mac-i386.sym" | head -10
                    rm -rf "${ofx_binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=i386 $ofx_binary failed with exit status $STATUS"
                fi
            fi
        fi

        echo "* stripping $ofx_binary";
        # Retain the original binary for QA and use with the util 'atos'
        #mv -f "$ofx_binary" "${binary}_FULL";
        if [ "$BITS" = "Universal" ] && lipo "$ofx_binary" -verify_arch i386 x86_64; then
            # Extract each arch into a "thin" binary for stripping
            lipo "$ofx_binary" -thin x86_64 -output "${ofx_binary}_x86_64";
            lipo "$ofx_binary" -thin i386   -output "${ofx_binary}_i386";

            # Perform desired stripping on each thin binary.
            strip -S -x -r "${ofx_binary}_i386";
            strip -S -x -r "${ofx_binary}_x86_64";
            # Make the new universal binary from our stripped thin pieces.
            lipo -arch i386 "${ofx_binary}_i386" -arch x86_64 "${ofx_binary}_x86_64" -create -output "${ofx_binary}";

            # We're now done with the temp thin binaries, so chuck them.
            rm -f "${ofx_binary}_i386";
            rm -f "${ofx_binary}_x86_64";
        # Do not strip, otherwise we get the following error "the __LINKEDIT segment does not cover the end of the file"
        #else
        #   strip -S -x -r "${ofx_binary}"
        fi
        #rm -f "${ofx_binary}_FULL";
    else
        echo "$ofx_binary not found for stripping"
    fi
done


if [ "${COMPILE_TYPE:-}" != "debug" ]; then

PLUGINDIR="${TMP_PORTABLE_DIR}.app/Contents/Plugins"
# move all libraries to the same place, put symbolic links instead
for plugin in "$PLUGINDIR/OFX/Natron"/*.ofx.bundle; do
    cd "$plugin/Contents/Libraries"
    # relative path to the ${TMP_PORTABLE_DIR}.app/Contents/Frameworks dir
    frameworks=../../../../../../Frameworks
    for lib in lib*.dylib; do
        if [ -f "$lib" ]; then
            if [ -f "$frameworks/$lib" ]; then
                rm "$lib"
            else
                mv "$lib" "$frameworks/$lib"
            fi
            ln -sf "$frameworks/$lib" "$lib"
        fi
    done
    if [ "$COMPILER" = "gcc" ]; then # use gcc's libraries everywhere
        for l in $gcclibs; do
            lib="lib${l}.dylib"
            for deplib in "$plugin"/Contents/MacOS/*.ofx "$plugin"/Contents/Libraries/lib*dylib ; do
                if [ -f "$deplib" ]; then
                    install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
		    install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
                fi
            done
        done
    fi
    if [ "$COMPILER" = "clang-omp" ]; then # use clang's OpenMP libraries everywhere
        for l in $omplibs; do
            lib="lib${l}.dylib"
            for deplib in "$plugin"/Contents/MacOS/*.ofx "$plugin"/Contents/Libraries/lib*dylib ; do
		if [ -f "$deplib" ]; then
                    install_name_tool -change "${MACPORTS}/lib/libomp/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
                fi
            done
        done
    fi
done

fi



package="${TMP_PORTABLE_DIR}.app"
if [ ! -d "$package" ]; then
    echo "Error: application directory '$package' does not exist"
    exit 1
fi

MACPORTS="/opt/local"
HOMEBREW="/brew2/local"
LOCAL="/usr/local"
SBKVER="1.2"
QTDIR="${MACPORTS}/libexec/qt4"
## all Qt frameworks:
QT_LIBS="Qt3Support QtCLucene QtCore QtDBus QtDeclarative QtDesigner QtDesignerComponents QtGui QtHelp QtMultimedia QtNetwork QtOpenGL QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtWebKit QtXml QtXmlPatterns"
## Qt frameworks used by Natron + PySide + Qt plugins:
#QT_LIBS="Qt3Support QtCLucene QtCore QtDBus QtDeclarative QtDesigner QtGui QtHelp QtMultimedia QtNetwork QtOpenGL QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtWebKit QtXml QtXmlPatterns"
STRIP=1

"$QTDIR"/bin/macdeployqt "${package}" -no-strip

binary="$package/Contents/MacOS/Natron"
libdir="Frameworks"
pkglib="$package/Contents/$libdir"

if [ ! -x "$binary" ]; then
    echo "Error: $binary does not exist or is not an executable"
    exit 1
fi

# macdeployqt doesn't deal correctly with libs in ${MACPORTS}/lib/libgcc : handle them manually
LIBGCC=0
if otool -L "$binary" | grep -F -q "${MACPORTS}/lib/libgcc"; then
    LIBGCC=1
fi
COPY_LIBCXX=0
if otool -L "$binary" | grep -F -q "/usr/lib/libc++"; then
    osmajor=$(uname -r | $GSED 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|')
    # libc++ appeared in on Lion 10.7 (osmajor=11)
    if [ "$osmajor" -lt 11 ]; then
        COPY_LIBCXX=1
    fi
fi

#Copy and change exec_path of the whole Python framework with libraries
rm -rf "$pkglib/Python.framework"
mkdir -p "$pkglib/Python.framework/Versions/${PYVER}/lib"
cp -r "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}" "$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}"
rm -rf "$pkglib/Python.framework/Versions/${PYVER}/Resources"
cp -r "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/Resources" "$pkglib/Python.framework/Versions/${PYVER}/Resources"
rm -rf "$pkglib/Python.framework/Versions/${PYVER}/Python"
cp "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/Python" "$pkglib/Python.framework/Versions/${PYVER}/Python"
cp -r "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/include" "$pkglib/Python.framework/Versions/${PYVER}/"
chmod 755 "$pkglib/Python.framework/Versions/${PYVER}/Python"
install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/${PYVER}/Python" "$pkglib/Python.framework/Versions/${PYVER}/Python"
ln -sf "Versions/${PYVER}/Python" "$pkglib/Python.framework/Python"

rm -rf "$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}/site-packages/"*
#rm -rf "$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}/__pycache__"
#rm -rf "$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}/*/__pycache__"
#FILES=`ls -l "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/lib|awk" '{print $9}'`
#for f in FILES; do
#    #FILE=echo "{$f}" | $GSED "s/cpython-34.//g"
#    cp -r "$f" "$pkglib/Python.framework/Versions/${PYVER}/lib/$FILE"
#done

# a few elements of ${TMP_PORTABLE_DIR}.app/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/lib-dynload may load other libraries
DYNLOAD="$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}/lib-dynload"
if [ ! -d "${DYNLOAD}" ]; then
    echo "lib-dynload not present"
    exit 1
fi

MPLIBS0="$(for i in "${DYNLOAD}"/*.so; do otool -L "$i" | grep -F "${MACPORTS}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
# also add first-level and second-level dependencies 
MPLIBS1="$(for i in $MPLIBS0; do echo "$i"; otool -L "$i" | grep -F "${MACPORTS}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
MPLIBS="$(for i in $MPLIBS1; do echo "$i"; otool -L "$i" | grep -F "${MACPORTS}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
for mplib in $MPLIBS; do
    if [ ! -f "$mplib" ]; then
        echo "missing python lib-dynload depend $mplib"
        exit 1
    fi
    lib="$(echo "$mplib" | awk -F / '{print $NF}')"
    if [ ! -f "$pkglib/${lib}" ]; then
        cp "$mplib" "$pkglib/${lib}"
        chmod +w "$pkglib/${lib}"
    fi
    install_name_tool -id "@executable_path/../Frameworks/$lib" "$pkglib/$lib"
    for deplib in "${DYNLOAD}"/*.so; do
	if [ -f "$deplib" ]; then
            install_name_tool -change "${mplib}" "@executable_path/../Frameworks/$lib" "$deplib"
        fi
    done
done

# also fix newly-installed libs
for mplib in $MPLIBS; do
    lib="$(echo "$mplib" | awk -F / '{print $NF}')"
    for mplibsub in $MPLIBS; do
        deplib="$(echo "$mplibsub" | awk -F / '{print $NF}')"
        install_name_tool -change "${mplib}" "@executable_path/../Frameworks/$lib" "$pkglib/$deplib"
    done
done

# qt4-mac@4.8.7_2 doesn't deploy plugins correctly. macdeployqt ${TMP_PORTABLE_DIR}.app -verbose=2 gives:
# Log: Deploying plugins from "/opt/local/libexec/qt4/Library/Framew/plugins" 
# see https://trac.macports.org/ticket/49344
if [ ! -d "${package}/Contents/PlugIns" ] && [ -d "$QTDIR/share/plugins" ]; then
    echo "Warning: Qt plugins not copied by macdeployqt, see https://trac.macports.org/ticket/49344. Copying them now."
    mkdir -p "${package}/Contents/PlugIns/sqldrivers"
    cp -r "$QTDIR/share/plugins"/{graphicssystems,iconengines,imageformats} "${package}/Contents/PlugIns/"
    #cp -r "$QTDIR/share/plugins/sqldrivers/libqsqlite.dylib" "${package}/Contents/PlugIns/sqldrivers/"
    for binary in "${package}/Contents/PlugIns"/*/*.dylib; do
        if [ -f "$binary" ]; then
            chmod +w "$binary"
            for lib in libjpeg.8.dylib libmng.2.dylib libtiff.5.dylib libQGLViewer.2.dylib; do
                install_name_tool -change "${MACPORTS}/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
            done
            for f in $QT_LIBS; do
                install_name_tool -change "${QTDIR}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$binary"
            done
            if otool -L "$binary" | grep -F "${MACPORTS}"; then
                echo "Error: MacPorts libraries remaining in $binary, please check"
                exit 1
            fi
        fi
    done
fi
# Now, because plugins were not installed (see see https://trac.macports.org/ticket/49344 ),
# their dependencies were not installed either (e.g. QtSvg and QtXml for imageformats/libqsvg.dylib)
# Besides, PySide may also load other Qt Frameworks. We have to make sure they are all present
for qtlib in $QT_LIBS; do
    if [ ! -d "${package}/Contents/Frameworks/${qtlib}.framework" ]; then
        binary="${package}/Contents/Frameworks/${qtlib}.framework/Versions/4/${qtlib}"
        mkdir -p "$(dirname "${binary}")"
        cp "${QTDIR}/Library/Frameworks/${qtlib}.framework/Versions/4/${qtlib}" "${binary}"
        chmod +w "${binary}"
        install_name_tool -id "@executable_path/../Frameworks/${qtlib}.framework/Versions/4/${qtlib}" "$binary"
        for f in $QT_LIBS; do
            install_name_tool -change "${QTDIR}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$binary"
        done
        for lib in libcrypto.1.0.0.dylib libdbus-1.3.dylib libpng16.16.dylib libssl.1.0.0.dylib libz.1.dylib; do
            install_name_tool -change "${MACPORTS}/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
        done
    fi
done

if [ "$LIBGCC" = "1" ]; then
    for l in $gcclibs; do
        lib="lib${l}.dylib"
        cp "${MACPORTS}/lib/libgcc/$lib" "$pkglib/$lib"
        install_name_tool -id "@executable_path/../Frameworks/$lib" "$pkglib/$lib"
    done
    for l in $gcclibs; do
        lib="lib${l}.dylib"
        install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$binary"
        for deplib in "$pkglib/"*.dylib "$pkglib/"*.framework/Versions/4/*; do
            if [ -f "$deplib" ]; then
                install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
            fi
        done
    done
    # use gcc's libraries everywhere
    for l in $gcclibs; do
        lib="lib${l}.dylib"
        for deplib in "$pkglib/"*.framework/Versions/*/* "$pkglib/"lib*.dylib; do
            if [ -f "$deplib" ]; then
                install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
            fi
        done
    done
fi
if [ "$COMPILER" = "clang-omp" ]; then
    for l in $omplibs; do
        lib="lib${l}.dylib"
        cp "${MACPORTS}/lib/libomp/$lib" "$pkglib/$lib"
        install_name_tool -id "@executable_path/../Frameworks/$lib" "$pkglib/$lib"
    done
    for l in $omplibs; do
        lib="lib${l}.dylib"
        install_name_tool -change "${MACPORTS}/lib/libomp/$lib" "@executable_path/../Frameworks/$lib" "$binary"
        for deplib in "$pkglib/"*.dylib "$pkglib/"*.framework/Versions/4/*; do
            if [ -f "$deplib" ]; then
                install_name_tool -change "${MACPORTS}/lib/libomp/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
            fi
        done
    done
fi
# shellcheck disable=SC2043
for f in Python; do
    install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "$binary"
done

# fix library ids
pushd "$pkglib"
for deplib in *.framework/Versions/*/* lib*.dylib; do
    if [ -f "$deplib" ]; then
        install_name_tool -id "@executable_path/../Frameworks/${deplib}" "$deplib"
    fi
done
popd # pushd "$pkglib"

if otool -L "$binary" | grep -F "${MACPORTS}"; then
    echo "Error: MacPorts libraries remaining in $binary, please check"
    exit 1
fi
for deplib in "$pkglib/"*.framework/Versions/*/* "$pkglib/"lib*.dylib; do
    if otool -L "$deplib" | grep -F "${MACPORTS}"; then
        echo "Error: MacPorts libraries remaining in $deplib, please check"
        exit 1
    fi
done


#cp "Gui/Resources/Stylesheets/mainstyle.qss" "${package}/Contents/Resources/"
cp -a "${MACPORTS}/share/poppler" "${package}/Contents/Resources/"


# install PySide in site-packages
echo "* installing PySide..."
PYLIB="Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}"
PYSIDE="${PYLIB}/site-packages/PySide"
rm -rf "${package}/Contents/${PYSIDE}"
cp -r "${MACPORTS}/Library/${PYSIDE}" "${package}/Contents/${PYSIDE}"
# install pyside and shiboken libs, and fix deps from Qt
for l in  pyside-python${PYVER}.${SBKVER} shiboken-python${PYVER}.${SBKVER}; do
    dylib="lib${l}.dylib"
    binary="${package}/Contents/Frameworks/$dylib"
    cp "${MACPORTS}/lib/$dylib" "$binary"
    for f in $QT_LIBS; do
        install_name_tool -change "${QTDIR}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$binary"
    done
done


echo "* Cleaning Python..."
# remove pyo files
#find "${package}/Contents/${PYLIB}" -type f -name '*.pyo' -exec rm {} \;
# prune large python files
(cd "${package}/Contents/${PYLIB}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"

pushd "${package}/Contents/${PYLIB}/config"
chmod -x Makefile Setup Setup.config Setup.local config.c config.c.in python.o
rm -f "libpython${PYVER}.a" "libpython${PYVER}.dylib"
# yes, the static library is actually a link to the dynamic library on OS X (check before doing this on other archs)
ln -s ../../../Python "libpython${PYVER}.a"
ln -s ../../../Python "libpython${PYVER}.dylib"
popd # pushd "${package}/Contents/${PYLIB}/config"

(cd "${package}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib"; ln -s ../Python "libpython${PYVER}.dylib")

echo "* Fixing sonames in PySide..."
for qtlib in $QT_LIBS ;do
    binary="${package}/Contents/${PYSIDE}/${qtlib}.so"
    if [ -f "$binary" ]; then
        install_name_tool -id "@executable_path/../${PYSIDE}/${qtlib}.so" "$binary"
        for f in $QT_LIBS; do
            install_name_tool -change "${QTDIR}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$binary"
        done

        for l in  pyside-python${PYVER}.${SBKVER} shiboken-python${PYVER}.${SBKVER}; do
            dylib="lib${l}.dylib"
            install_name_tool -change "${MACPORTS}/lib/$dylib" "@executable_path/../Frameworks/$dylib" "$binary"
        done
        if [ "$LIBGCC" = "1" ]; then
            for l in $gcclibs; do
                lib="lib${l}.dylib"
                install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
            done
        fi
        
        if otool -L "$binary" | grep -F "${MACPORTS}"; then
            echo "Error: MacPorts libraries remaining in $binary, please check"
            exit 1
        fi
    fi
done

#############################################################################
# Other binaries

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    natronbins="Natron NatronRenderer NatronProjectConverter natron-python NatronCrashReporter NatronRendererCrashReporter Tests"
else
    natronbins="Natron NatronRenderer NatronProjectConverter natron-python Tests"
fi

otherbins="ffmpeg ffprobe iconvert idiff igrep iinfo exrheader tiffinfo" # iv maybe?
for bin in $otherbins; do
    cp "${MACPORTS}/bin/$bin" "$package/Contents/MacOS/"
done

echo "* Obfuscate the MacPorts paths..."

# generate a pseudo-random string which has the same length as $MACPORTS
RANDSTR="R7bUU6jiFvqrPy6zLVPwIC3b93R2b1RG2qD3567t8hC3b93R2b1RG2qD3567t8h"
MACRAND=${RANDSTR:0:${#MACPORTS}}
HOMEBREWRAND=${RANDSTR:0:${#HOMEBREW}}
LOCALRAND=${RANDSTR:0:${#LOCAL}}


for bin in $natronbins $otherbins; do
    binary="$package/Contents/MacOS/$bin"
    if [ -f "$binary" ]; then
        rpath="$(otool -l "$binary" | grep -A 3 LC_RPATH | grep path|awk '{ print $2 }')"
        if [ "$rpath" != "@loader_path/../$libdir" ]; then
            echo "Warning: The runtime search path in $binary does not contain \"@loader_path/../$libdir\". Please set it in your Xcode project, or link the binary with the flags -Xlinker -rpath -Xlinker \"@loader_path/../$libdir\" . Fixing it!"
            #exit 1
	    install_name_tool -add_rpath "@loader_path/../$libdir" "$binary"
        fi
        # Test dirs
        if [ ! -d "$pkglib" ]; then
            mkdir "$pkglib"
        fi

        ### FIX FRAMEWORKS (must be done before dylibs)
        
	# maydeployqt only fixes the main binary, fix others too
        for f in $QT_LIBS; do
            install_name_tool -change "${QTDIR}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$binary"
	done

	# Copy and change exec_path of the whole Python framework with libraries
        # shellcheck disable=SC2043
	for f in Python; do
            install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "$binary"
	done


        ### FIX DYLIBS
        
	if [ "$LIBGCC" = "1" ]; then
            for l in $gcclibs; do
		lib="lib${l}.dylib"
		install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$binary"
            done
	fi

	if [ "$LIBGCC" = "1" ]; then
            for l in $gcclibs; do
		lib="lib${l}.dylib"
		cp "${MACPORTS}/lib/libgcc/$lib" "$pkglib/$lib"
		install_name_tool -id "@executable_path/../Frameworks/$lib" "$pkglib/$lib"
            done
            for l in $gcclibs; do
		lib="lib${l}.dylib"
		install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$binary"
		install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
		for deplib in "$pkglib/"*.dylib "$pkglib/"*.framework/Versions/4/*; do
                    if [ -f "$deplib" ]; then
                        install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
                    fi
		done
            done
            # use gcc's libraries everywhere
            for l in $gcclibs; do
		lib="lib${l}.dylib"
		for deplib in "$pkglib/"*.framework/Versions/*/* "$pkglib/"lib*.dylib; do
                    if [ -f "$deplib" ]; then
                        install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
                    fi
		done
            done
	fi
	if [ "$COMPILER" = "clang-omp" ]; then
            for l in $omplibs; do
		lib="lib${l}.dylib"
		cp "${MACPORTS}/lib/libomp/$lib" "$pkglib/$lib"
		install_name_tool -id "@executable_path/../Frameworks/$lib" "$pkglib/$lib"
            done
            for l in $omplibs; do
		lib="lib${l}.dylib"
		install_name_tool -change "${MACPORTS}/lib/libomp/$lib" "@executable_path/../Frameworks/$lib" "$binary"
		for deplib in "$pkglib/"*.dylib "$pkglib/"*.framework/Versions/4/*; do
                    if [ -f "$deplib" ]; then
                        install_name_tool -change "${MACPORTS}/lib/libomp/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
                    fi
		done
            done
	fi

        LIBADD=()

        #############################
        # test if ImageMagick is used
        if otool -L "$binary"  | grep -F libMagick > /dev/null; then
            # Check that ImageMagick is properly installed
            if ! pkg-config --modversion ImageMagick >/dev/null 2>&1; then
                echo "Missing ImageMagick -- please install ImageMagick ('sudo port install ImageMagick +no_x11 +universal') and try again." >&2
                exit 1
            fi

            # Update the ImageMagick path in startup script.
            IMAGEMAGICKVER="$(pkg-config --modversion ImageMagick)"
            IMAGEMAGICKMAJ=${IMAGEMAGICKVER%.*.*}
            IMAGEMAGICKLIB="$(pkg-config --variable=libdir ImageMagick)"
            IMAGEMAGICKSHARE="$(pkg-config --variable=prefix ImageMagick)/share"
            # if I get this right, $GSED substitutes in the exe the occurrences of IMAGEMAGICKVER
            # into the actual value retrieved from the package.
            # We don't need this because we use MAGICKCORE_PACKAGE_VERSION declared in the <magick/magick-config.h>
            # $GSED -e "s,IMAGEMAGICKVER,$IMAGEMAGICKVER,g" --in-place $pkgbin/DisparityKillerM

            # copy the ImageMagick libraries (.la and .so)
            cp -r "$IMAGEMAGICKLIB/ImageMagick-$IMAGEMAGICKVER" "$pkglib/"
            if [ ! -d "$pkglib/share" ]; then
                mkdir "$pkglib/share"
            fi
            cp -r "$IMAGEMAGICKSHARE/ImageMagick-$IMAGEMAGICKMAJ" "$pkglib/share/"

            # see http://stackoverflow.com/questions/7577052/bash-empty-array-expansion-with-set-u
            LIBADD=(${LIBADD[@]+"${LIBADD[@]}"} "$pkglib/ImageMagick-$IMAGEMAGICKVER"/modules-*/*/*.so)
            #WITH_IMAGEMAGICK=yes
        fi

        # expand glob patterns in LIBADD (do not double-quote $LIBADD)
        #LIBADD="$(echo $LIBADD)"

        # Find out the library dependencies
        # (i.e. $LOCAL or $MACPORTS), then loop until no changes.
        a=1
        nfiles=0
        alllibs=()
        endl=true
        while $endl; do
            #echo -e "\033[1mLooking for dependencies.\033[0m Round" $a
            # see http://stackoverflow.com/questions/7577052/bash-empty-array-expansion-with-set-u
            libs=( $(otool -L "$pkglib"/*.dylib "${LIBADD[@]+${LIBADD[@]}}" "$binary" 2>/dev/null | grep -F compatibility | cut -d\( -f1 | grep -e "$LOCAL"'\|'"$HOMEBREW"'\|'"$MACPORTS" | sort | uniq) )
            if [ -n "${libs[*]-}" ]; then
                # cp option '-n': do not overwrite any existing file (mainly for pyside and shiboken, which were copied before)
                cp -n "${libs[@]}" "$pkglib" || true
                # shellcheck disable=SC2012
                alllibs=( $(ls "${alllibs[@]+${alllibs[@]}}" "${libs[@]}" | sort | uniq) )
            fi
            let "a+=1"  
            # shellcheck disable=SC2012
            nnfiles="$(ls "$pkglib" | wc -l)"
            if [ "$nnfiles" = "$nfiles" ]; then
                endl=false
            else
                nfiles=$nnfiles
            fi
        done

        # all the libraries were copied, now change the names...
        ## We use @rpath instead of @executable_path/../$libdir because it's shorter
        ## than /opt/local, so it always works. The downside is that the XCode project
        ## has to link the binary with "Runtime search paths" set correctly
        ## (e.g. to "@loader_path/../Frameworks @loader_path/../Libraries" ),
        ## or the binary has to be linked with the following flags:
        ## -Wl,-rpath,@loader_path/../Frameworks -Wl,-rpath,@loader_path/../Libraries
        if [ -n "${alllibs[*]-}" ]; then
            changes=()
            for l in "${alllibs[@]}"; do
                changes=( ${changes[@]+"${changes[@]}"} -change "$l" "@rpath/$(basename "$l")" )
            done
            if [ -n "${changes[*]-}" ]; then
                for f in  "$pkglib"/* "${LIBADD[@]+${LIBADD[@]}}" "$binary"; do
                    # avoid directories
                    if [ -f "$f" ]; then
                        chmod +w "$f"
                        if ! install_name_tool "${changes[@]}" "$f"; then
                            echo "Error: 'install_name_tool ${changes[*]} $f' failed"
                            exit 1
                        fi
                        if ! install_name_tool -id "@rpath/$(basename "$f")" "$f"; then
                            echo "Error: 'install_name_tool -id @rpath/$(basename "$f") $f' failed"
                            exit 1
                        fi
                    fi
                done
            fi
        fi

	# extra steps (maybe not really necessary)
	#Change @executable_path for binary deps
	for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2 pyside-python${PYVER}.${SBKVER} shiboken-python${PYVER}.${SBKVER} intl.8; do
            lib="lib${l}.dylib"
            install_name_tool -change "${MACPORTS}/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
	done

	# NOT NECESSARY?
	if false; then
	    # Fix any remaining MacPorts libs
	    alllibs=( "${package}/Contents/Frameworks/"lib*.dylib )
	    for l in "${alllibs[@]+${alllibs[@]}}"; do
		lib="$(basename "$l")"
		install_name_tool -change "${MACPORTS}/lib/$lib" "@executable_path/../Frameworks/$lib" "$binary"
	    done
	fi
	
	if otool -L "$binary" | grep -F "${MACPORTS}"; then
            echo "Error: MacPorts libraries remaining in $binary, please check"
            exit 1
	fi

        $GSED -e "s@$MACPORTS@$MACRAND@g" -e "s@$HOMEBREW@$HOMEBREWRAND@g" -e "s@$LOCAL@$LOCALRAND@g" --in-place "$binary"
    fi
done

# One last check for MacPorts libs
alllibs=( "${package}/Contents/Frameworks/"lib*.dylib )
for l in "${alllibs[@]+${alllibs[@]}}"; do
    lib="$(basename "$l")"
    
    # NOT NECESSARY?
    if false; then
	for l2 in "${alllibs[@]+${alllibs[@]}}"; do
	    lib2="$(basename "$l2")"
	    install_name_tool -change "${MACPORTS}/lib/$lib2" "@executable_path/../Frameworks/$lib" "${package}/Contents/Frameworks/$lib"
	done
    fi
    
    if otool -L "${package}/Contents/Frameworks/$lib" | grep -F "${MACPORTS}"; then
        echo "Error: MacPorts libraries remaining in ${package}/Contents/Frameworks/$lib, please check"
        exit 1
    fi

    $GSED -e "s@$MACPORTS@$MACRAND@g" -e "s@$HOMEBREW@$HOMEBREWRAND@g" -e "s@$LOCAL@$LOCALRAND@g" --in-place "${package}/Contents/Frameworks/$lib"
done

# and now, obfuscate all the default paths in dynamic libraries
# and ImageMagick modules and config files

find "$pkglib" -type f -exec $GSED -e "s@$MACPORTS@$MACRAND@g" -e "s@$HOMEBREW@$HOMEBREWRAND@g" -e "s@$LOCAL@$LOCALRAND@g" --in-place {} \;

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mv "$package/Contents/MacOS/Natron" "$package/Contents/MacOS/Natron-bin"
    mv "$package/Contents/MacOS/NatronCrashReporter" "$package/Contents/MacOS/Natron"
    mv "$package/Contents/MacOS/NatronRenderer" "$package/Contents/MacOS/NatronRenderer-bin"
    mv "$package/Contents/MacOS/NatronRendererCrashReporter" "$package/Contents/MacOS/NatronRenderer"
    natronbins="$natronbins Natron-bin NatronRenderer-bin"

    echo "* Extract symbols from executables..."

    for bin in Natron-bin NatronRenderer-bin; do

	binary="$package/Contents/MacOS/$bin";
	#Dump symbols for breakpad before stripping
	if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
            if [ -x "$binary" ]; then
                STATUS=0
                $DSYMUTIL --arch=x86_64 "$binary" || STATUS=$?
                if [ $STATUS == 0 ] && [ -d "${binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${binary}.dSYM"  2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}-${CURRENT_DATE}${BPAD_TAG}-Mac-x86_64.sym" | head -10
                    rm -rf "${binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 $binary failed with exit status $STATUS"
                fi
                STATUS=0
                $DSYMUTIL --arch=i386 "$binary" || STATUS=$?
                if [ $STATUS == 0 ] && [ -d "${binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}-${CURRENT_DATE}${BPAD_TAG}-Mac-i386.sym" | head -10
                    rm -rf "${binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 $binary failed with exit status $STATUS"
                fi
            fi
	fi
    done
fi

if [ "$STRIP" = 1 ]; then
    echo "* Strip executables..."
    for bin in $natronbins $otherbins; do
        binary="$package/Contents/MacOS/$bin";

        if [ -x "$binary" ] && [ "$COMPILE_TYPE" != "debug" ]; then
            echo "* stripping $binary";
            # Retain the original binary for QA and use with the util 'atos'
            #mv -f "$binary" "${binary}_FULL";
            if [ "$BITS" = "Universal" ] && lipo "$binary" -verify_arch x86_64 i386; then
                # Extract each arch into a "thin" binary for stripping
                lipo "$binary" -thin x86_64 -output "${binary}_x86_64";
                lipo "$binary" -thin i386   -output "${binary}_i386";

                # Perform desired stripping on each thin binary.
                strip -S -x -r "${binary}_i386";
                strip -S -x -r "${binary}_x86_64";

                # Make the new universal binary from our stripped thin pieces.
                lipo -arch i386 "${binary}_i386" -arch x86_64 "${binary}_x86_64" -create -output "${binary}";


                # We're now done with the temp thin binaries, so chuck them.
                rm -f "${binary}_i386";
                rm -f "${binary}_x86_64";
# Do not strip, otherwise we get the following error "the __LINKEDIT segment does not cover the end of the file"
# else
#               strip -S -x -r "${binary}"
            fi
            #rm -f "${binary}_FULL";
        fi
    done
fi

# put links to the GCC libs in a separate directory, and compile the launcher which sets DYLD_LIBRARY_PATH to that directory
if [ "$LIBGCC" = 1 ]; then
    mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/libgcc"
    for l in $gcclibs; do
        lib="lib${l}.dylib"
        if [ -f "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/$lib" ]; then
            ln -s "../$lib" "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/libgcc/$lib"
        fi
    done
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron-driver"
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer-driver"
    # MACOSX_DEPLOYMENT_TARGET set in common.sh
    if [ "${BITS}" = "Universal" ]; then
        ${CC} -DLIBGCC -arch i386 -arch x86_64 -mmacosx-version-min=10.6 -isysroot /Developer/SDKs/MacOSX10.6.sdk "$INC_PATH/natron/NatronDyLd.c" -s -o "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron"
    else
        ${CC} -DLIBGCC "$INC_PATH/natron/NatronDyLd.c" -s -o "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron"
    fi
    cp "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer"
fi

# copy libc++ if necessary (on OSX 10.6 Snow Leopard with libc++ only)
# DYLD_LIBRARY_PATH should be set by NatronDyLd.c
if [ "$COPY_LIBCXX" = 1 ]; then
    mkdir -p "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/libcxx"
    cp /usr/lib/libc++.1.dylib "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/libcxx"
    cp /usr/lib/libc++abi.dylib "${TMP_PORTABLE_DIR}.app/Contents/Frameworks/libcxx"
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron-driver"
    mv "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer-driver"
    # MACOSX_DEPLOYMENT_TARGET set in common.sh
    if [ "${BITS}" = "Universal" ]; then
        ${CC} -DLIBCXX -arch i386 -arch x86_64 -mmacosx-version-min=10.6 -isysroot /Developer/SDKs/MacOSX10.6.sdk "$INC_PATH/natron/NatronDyLd.c" -o "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron"
    else
        ${CC} -DLIBCXX "$INC_PATH/natron/NatronDyLd.c" -o "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron"
    fi
    strip "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron"
    cp "${TMP_PORTABLE_DIR}.app/Contents/MacOS/Natron" "${TMP_PORTABLE_DIR}.app/Contents/MacOS/NatronRenderer"
fi


export PY_BIN="${MACPORTS}/bin/python${PYVER:-}"
export PYDIR="$pkglib/Python.framework/Versions/${PYVER}/lib/python${PYVER}"
. "$CWD/zip-python.sh"

# Install pip
if [ -x "${TMP_PORTABLE_DIR}.app/Contents/MacOS"/natron-python ]; then
    $CURL --remote-name --insecure http://bootstrap.pypa.io/get-pip.py
    "${TMP_PORTABLE_DIR}.app/Contents/MacOS"/natron-python get-pip.py
    rm get-pip.py
fi

# Run extra user provided pip install scripts
if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
    "${TMP_PORTABLE_DIR}.app/Contents/MacOS"/natron-python "$EXTRA_PYTHON_MODULES_SCRIPT" || true
fi


# Generate documentation
cd "$CWD"
bash gen-natron-doc.sh

cd "$TMP_BINARIES_PATH"


# At this point we can run Natron unit tests to check that the deployment is ok.
# Check manually that the Tests file is there, because gtimeout crashes if binary is not present
if [ ! -e "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests" ]; then
    echo "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests is not present! This is a bug"
    exit 1
fi
$TIMEOUT -s KILL 1800 "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests"
rm "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests"

echo "* Creating the disk image"
# Make the dmg
APP_NAME=Natron
DMG_FINAL="${APP_NAME}"
if [ "$NATRON_BUILD_CONFIG" = "SNAPSHOT" ]; then
    DMG_FINAL="${DMG_FINAL}-${NATRON_GIT_BRANCH}-${CURRENT_DATE}"
fi

DMG_FINAL="${DMG_FINAL}-${NATRON_VERSION_STRING}-${PKGOS}"
if [ "$COMPILE_TYPE" = "debug" ]; then
    DMG_FINAL="${DMG_FINAL}-debug"
fi

DMG_FINAL=${DMG_FINAL}.dmg
DMG_TMP=tmp${DMG_FINAL}


if [ -f "$TMP_BINARIES_PATH/splashscreen.png" ]; then
    DMG_BACK=$TMP_BINARIES_PATH/splashscreen.png
else 
    DMG_BACK=$TMP_BINARIES_PATH/splashscreen.jpg
fi

if [ ! -f "$DMG_BACK" ]; then
    echo "Could not find $DMG_BACK, make sure it was copied properly"
fi

if [ -e /Volumes/"${APP_NAME}" ]; then
    umount -f /Volumes/"${APP_NAME}"
fi
if [ -e "$DMG_TMP" ]; then
    rm -f "$DMG_TMP"
fi
if [ -e "$DMG_FINAL" ]; then
    rm -f "$DMG_FINAL"
fi

# Name the portable archive to the name of the application
mv "${PORTABLE_DIRNAME}.app" "${APP_NAME}.app"
hdiutil create -srcfolder "${APP_NAME}.app" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -volname "${APP_NAME}" "$DMG_TMP"
DIRNAME="$(hdiutil attach "$DMG_TMP" | grep -F /dev/disk| grep -F Apple_HFS|$GSED -e 's@.*/Volumes@/Volumes@')"
if [ -z "${DIRNAME:-}" ]; then
    echo "Error: cannot find disk image mount point"
    exit 1
fi
DISK="$(echo "$DIRNAME" | $GSED -e s@/Volumes/@@)"

# copy the background image
mkdir "/Volumes/${DISK}/.background"
DMG_BACK_NAME="$(basename "${DMG_BACK}")"
# use ImageMagick to reduce contrast and brighten the image
oiiotool -i "${DMG_BACK}" --powc 0.3 -o "/Volumes/${DISK}/.background/${DMG_BACK_NAME}"

# already copied the application during "hdiutil create"
#(cd Natron/App; tar cf - ${APP}) | (cd "$DIRNAME"; tar xvf -)

# create an alias to Applications
ln -sf /Applications "/Volumes/${DISK}/Applications"

# dmg window dimensions
dmg_width=$(oiiotool --info -v "${DMG_BACK}" | grep Exif:PixelXDimension | awk '{print $2}')
dmg_height=$(oiiotool --info -v "${DMG_BACK}" | grep Exif:PixelYDimension | awk '{print $2}')
dmg_topleft_x=200
dmg_topleft_y=200
dmg_bottomright_x=$((dmg_topleft_x + dmg_width))
dmg_bottomright_y=$((dmg_topleft_y + dmg_height))

#create the view of the dmg with the alias to Applications
cat > tmpScript <<EOT
tell application "Finder"
tell disk "$DISK"
open
set current view of container window to icon view
set toolbar visible of container window to false
set statusbar visible of container window to false
set the bounds of container window to {${dmg_topleft_x}, ${dmg_topleft_y}, ${dmg_bottomright_x}, ${dmg_bottomright_y}}
set theViewOptions to the icon view options of container window
set arrangement of theViewOptions to not arranged
set icon size of theViewOptions to 104
set background picture of theViewOptions to file ".background:${DMG_BACK_NAME}"
set position of item "${APP_NAME}.app" of container window to {120, 180}
set position of item "Applications" of container window to {400, 180}
# Force saving changes to the disk by closing and opening the window
close
open
update without registering applications
delay 5
eject
delay 5
end tell
end tell
EOT

# On Jenkins, this may fail once with the following error, the workaround is to re-run the script a second time.
# execution error: Finder got an error: Can’t get disk "Test DMG". (-1728)
FAIL=0
osascript tmpScript || FAIL=1
if [ "$FAIL" = "1" ]; then
    FAIL=0
    hdiutil attach "$DMG_TMP"
    osascript tmpScript
fi

rm tmpScript

# convert to compressed image, delete temp image
# UDBZ (bzip2) is supported on OS X >= 10.4
hdiutil convert "${DMG_TMP}" -format UDBZ -o "${DMG_FINAL}"

# Rename to the original portable dir name so that the unit tests script can be the same
# than the one used for other platforms
mv "${APP_NAME}.app" "${PORTABLE_DIRNAME}.app"

rm -rf "${DMG_TMP}"
rm -rf splashscreen.*


# echo "* Checking that the image can be mounted"
# DIRNAME=`hdiutil attach "${DMG_FINAL}" | grep -F /dev/disk | grep -F Apple_HFS|$GSED -e 's@.*/Volumes@/Volumes@'`
# if [ -z "${DIRNAME:-}" ]; then
#     echo "Error: cannot find disk image mount point for '$DMG_FINAL'"
#     exit 1
# else
#     DISK=`echo $DIRNAME | $GSED -e s@/Volumes/@@`
#     hdiutil eject "/Volumes/${DISK}"
# fi
if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi
mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/compressed_no_installer"
mv "${DMG_FINAL}" "${BUILD_ARCHIVE_DIRECTORY}/compressed_no_installer/"

echo "*** Artifacts:"
ls -R  "${BUILD_ARCHIVE_DIRECTORY}"
echo "*** OSX installer: done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
