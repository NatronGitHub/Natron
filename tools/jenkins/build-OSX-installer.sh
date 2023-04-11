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
#set -x # Print commands and their arguments as they are executed.
#set -v # Prints shell input lines as they are read.
shopt -s extglob

echo "*** OSX installer..."

source common.sh
source manageBuildOptions.sh
source manageLog.sh

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd > /dev/null
}

updateBuildOptions

echo "*** macOS version:"
macosx=$(uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|')
case "${macosx}" in
22) echo "macOS 13.x    Ventura";;
21) echo "macOS 12.x    Monterey";;
20) echo "macOS 11.x    Big Sur";;
19) echo "macOS 10.15.x Catalina";;
18) echo "macOS 10.14.x Mojave";;
17) echo "macOS 10.13.x High Sierra";;
16) echo "macOS 10.12.x Sierra";;
15) echo "OS X  10.11.x El Capitan";;
14) echo "OS X  10.10.x Yosemite";;
13) echo "OS X  10.9.x  Mavericks";;
12) echo "OS X  10.8.x  Mountain Lion";;
11) echo "OS X  10.7.x  Lion";;
10) echo "OS X  10.6.x  Snow Leopard";;
9) echo "OS X  10.5.x  Leopard";;
8) echo "OS X  10.4.x  Tiger";;
7) echo "OS X  10.3.x  Panther";;
6) echo "OS X  10.2.x  Jaguar";;
5) echo "OS X  10.1.x  Puma";;
*) echo "unknown macOS version: ${macosx}";;
esac

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    natronbins=(Natron NatronRenderer NatronProjectConverter natron-python NatronCrashReporter NatronRendererCrashReporter Tests)
else
    natronbins=(Natron NatronRenderer NatronProjectConverter natron-python Tests)
fi
otherbins=(ffmpeg ffprobe iconvert idiff igrep iinfo exrheader tiffinfo) # iv maybe?

if [ "${SDK_HOME}" = "${MACPORTS}" ] && [ ! -h "${SDK_HOME}/Library/lib" ]; then
    echo "Error: missing link \"${SDK_HOME}/Library/lib\" to help macdeployqt find the Python framework, please execute the following and relaunch:"
    echo "sudo ln -s Frameworks \"${SDK_HOME}/Library/lib\""
    exit 1
fi

## Code signing
###############
# Docs about code signing:
# - Code Signing Resources https://developer.apple.com/forums/thread/707080
# - Resolving errSecInternalComponent errors https://developer.apple.com/forums/thread/712005 (when signing over ssh)
#CODE_SIGN_IDENTITY=${CODE_SIGN_IDENTITY:-8ZN4B9KT99}
CODE_SIGN_IDENTITY=${CODE_SIGN_IDENTITY:--}
BUNDLE_ID=fr.inria.Natron
if [ "${CODE_SIGN_IDENTITY}" = "-" ]; then
    # If identity is the single letter "-" (dash), ad-hoc signing is performed. Ad-hoc signing does not use
    # an identity at all, and identifies exactly one instance of code. Significant restrictions apply to the
    # use of ad-hoc signed code; consult documentation before using this.
    echo "code signing with ad-hoc signing."
    CODESIGN=codesign
elif [ -z "${CODE_SIGN_IDENTITY}" ]; then
    echo "no code signing."
    CODESIGN=true
elif (security find-identity -v -p codesigning | grep -F -q "${CODE_SIGN_IDENTITY}"); then
    echo "code signing identity ${CODE_SIGN_IDENTITY} found"
    CODESIGN=codesign
else
    echo "code signing identity ${CODE_SIGN_IDENTITY} not found: Disabling code signing."
    CODESIGN=true
fi
if [ -z "${CODE_SIGN_IDENTITY}" ] && [ "$(uname -m)" = "arm64" ]; then
    echo "Error: Apple silicon Mac binaries must be signed to be runnable"
    echo "see also:"
    echo "- https://mjtsai.com/blog/2020/08/19/apple-silicon-macs-to-require-signed-code/"
    echo "- https://eclecticlight.co/2020/08/22/apple-silicon-macs-will-require-signed-code/"
    exit 1
fi

CODE_SIGN_OPTS=(--force --sign "${CODE_SIGN_IDENTITY}")
CODE_SIGN_MAIN_OPTS=()
# entitlements for hardened runtime, see https://kb.froglogic.com/squish/mac/troubleshoot/hardened-runtime/
# We definitely need to disable library validation, because of external OpenFX plugins.
# Some of these plugins may require setting DYLD_LIBRARY_PATH
if [ "${macosx}" -ge 18 ]; then # Started with 10.14
    # see https://github.com/JetBrains/compose-multiplatform/issues/2887#issuecomment-1473702563
    cat > "${TMP_BINARIES_PATH}/natron.entitlements" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>com.apple.security.cs.allow-jit</key>
    <true/>
    <key>com.apple.security.cs.allow-unsigned-executable-memory</key>
    <true/>
	<key>com.apple.security.cs.disable-library-validation</key>
	<true/>
	<key>com.apple.security.cs.allow-dyld-environment-variables</key>
	<true/>
</dict>
</plist>
EOF
    CODE_SIGN_MAIN_OPTS+=(--options runtime)
    CODE_SIGN_OPTS+=(--timestamp)
    CODE_SIGN_MAIN_OPTS+=(--entitlements "${TMP_BINARIES_PATH}/natron.entitlements")
fi
#if [ "${macosx}" -ge 13 ]; then
#    CODE_SIGN_OPTS+=(--deep)
#fi


DUMP_SYMS=/usr/local/bin/dump_syms
DSYMUTIL=dsymutil
if [ "${COMPILER}" = "clang" ] || [ "${COMPILER}" = "clang-omp" ]; then
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
    if [ -x /opt/local/bin/dsymutil-mp-11 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-11
    fi
    if [ -x /opt/local/bin/dsymutil-mp-14 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-14
    fi
    if [ -x /opt/local/bin/dsymutil-mp-15 ]; then
        DSYMUTIL=/opt/local/bin/dsymutil-mp-15
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
    #if [ -f "${TMP_BINARIES_PATH}/natron-fullversion.txt" ]; then
    #    GET_VER=`cat "${TMP_BINARIES_PATH}/natron-fullversion.txt"`
    #    if [ "$GET_VER" != "" ]; then
    #        TAG=$GET_VER
    #    fi
    #fi
fi

if [ ! -d "${TMP_BINARIES_PATH}" ]; then
    echo "=====> NO ${TMP_BINARIES_PATH}, NOTHING TO DO HERE!"
    exit 1
fi

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi

package="${TMP_PORTABLE_DIR}.app"
if [ -d "${package}" ]; then
    echo "${package} already exists, removing it"
    chmod -R u+w "${package}"
    rm -rf "${package}"
fi
mkdir -p "${package}/Contents/Frameworks"
mkdir -p "${package}/Contents/Plugins"
mkdir -p "${package}/Contents/MacOS"
mkdir -p "${package}/Contents/Plugins/OFX/Natron"

cp -a "${TMP_BINARIES_PATH}/bin/"* "${package}/Contents/MacOS/"
cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/"*.ofx.bundle "${package}/Contents/Plugins/OFX/Natron/" || true


cp -a "${TMP_BINARIES_PATH}/PyPlugs" "${package}/Contents/Plugins/"
cp -a "${TMP_BINARIES_PATH}/Resources" "${package}/Contents/"

NATRON_BIN_VERSION="${NATRON_VERSION_SHORT}"
if [ "${NATRON_BIN_VERSION}" = "" ]; then
    NATRON_BIN_VERSION="2.0"
fi

# move Info.plist and PkgInfo, see "copy Info.plist and PkgInfo" in build-natron.sh
if [ ! -f "${package}/Contents/Info.plist" ]; then
    if [ -f "${package}/Contents/MacOS/Info.plist" ]; then
        mv "${package}/Contents/MacOS/Info.plist" "${package}/Contents/Info.plist"
    elif [ -f "${INC_PATH}/natron/Info.plist" ]; then
        ${GSED} "s/_REPLACE_VERSION_/${NATRON_BIN_VERSION:-}/" < "${INC_PATH}/natron/Info.plist" > "${package}/Contents/Info.plist"
    fi
fi
if [ -f "${package}/Contents/MacOS/PkgInfo" ]; then
    mv "${package}/Contents/MacOS/PkgInfo" "${package}/Contents/PkgInfo"
else
    echo "APPLNtrn" > "${package}/Contents/PkgInfo"
fi

#Dump symbols for breakpad before stripping
for bin in IO Misc CImg Arena GMIC Shadertoy Extra Magick OCL; do
    ofx_binary="${package}/Contents/Plugins/OFX/Natron/${bin}.ofx.bundle/Contents/MacOS/${bin}.ofx"

    #Strip binary
    if [ -x "$ofx_binary" ] && [ "$COMPILE_TYPE" != "debug" ]; then
        if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
            STATUS=0
            if [ "${BITS}" = "64" ] || [ "${BITS}" = "Universal" ]; then
                $DSYMUTIL --arch=x86_64 "$ofx_binary" || STATUS=$?
                if [ ${STATUS} == 0 ] && [ -d "${ofx_binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${ofx_binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-Mac-x86_64.sym" | head -10
                    rm -rf "${ofx_binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 $ofx_binary failed with exit status ${STATUS}"
                fi
            fi
            STATUS=0
            if [ "${BITS}" = "32" ] || [ "${BITS}" = "Universal" ]; then
                $DSYMUTIL --arch=i386 "$ofx_binary" || STATUS=$?
                if [ ${STATUS} == 0 ] && [ -d "${ofx_binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${ofx_binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-Mac-i386.sym" | head -10
                    rm -rf "${ofx_binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=i386 $ofx_binary failed with exit status ${STATUS}"
                fi
            fi
        fi

        echo "* stripping $ofx_binary";
        # Retain the original binary for QA and use with the util 'atos'
        #mv -f "$ofx_binary" "${binary}_FULL";
        if [ "${BITS}" = "Universal" ] && lipo "$ofx_binary" -verify_arch i386 x86_64; then
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

PLUGINDIR="${package}/Contents/Plugins"
# move all libraries to the same place, put symbolic links instead
for plugin in "${PLUGINDIR}/OFX/Natron"/*.ofx.bundle; do
    cd "${plugin}/Contents/Libraries"
    # relative path to the ${package}/Contents/Frameworks dir
    frameworks="../../../../../../Frameworks"
    for lib in lib*.dylib; do
        if [ -f "${lib}" ]; then
            if [ -f "$frameworks/${lib}" ]; then
                rm "${lib}"
            else
                mv "${lib}" "$frameworks/${lib}"
            fi
            ln -sf "$frameworks/${lib}" "${lib}"
        fi
    done
    if [ "${COMPILER}" = "gcc" ]; then # use gcc's libraries everywhere
        for l in ${gcclibs}; do
            lib="lib${l}.dylib"
            for deplib in "${plugin}"/Contents/MacOS/*.ofx "${plugin}"/Contents/Libraries/lib*dylib ; do
                if [ -f "${deplib}" ]; then
                    install_name_tool -change "/usr/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                    install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                fi
            done
        done
    fi
    if [ "${COMPILER}" = "clang-omp" ]; then # use clang's OpenMP libraries everywhere
        for l in ${omplibs}; do
            lib="lib${l}.dylib"
            for deplib in "${plugin}"/Contents/MacOS/*.ofx "${plugin}"/Contents/Libraries/lib*dylib ; do
                if [ -f "${deplib}" ]; then
                    install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                fi
            done
        done
    fi
done

fi

SDK_HOME="${SDK_HOME:-/opt/local}"
LOCAL="/usr/local"
HOMEBREW="${LOCAL}"
if [ -e "/brew2/local" ]; then
    HOMEBREW="/brew2/local"
elif [ -e "/opt/homebrew" ]; then
    HOMEBREW="/opt/homebrew"
fi
SBKVER="1.2"
QT_VERSION_MAJOR=${QT_VERSION_MAJOR:-4}
QTDIR=${QTDIR:-${SDK_HOME}/libexec/qt${QT_VERSION_MAJOR}}
PYTHON_HOME=${PYTHON_HOME:-${SDK_HOME}/Library/Frameworks/Python.framework/Versions/${PYVER}}
## all Qt frameworks:
case "${QT_VERSION_MAJOR}" in
4)
    qt_libs=(Qt3Support QtCLucene QtCore QtDBus QtDeclarative QtDesigner QtDesignerComponents QtGui QtHelp QtMultimedia QtNetwork QtOpenGL QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtWebKit QtXml QtXmlPatterns)
    ## Qt frameworks used by Natron + PySide + Qt plugins:
    #qt_libs=(Qt3Support QtCLucene QtCore QtDBus QtDeclarative QtDesigner QtGui QtHelp QtMultimedia QtNetwork QtOpenGL QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtWebKit QtXml QtXmlPatterns)
    qt_frameworks_dir="${QTDIR}/Library/Frameworks"
    ;;
5)
    qt_libs=(Qt3DAnimation Qt3DCore Qt3DExtras Qt3DInput Qt3DLogic Qt3DQuick Qt3DQuickAnimation Qt3DQuickExtras Qt3DQuickInput Qt3DQuickRender Qt3DQuickScene2D Qt3DRender QtBluetooth QtCharts QtConcurrent QtCore QtDBus QtDataVisualization QtDesigner QtDesignerComponents QtGamepad QtGui QtHelp QtLocation QtMacExtras QtMultimedia QtMultimediaQuick QtMultimediaWidgets QtNetwork QtNetworkAuth QtNfc QtOpenGL QtPdf QtPdfWidgets QtPositioning QtPositioningQuick QtPrintSupport QtQml QtQmlModels QtQmlWorkerScript QtQuick QtQuickControls2 QtQuickParticles QtQuickShapes QtQuickTemplates2 QtQuickTest QtQuickWidgets QtRemoteObjects QtRepParser QtScript QtScriptTools QtScxml QtSensors QtSerialBus QtSerialPort QtSql QtSvg QtTest QtTextToSpeech QtUiPlugin QtWebChannel QtWebEngine QtWebEngineCore QtWebEngineWidgets QtWebSockets QtWidgets QtXml QtXmlPatterns)
    qt_frameworks_dir="${QTDIR}/lib"
    ;;
esac
STRIP=1

# macdeployqt only works if the package name has the same name as the executable inside:
# ERROR: Could not find bundle binary for "/Users/devernay/Development/workspace/tmp/tmp_deploy/Natron-RB-2.3-201909281526-05aaefe-64-no-installer.app"
# ERROR: "error: otool: can't open file:  (No such file or directory)
app_for_macdeployqt="$(dirname "${package}")/Natron.app"
[ -e  "${app_for_macdeployqt}" ] && rm -rf "${app_for_macdeployqt}"
# make a temporary link to Natron.app
ln -sf "${package}" "${app_for_macdeployqt}"
MACDEPLOYQT_OPTS=(-no-strip)
MACDEPLOYQT_OPTS+=(-libpath="${SDK_HOME}/lib/nss" -libpath="${SDK_HOME}/lib/nspr")
for bin in "${natronbins[@]}" "${otherbins[@]}"; do
    binary="$app_for_macdeployqt/Contents/MacOS/${bin}"
    if [ ! -e "${binary}" ] && [ -e "${SDK_HOME}/bin/${bin}" ]; then
        cp "${SDK_HOME}/bin/${bin}" "${binary}"
    fi
    if [ -e "${binary}" ]; then
        MACDEPLOYQT_OPTS+=(-executable="${binary}")
    fi
done
if [ "${QT_VERSION_MAJOR}" = 5 ]; then
    if [ -n "${CODE_SIGN_IDENTITY}" ]; then
        echo "* executing macdeployqt without code signing"
        #MACDEPLOYQT_OPTS+=(-codesign="${CODE_SIGN_IDENTITY}")
    fi
    #if [ "${macosx}" -ge 18 ]; then
    #    MACDEPLOYQT_OPTS+=(-hardened-runtime)
    #fi
fi
echo Executing: "${QTDIR}"/bin/macdeployqt "${app_for_macdeployqt}" "${MACDEPLOYQT_OPTS[@]}"
#echo "********** DISABLED! press return"
#read
"${QTDIR}"/bin/macdeployqt "${app_for_macdeployqt}" "${MACDEPLOYQT_OPTS[@]}"
# remove temp link
rm  "${app_for_macdeployqt}"

natron_binary="${package}/Contents/MacOS/Natron"
libdir="Frameworks"
pkglib="${package}/Contents/${libdir}"

if [ ! -x "${natron_binary}" ]; then
    echo "Error: ${natron_binary} does not exist or is not an executable"
    exit 1
fi

# macdeployqt does not deal correctly with libs in ${SDK_HOME}/lib/libgcc : handle them manually
LIBGCC=0
if otool -L "${natron_binary}" | grep -F -q "${SDK_HOME}/lib/libgcc"; then
    LIBGCC=1
fi
COPY_LIBCXX=0
if otool -L "${natron_binary}" | grep -F -q "/usr/lib/libc++"; then
    # libc++ appeared in on Lion 10.7 (osmajor=11)
    if [ "${macosx}" -lt 11 ]; then
        COPY_LIBCXX=1
    fi
fi
if [ -f "${pkglib}/libc++.1.dylib" ] || [ -f "${pkglib}/libc++abi.1.dylib" ]; then
    echo "Error: ${pkglib}/libc++.1.dylib or ${pkglib}/libc++abi.1.dylib was copied by macdeployqt"
    exit 1
fi

echo "*** Copy and change exec_path of the whole Python framework with libraries..."
rm -rf "${pkglib}/Python.framework"
mkdir -p "${pkglib}/Python.framework/Versions/${PYVER}/lib"
ln -sf "${PYVER}" "${pkglib}/Python.framework/Versions/Current"
cp -r "${PYTHON_HOME}/lib/python${PYVER}" "${pkglib}/Python.framework/Versions/Current/lib/python${PYVER}"
rm -rf "${pkglib}/Python.framework/Versions/Current/Resources"
cp -r "${PYTHON_HOME}/Resources" "${pkglib}/Python.framework/Versions/Current/Resources"
rm -rf "${pkglib}/Python.framework/Versions/Current/Python"
cp "${PYTHON_HOME}/Python" "${pkglib}/Python.framework/Versions/Current/Python"
cp -r "${PYTHON_HOME}/include" "${pkglib}/Python.framework/Versions/Current/"
chmod -R u+w "${pkglib}/Python.framework"
chmod 755 "${pkglib}/Python.framework/Versions/Current/Python"
install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/${PYVER}/Python" "${pkglib}/Python.framework/Versions/Current/Python"
ln -sf "Versions/Current/Python" "${pkglib}/Python.framework/Python"

rm -rf "${pkglib}/Python.framework/Versions/Current/lib/python${PYVER}/site-packages/"*
#FILES=`ls -l "${PYTHON_HOME}/lib|awk" '{print $9}'`
#for f in FILES; do
#    #FILE=echo "{$f}" | ${GSED} "s/cpython-34.//g"
#    cp -r "$f" "${pkglib}/Python.framework/Versions/Current/lib/$FILE"
#done

# a few elements of ${package}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/lib-dynload may load other libraries
DYNLOAD="${pkglib}/Python.framework/Versions/Current/lib/python${PYVER}/lib-dynload"
if [ ! -d "${DYNLOAD}" ]; then
    echo "lib-dynload not present"
    exit 1
fi

echo "*** Copying and fixing ${SDK_HOME} dependencies..."
MPLIBS0="$(for i in "${pkglib}/Python.framework/Versions/Current/Python" "${DYNLOAD}"/*.so; do otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
# also add first-level and second-level dependencies 
MPLIBS1="$(for i in ${MPLIBS0}; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
MPLIBS="$(for i in ${MPLIBS1}; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
for mplib in ${MPLIBS}; do
    if [ ! -f "${mplib}" ]; then
        echo "missing python lib-dynload depend ${mplib}"
        exit 1
    fi
    lib="$(echo "${mplib}" | awk -F / '{print $NF}')"
    if [ "${lib}" = "libc++.1.dylib" ] || [ "${lib}" = "libc++abi.1.dylib" ]; then
        echo "Error: ${lib} being copied from ${mplib}"
        exit 1
    fi
    if [ ! -f "${pkglib}/${lib}" ]; then
        cp "${mplib}" "${pkglib}/${lib}"
        chmod +w "${pkglib}/${lib}"
    fi
    install_name_tool -id "@executable_path/../Frameworks/${lib}" "${pkglib}/${lib}"
    for deplib in "${pkglib}/Python.framework/Versions/Current/Python" "${DYNLOAD}"/*.so; do
	if [ -f "${deplib}" ]; then
            install_name_tool -change "${mplib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
        fi
    done
done

# also fix newly-installed libs
for mplib in ${MPLIBS}; do
    lib="$(echo "${mplib}" | awk -F / '{print $NF}')"
    for mplibsub in ${MPLIBS}; do
        deplib="$(echo "${mplibsub}" | awk -F / '{print $NF}')"
        install_name_tool -change "${mplib}" "@executable_path/../Frameworks/${lib}" "${pkglib}/${deplib}"
    done
done

# qt4-mac@4.8.7_2 doesn't deploy plugins correctly. macdeployqt ${package} -verbose=2 gives:
# Log: Deploying plugins from "/opt/local/libexec/qt4/Library/Frameworks/plugins" 
# see https://trac.macports.org/ticket/49344
if [ ! -d "${package}/Contents/PlugIns" ] && [ -d "${QTDIR}/share/plugins" ]; then
    echo "Warning: Qt plugins not copied by macdeployqt, see https://trac.macports.org/ticket/49344. Copying them now."
    mkdir -p "${package}/Contents/PlugIns/sqldrivers"
    cp -r "${QTDIR}/share/plugins"/{graphicssystems,iconengines,imageformats} "${package}/Contents/PlugIns/"
    #cp -r "${QTDIR}/share/plugins/sqldrivers/libqsqlite.dylib" "${package}/Contents/PlugIns/sqldrivers/"
    for binary in "${package}/Contents/PlugIns"/*/*.dylib; do
        if [ -f "${binary}" ]; then
            chmod +w "${binary}"
            for lib in libjpeg.8.dylib libmng.2.dylib libtiff.5.dylib libQGLViewer.2.dylib; do
                install_name_tool -change "${SDK_HOME}/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
            done
            for f in "${qt_libs[@]}"; do
                install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
            done
            if otool -L "${binary}" | grep -F "${SDK_HOME}"; then
                echo "Error: MacPorts libraries remaining in ${binary}, please check"
                exit 1
            fi
        fi
    done
fi
# Now, because plugins were not installed (see see https://trac.macports.org/ticket/49344 ),
# their dependencies were not installed either (e.g. QtSvg and QtXml for imageformats/libqsvg.dylib)
# Besides, PySide may also load other Qt Frameworks. We have to make sure they are all present
echo "*** Installing Qt plugins dependencies"
for qtlib in "${qt_libs[@]}"; do
    if [ ! -d "${package}/Contents/Frameworks/${qtlib}.framework" ] && [ -f "${qt_frameworks_dir}/${qtlib}.framework/Versions/${QT_VERSION_MAJOR}/${qtlib}" ]; then
        fw="${package}/Contents/Frameworks/${qtlib}.framework"
        binary="${fw}/Versions/${QT_VERSION_MAJOR}/${qtlib}"
        mkdir -p "$(dirname "${binary}")"
        cp "${qt_frameworks_dir}/${qtlib}.framework/Versions/${QT_VERSION_MAJOR}/${qtlib}" "${binary}"
        (cd "${fw}/Versions"; ln -sf "${QT_VERSION_MAJOR}" Current)
        (cd "${fw}"; ln -sf "Versions/Current/${qtlib}" .)
        # copy resources
        if [ -e "${qt_frameworks_dir}/${qtlib}.framework/Versions/${QT_VERSION_MAJOR}/Resources" ]; then
            cp -a "${qt_frameworks_dir}/${qtlib}.framework/Versions/${QT_VERSION_MAJOR}/Resources" "${fw}/Versions/${QT_VERSION_MAJOR}/"
            (cd "${fw}"; ln -sf "Versions/Current/Resources" .)
        fi

        chmod +w "${binary}"
        install_name_tool -id "@executable_path/../Frameworks/${qtlib}.framework/Versions/${QT_VERSION_MAJOR}/${qtlib}" "${binary}"
        for f in "${qt_libs[@]}"; do
            install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
        done
        for lib in libcrypto.3.dylib libdbus-1.3.dylib libpng16.16.dylib libssl.3.dylib libz.1.dylib; do
            install_name_tool -change "${SDK_HOME}/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
        done
        for lib in libcrypto.3.dylib libssl.3.dylib; do
            install_name_tool -change "${SDK_HOME}/libexec/openssl3/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
        done
    fi
done

if [ "${QT_VERSION_MAJOR}" = 4 ]; then
    echo "*** Repairing Qt4 frameworks"
    # macdeployqt from Qt4 forgets the Info.plist (necessary for code signing)
    # For the correct Framework Bundle structure, see https://developer.apple.com/library/archive/documentation/MacOSX/Conceptual/BPFrameworks/Concepts/FrameworkAnatomy.html
    for f in "${qt_libs[@]}"; do
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework" ]; then
            echo "Error: ${f}.framework is missing"
            exit 1
            #continue
        fi
        echo "fixing ${f}.framework"
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Versions/Current" ]; then
            echo "fixing ${f}.framework/Versions/Current"
            (cd "${package}/Contents/Frameworks/${f}.framework/Versions"; ln -sf "${QT_VERSION_MAJOR}" Current)
        fi
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources" ]; then
            echo "adding ${f}.framework/Versions/Current/Resources"
            if [ -d "${package}/Contents/Frameworks/${f}.framework/Resources" ]; then
                echo "moving  ${f}.framework/Resources to ${f}.framework/Versions/Current/Resources"
                mv "${package}/Contents/Frameworks/${f}.framework/Resources" "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources"
            else
                echo "creating ${f}.framework/Versions/Current/Resources"
                mkdir "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources"
            fi
        fi
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources/Info.plist" ]; then
            echo "adding ${f}.framework/Versions/Current/Resources/Info.plist"
            if [ -e "${qt_frameworks_dir}/${f}.framework/Contents/Info.plist" ]; then
                echo "copying ${f}.framework/Versions/Current/Resources/Info.plist from ${qt_frameworks_dir}/${f}.framework/Contents/Info.plist"
                cp "${qt_frameworks_dir}/${f}.framework/Contents/Info.plist" "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources/Info.plist"
            fi
            if [ -e "${qt_frameworks_dir}/${f}.framework/Versions/Current/Resources/Info.plist" ]; then
                echo "copying ${f}.framework/Versions/Current/Resources/Info.plist from ${qt_frameworks_dir}/${f}.framework/Versions/Current/Resources/Info.plist"
                cp "${qt_frameworks_dir}/${f}.framework/Versions/Current/Resources/Info.plist" "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources/Info.plist"
            fi
            if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Versions/Current/Resources/Info.plist" ]; then
                echo "Error: couldn't find Info.plist for ${qt_frameworks_dir}/${f}.framework"
                exit 1
            fi
        fi
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/${f}" ]; then
            echo "creating link ${f}.framework/${f}"
            (cd "${package}/Contents/Frameworks/${f}.framework"; ln -sf "Versions/Current/${f}" .)
        fi
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Resources" ]; then
            echo "creating link ${f}.framework/Resources"
            (cd "${package}/Contents/Frameworks/${f}.framework"; ln -sf "Versions/Current/Resources" .)
        fi
        if [ ! -e "${package}/Contents/Frameworks/${f}.framework/Resources/Info.plist" ]; then
            echo "Error: ${f}.framework/Resources/Info.plist is missing"
            exit 1
        fi
    done
fi

if [ "${LIBGCC}" = "1" ]; then
    echo "*** Copying and fixing GCC libs"
    for l in ${gcclibs}; do
        lib="lib${l}.dylib"
        cp "${SDK_HOME}/lib/libgcc/${lib}" "${pkglib}/${lib}"
        install_name_tool -id "@executable_path/../Frameworks/${lib}" "${pkglib}/${lib}"
    done
    for l in ${gcclibs}; do
        lib="lib${l}.dylib"
        #install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${natron_binary}"
        for deplib in "${pkglib}/"*.dylib "${pkglib}/"*".framework/Versions/${QT_VERSION_MAJOR}/"*; do
            if [ -f "${deplib}" ]; then
                install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
            fi
        done
    done
    # use gcc's libraries everywhere
    for l in ${gcclibs}; do
        lib="lib${l}.dylib"
        for deplib in "${pkglib}/"*.framework/Versions/*/* "${pkglib}/"lib*.dylib; do
            if [ -f "${deplib}" ]; then
                install_name_tool -change "/usr/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
            fi
        done
    done
fi
if [ "${COMPILER}" = "clang-omp" ]; then
    echo "*** Copying and fixing OpenMP support lib"
    for l in ${omplibs}; do
        lib="lib${l}.dylib"
        cp "${SDK_HOME}/lib/libomp/${lib}" "${pkglib}/${lib}"
        install_name_tool -id "@executable_path/../Frameworks/${lib}" "${pkglib}/${lib}"
    done
    for l in ${omplibs}; do
        lib="lib${l}.dylib"
        #install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${natron_binary}"
        for deplib in "${pkglib}/"*.dylib "${pkglib}/"*".framework/Versions/${QT_VERSION_MAJOR}/"*; do
            if [ -f "${deplib}" ]; then
                install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
            fi
        done
    done
fi
# shellcheck disable=SC2043
#for f in Python; do
#    install_name_tool -change "${SDK_HOME}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "${natron_binary}"
#done

# fix library ids
echo "*** Fixing library IDs"
pushd "${pkglib}"
for deplib in *.framework/Versions/*/* lib*.dylib; do
    if [ -f "${deplib}" ]; then
        install_name_tool -id "@executable_path/../Frameworks/${deplib}" "${deplib}"
    fi
done
popd # pushd "${pkglib}"


echo "*** Copying poppler resources"
#cp "Gui/Resources/Stylesheets/mainstyle.qss" "${package}/Contents/Resources/"
cp -a "${SDK_HOME}/share/poppler" "${package}/Contents/Resources/"


# install PySide in site-packages
if [ "${QT_VERSION_MAJOR}" = 4 ]; then
    PYSIDE_PKG=PySide
else
    PYSIDE_PKG=PySide2
fi

echo "*** Installing ${PYSIDE_PKG}..."

# the python "system"
pysys="darwin"
# The python-specific part in the pyside and shiboken library names
if [ "${PYV}" = 3 ]; then
    pypart=".cpython-${PYVERNODOT}-${pysys}"
else
    pypart="-python${PYVER}"
fi

PYLIB="Python.framework/Versions/${PYVER}/lib/python${PYVER}"
PYSIDE="${PYLIB}/site-packages/${PYSIDE_PKG}"
rm -rf "${package}/Contents/Frameworks/${PYSIDE}"
cp -a "${SDK_HOME}/Library/Frameworks/${PYSIDE}" "${package}/Contents/Frameworks/${PYSIDE}"
if [ "${QT_VERSION_MAJOR}" = 4 ]; then
    PYSHIBOKEN="${PYLIB}/site-packages/shiboken.so"
    rm -rf "${package}/Contents/Frameworks/${PYSHIBOKEN}"
    cp "${SDK_HOME}/Library/Frameworks/${PYSHIBOKEN}" "${package}/Contents/Frameworks/${PYSHIBOKEN}"
    # fix shiboken.so
    lib_shiboken="shiboken${pypart}.${SBKVER}"
    dylib_shiboken="lib${lib_shiboken}.dylib"
    lib_pyside="pyside${pypart}.${SBKVER}"
    dylib_pyside="lib${l}.dylib"
    binary="${package}/Contents/Frameworks/${PYSHIBOKEN}"
    install_name_tool -change "${SDK_HOME}/lib/${dylib_shiboken}" "@executable_path/../Frameworks/${dylib_shiboken}" "${binary}"
    install_name_tool -change "${SDK_HOME}/lib/${dylib_pyside}" "@executable_path/../Frameworks/${dylib_pyside}" "${binary}"
    # install pyside and shiboken libs, and fix deps from Qt
    for l in  "${lib_shiboken}" "${lib_pyside}"; do
        dylib="lib${l}.dylib"
        binary="${package}/Contents/Frameworks/${dylib}"
        cp "${SDK_HOME}/lib/${dylib}" "${binary}"
        install_name_tool -id "@executable_path/../Frameworks/${dylib}" "${binary}"
        install_name_tool -change "${SDK_HOME}/lib/${dylib_shiboken}" "@executable_path/../Frameworks/${dylib_shiboken}" "${binary}"
        install_name_tool -change "${SDK_HOME}/lib/${dylib_pyside}" "@executable_path/../Frameworks/${dylib_pyside}" "${binary}"
        for f in "${qt_libs[@]}"; do
            install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
        done
    done
else
    PYSHIBOKEN="${PYLIB}/site-packages/shiboken2"
    cp -a "${SDK_HOME}/Library/Frameworks/${PYSHIBOKEN}" "${package}/Contents/Frameworks/${PYSHIBOKEN}"
    rm -rf "${package}/Contents/Frameworks/${PYSHIBOKEN}/docs"
    (cd "${package}/Contents/Frameworks" && ln -sf "${PYLIB}/site-packages/shiboken2/libshiboken2"*.dylib .)
    #install_name_tool -rpath "${SDK_HOME}/Library/Frameworks/${PYSHIBOKEN}" "@executable_path/../Frameworks/${PYSHIBOKEN}" "${natron_binary}"
    (cd "${package}/Contents/Frameworks" && ln -sf "${PYSIDE}/libpyside2"*.dylib .)
    #install_name_tool -rpath "${SDK_HOME}/Library/Frameworks/${PYSIDE}" "@executable_path/../Frameworks/${PYSIDE}" "${natron_binary}"
    for binary in  "${package}/Contents/Frameworks/${PYSHIBOKEN}"/lib*.dylib "${package}/Contents/Frameworks/${PYSIDE}"/lib*.dylib; do
        for f in "${qt_libs[@]}"; do
            install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
        done
        dylib=$(basename "${binary}")
        install_name_tool -id "@executable_path/../Frameworks/${dylib}" "${binary}"
        install_name_tool -change "${SDK_HOME}/lib/${dylib}" "@executable_path/../Frameworks/${dylib}" "${binary}"
    done

fi



echo "*** Cleaning Python..."
# remove pyo files
#find "${package}/Contents/Frameworks/${PYLIB}" -type f -name '*.pyo' -exec rm {} \;
# prune large python files
(cd "${package}/Contents/Frameworks/${PYLIB}"; xargs rm -rf || true) < "${INC_PATH}/python-exclude.txt"

if [ "${PYV}" = 3 ]; then
    pycfg="${package}/Contents/Frameworks/${PYLIB}/config-${PYVER}-${pysys}"
else
    pycfg="${package}/Contents/Frameworks/${PYLIB}/config"
fi
pushd "${pycfg}"
chmod -x Makefile Setup Setup.config Setup.local config.c config.c.in python.o || true
rm -f "libpython${PYVER}.a" "libpython${PYVER}.dylib"
# yes, the static library is actually a link to the dynamic library on OS X (check before doing this on other archs)
ln -sf ../../../Python "libpython${PYVER}.a"
ln -sf ../../../Python "libpython${PYVER}.dylib"
popd # pushd "${package}/Contents/Frameworks/${PYLIB}/config"

(cd "${package}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib"; ln -sf ../Python "libpython${PYVER}.dylib")

echo "*** Fixing sonames in ${PYSIDE_PKG}..."
pushd "${package}/Contents/Frameworks/${PYSIDE}"
for qtlib in "${qt_libs[@]}" ;do
    for binary in "${qtlib}.so" "${qtlib}${pypart}.so"; do
        if [ ! -e "${binary}" ]; then
            continue
        fi
        install_name_tool -id "@executable_path/../Frameworks/${PYSIDE}/${qtlib}.so" "${binary}"
        for f in "${qt_libs[@]}"; do
            install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
        done

        for l in  pyside${pypart}.${SBKVER} shiboken${pypart}.${SBKVER}; do
            dylib="lib${l}.dylib"
            install_name_tool -change "${SDK_HOME}/lib/${dylib}" "@executable_path/../Frameworks/${dylib}" "${binary}"
        done
        if [ "${LIBGCC}" = "1" ]; then
            for l in ${gcclibs}; do
                lib="lib${l}.dylib"
                install_name_tool -change "/usr/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
            done
        fi
        
        if otool -L "${binary}" | grep -F "${SDK_HOME}"; then
            echo "Error: MacPorts libraries remaining in ${binary}, please check"
            exit 1
        fi
    done
done
popd

#############################################################################
# Other binaries

for bin in "${otherbins[@]}"; do
    # fix macports libs
    MPLIBS0="$(otool -L "${package}/Contents/MacOS/${bin}" | grep -F "${SDK_HOME}/lib" | grep -F -v ':' |sort|uniq |awk '{print $1}')"
    # also add first-level and second-level dependencies 
    MPLIBS1="$(for i in ${MPLIBS0}; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
    MPLIBS="$(for i in ${MPLIBS1}; do echo "$i"; otool -L "$i" | grep -F "${SDK_HOME}/lib" | grep -F -v ':'; done |sort|uniq |awk '{print $1}')"
    for mplib in ${MPLIBS}; do
        if [ ! -f "${mplib}" ]; then
            echo "missing ${bin} depend ${mplib}"
            exit 1
        fi
        lib="$(echo "${mplib}" | awk -F / '{print $NF}')"
        if [ ! -f "${pkglib}/${lib}" ]; then
            echo "copying missing lib ${lib}"
            if [ "${lib}" = "libc++.1.dylib" ] || [ "${lib}" = "libc++abi.1.dylib" ]; then
                echo "Error: ${lib} being copied from ${mplib}"
                exit 1
            fi
            cp "${mplib}" "${pkglib}/${lib}"
            chmod +w "${pkglib}/${lib}"
        fi
        install_name_tool -change "${mplib}" "@executable_path/../Frameworks/${lib}" "${package}/Contents/MacOS/${bin}"
    done
done

# Fix rpath
for bin in "${natronbins[@]}" "${otherbins[@]}"; do
    binary="${package}/Contents/MacOS/${bin}"
    if [ ! -x "${binary}" ]; then
        continue
    fi
    if [ "${LIBGCC}" = "1" ]; then
        for l in ${gcclibs}; do
            lib="lib${l}.dylib"
            install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}" || true
        done
    fi
    if [ "${COMPILER}" = "clang-omp" ]; then
        for l in ${omplibs}; do
            lib="lib${l}.dylib"
            install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}" || true
        done
    fi
    for f in Python; do
        install_name_tool -change "${SDK_HOME}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "${binary}" || true
    done
    if [ "${QT_VERSION_MAJOR}" = 5 ]; then
        install_name_tool -rpath "${SDK_HOME}/Library/Frameworks/${PYSHIBOKEN}" "@executable_path/../Frameworks/${PYSHIBOKEN}" "${binary}" || true
        install_name_tool -rpath "${SDK_HOME}/Library/Frameworks/${PYSIDE}" "@executable_path/../Frameworks/${PYSIDE}" "${binary}" || true
    fi
done

echo "*** Obfuscate the MacPorts/Homebrew/SDK paths..."
# generate a pseudo-random string which has the same length as ${SDK_HOME}
RANDSTR="R7bUU6jiFvqrPy6zLVPwIC3b93R2b1RG2qD3567t8hC3b93R2b1RG2qD3567t8h"
MACPORTSRAND=${RANDSTR:0:${#MACPORTS}}
HOMEBREWRAND=${RANDSTR:1:${#HOMEBREW}}
LOCALRAND=${RANDSTR:2:${#LOCAL}}
SDKRAND=${RANDSTR:3:${#SDK_HOME}}

for deplib in "${pkglib}/"*.framework/Versions/*/* "${pkglib}/"lib*.dylib; do
    if [ ! -f "${deplib}" ]; then
        continue
    fi
    if otool -L "${deplib}" | grep -F "${MACPORTS}"; then
        echo "Error: MacPorts libraries remaining in ${deplib}, please check"
        exit 1
    fi
    if otool -L "${deplib}" | grep -F "${HOMEBREW}"; then
        echo "Error: Homebrew libraries remaining in ${deplib}, please check"
        exit 1
    fi
    if otool -L "${deplib}" | grep -F "${SDK_HOME}"; then
        echo "Error: SDK libraries remaining in ${deplib}, please check"
        exit 1
    fi
done

for bin in "${natronbins[@]}" "${otherbins[@]}"; do
    binary="${package}/Contents/MacOS/${bin}"
    if [ -f "${binary}" ]; then
        if otool -L "${binary}" | grep -F "${MACPORTS}"; then
            echo "Error: MacPorts libraries remaining in ${binary}, please check"
            exit 1
        fi
        if otool -L "${binary}" | grep -F "${HOMEBREW}"; then
            echo "Error: Homebrew libraries remaining in ${binary}, please check"
            exit 1
        fi
        if otool -L "${binary}" | grep -F "${SDK_HOME}"; then
            echo "Error: SDK libraries remaining in ${binary}, please check"
            exit 1
        fi
        # check if rpath does not contains some path, see https://stackoverflow.com/a/15394738
        rpath=( $(otool -l "${binary}" | grep -A 3 LC_RPATH | grep path|awk '{ print $2 }') )
        if [ ${#rpath[@]} -eq 0 ] || [[ ! " ${rpath[*]} " =~ " @loader_path/../${libdir} " ]]; then
            echo "Warning: The runtime search path in ${binary} does not contain \"@loader_path/../${libdir}\". Please set it in your Xcode project, or link the binary with the flags -Xlinker -rpath -Xlinker \"@loader_path/../${libdir}\" . Fixing it!"
            #exit 1
            install_name_tool -add_rpath "@loader_path/../${libdir}" "${binary}"
        fi
        # remove remnants of llvm path (libraries were copied already)
        if [ ${#rpath[@]} -gt 0 ]; then
            for r in "${rpath[@]}"; do
                case "$r" in
                    "${SDK_HOME}"/libexec/llvm-*/lib|"${SDK_HOME}"/lib)
                        install_name_tool -delete_rpath "$r" "${binary}"
                        ;;
                esac
            done
        fi
        # Test dirs
        if [ ! -d "${pkglib}" ]; then
            mkdir "${pkglib}"
        fi

        ### FIX FRAMEWORKS (must be done before dylibs)
        
        # maydeployqt only fixes the main binary, fix others too
        for f in "${qt_libs[@]}"; do
            install_name_tool -change "${qt_frameworks_dir}/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" "${binary}"
        done

        # Copy and change exec_path of the whole Python framework with libraries
        # shellcheck disable=SC2043
        for f in Python; do
            install_name_tool -change "${SDK_HOME}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "${binary}"
        done


        ### FIX DYLIBS
        
        if [ "${LIBGCC}" = "1" ]; then
            for l in ${gcclibs}; do
                lib="lib${l}.dylib"
                install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
            done
        fi

        if [ "${LIBGCC}" = "1" ]; then
            for l in ${gcclibs}; do
                lib="lib${l}.dylib"
                cp "${SDK_HOME}/lib/libgcc/${lib}" "${pkglib}/${lib}"
                install_name_tool -id "@executable_path/../Frameworks/${lib}" "${pkglib}/${lib}"
            done
            for l in ${gcclibs}; do
                lib="lib${l}.dylib"
                install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
                install_name_tool -change "/usr/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
                for deplib in "${pkglib}/"*".dylib" "${pkglib}/"*".framework/Versions/${QT_VERSION_MAJOR}/"*; do
                    if [ -f "${deplib}" ]; then
                        install_name_tool -change "${SDK_HOME}/lib/libgcc/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                    fi
                done
            done
            # use gcc's libraries everywhere
            for l in ${gcclibs}; do
                lib="lib${l}.dylib"
                for deplib in "${pkglib}/"*.framework/Versions/*/* "${pkglib}/"lib*.dylib; do
                    if [ -f "${deplib}" ]; then
                        install_name_tool -change "/usr/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                    fi
                done
            done
        fi
        if [ "${COMPILER}" = "clang-omp" ]; then
            for l in ${omplibs}; do
                lib="lib${l}.dylib"
                cp "${SDK_HOME}/lib/libomp/${lib}" "${pkglib}/${lib}"
                install_name_tool -id "@executable_path/../Frameworks/${lib}" "${pkglib}/${lib}"
            done
            for l in ${omplibs}; do
                lib="lib${l}.dylib"
                install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
                for deplib in "${pkglib}/"*.dylib "${pkglib}/"*".framework/Versions/${QT_VERSION_MAJOR}/"*; do
                    if [ -f "${deplib}" ]; then
                        install_name_tool -change "${SDK_HOME}/lib/libomp/${lib}" "@executable_path/../Frameworks/${lib}" "${deplib}"
                    fi
                done
            done
        fi

        LIBADD=()

        #############################
        # test if ImageMagick is used
        if otool -L "${binary}"  | grep -F libMagick > /dev/null; then
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
            # if I get this right, ${GSED} substitutes in the exe the occurrences of IMAGEMAGICKVER
            # into the actual value retrieved from the package.
            # We don't need this because we use MAGICKCORE_PACKAGE_VERSION declared in the <magick/magick-config.h>
            # ${GSED} -e "s,IMAGEMAGICKVER,$IMAGEMAGICKVER,g" --in-place $pkgbin/DisparityKillerM

            # copy the ImageMagick libraries (.la and .so)
            cp -r "$IMAGEMAGICKLIB/ImageMagick-$IMAGEMAGICKVER" "${pkglib}/"
            if [ ! -d "${pkglib}/share" ]; then
                mkdir "${pkglib}/share"
            fi
            cp -r "$IMAGEMAGICKSHARE/ImageMagick-$IMAGEMAGICKMAJ" "${pkglib}/share/"

            # see http://stackoverflow.com/questions/7577052/bash-empty-array-expansion-with-set-u
            LIBADD=(${LIBADD[@]+"${LIBADD[@]}"} "${pkglib}/ImageMagick-$IMAGEMAGICKVER"/modules-*/*/*.so)
            #WITH_IMAGEMAGICK=yes
        fi

        # expand glob patterns in LIBADD (do not double-quote ${LIBADD})
        #LIBADD="$(echo ${LIBADD})"

        # Find out the library dependencies
        # (i.e. ${LOCAL} or ${MACPORTS}), then loop until no changes.
        nfiles=0
        alllibs=()
        endl=true
        while $endl; do
            #echo -e "\033[1mLooking for dependencies.\033[0m Round" $a
            # see http://stackoverflow.com/questions/7577052/bash-empty-array-expansion-with-set-u
            libs=( $(otool -L "${pkglib}"/*.dylib "${LIBADD[@]+${LIBADD[@]}}" "${binary}" 2>/dev/null | grep -F compatibility | cut -d\( -f1 | grep -e "${LOCAL}"'\|'"${HOMEBREW}"'\|'"${MACPORTS}"'\|'"${SDK_HOME}" | sort | uniq) )
            if [ -n "${libs[*]-}" ]; then
                # cp option '-n': do not overwrite any existing file (mainly for pyside and shiboken, which were copied before)
                cp -n "${libs[@]}" "${pkglib}" || true
                # shellcheck disable=SC2012
                alllibs=( $(ls "${alllibs[@]+${alllibs[@]}}" "${libs[@]}" | sort | uniq) )
            fi
            # shellcheck disable=SC2012
            nnfiles="$(ls "${pkglib}" | wc -l)"
            if [ "$nnfiles" = "$nfiles" ]; then
                endl=false
            else
                nfiles=$nnfiles
            fi
        done

        # all the libraries were copied, now change the names...
        ## We use @rpath instead of @executable_path/../${libdir} because it's shorter
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
                for f in  "${pkglib}"/* "${LIBADD[@]+${LIBADD[@]}}" "${binary}"; do
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
        for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2 pyside${pypart}.${SBKVER} shiboken${pypart}.${SBKVER} intl.8; do
                lib="lib${l}.dylib"
            install_name_tool -change "${SDK_HOME}/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
        done

        # NOT NECESSARY?
        if false; then
            # Fix any remaining MacPorts libs
            alllibs=( "${package}/Contents/Frameworks/"lib*.dylib )
            for l in "${alllibs[@]+${alllibs[@]}}"; do
                lib="$(basename "$l")"
                install_name_tool -change "${SDK_HOME}/lib/${lib}" "@executable_path/../Frameworks/${lib}" "${binary}"
	        done
	    fi

        if otool -L "${binary}" | grep -F "${MACPORTS}"; then
            echo "Error: MacPorts libraries remaining in ${binary}, please check"
            exit 1
        fi
        if otool -L "${binary}" | grep -F "${HOMEBREW}"; then
            echo "Error: Homebrew libraries remaining in ${binary}, please check"
            exit 1
        fi
        if otool -L "${binary}" | grep -F "${SDK_HOME}"; then
            echo "Error: SDK libraries remaining in ${binary}, please check"
            exit 1
        fi

        ${GSED} -e "s@${MACPORTS}@${MACPORTSRAND}@g" -e "s@${HOMEBREW}@${HOMEBREWRAND}@g" -e "s@${LOCAL}@${LOCALRAND}@g" -e "s@${SDK_HOME}@${SDKRAND}@g" --in-place "${binary}"
    fi
done

# One last check for MacPorts libs
echo "*** Checking for remaining MacPorts depends"
alllibs=( "${package}/Contents/Frameworks/"lib*.dylib )
for l in "${alllibs[@]+${alllibs[@]}}"; do
    lib="$(basename "$l")"
    
    # NOT NECESSARY?
    if false; then
	for l2 in "${alllibs[@]+${alllibs[@]}}"; do
	    lib2="$(basename "${l2}")"
	    install_name_tool -change "${SDK_HOME}/lib/${lib2}" "@executable_path/../Frameworks/${lib}" "${package}/Contents/Frameworks/${lib}"
	done
    fi
    
    if otool -L "${package}/Contents/Frameworks/${lib}" | grep -F "${MACPORTS}"; then
        echo "Error: MacPorts libraries remaining in ${package}/Contents/Frameworks/${lib}, please check"
        exit 1
    fi
    if otool -L "${package}/Contents/Frameworks/${lib}" | grep -F "${HOMEBREW}"; then
        echo "Error: Homebrew libraries remaining in ${package}/Contents/Frameworks/${lib}, please check"
        exit 1
    fi
    if otool -L "${package}/Contents/Frameworks/${lib}" | grep -F "${SDK_HOME}"; then
        echo "Error: SDK libraries remaining in ${package}/Contents/Frameworks/${lib}, please check"
        exit 1
    fi

    ${GSED} -e "s@${MACPORTS}@${MACPORTSRAND}@g" -e "s@$HOMEBREW@${HOMEBREWRAND}@g" -e "s@${LOCAL}@${LOCALRAND}@g" -e "s@${SDK_HOME}@${SDKRAND}@g" --in-place "${package}/Contents/Frameworks/${lib}"
done

# and now, obfuscate all the default paths in dynamic libraries
# and ImageMagick modules and config files

find "${pkglib}" -type f -exec ${GSED} -e "s@${MACPORTS}@${MACPORTSRAND}@g" -e "s@$HOMEBREW@${HOMEBREWRAND}@g" -e "s@${LOCAL}@${LOCALRAND}@g" -e "s@${SDK_HOME}@${SDKRAND}@g" --in-place {} \;

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mv "${package}/Contents/MacOS/Natron" "${package}/Contents/MacOS/Natron-bin"
    mv "${package}/Contents/MacOS/NatronCrashReporter" "${package}/Contents/MacOS/Natron"
    mv "${package}/Contents/MacOS/NatronRenderer" "${package}/Contents/MacOS/NatronRenderer-bin"
    mv "${package}/Contents/MacOS/NatronRendererCrashReporter" "${package}/Contents/MacOS/NatronRenderer"
    natronbins+=(Natron-bin NatronRenderer-bin)

    echo "* Extract symbols from executables..."

    for bin in Natron-bin NatronRenderer-bin; do

	binary="${package}/Contents/MacOS/${bin}";
	#Dump symbols for breakpad before stripping
	if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
            if [ -x "${binary}" ]; then
                STATUS=0
                $DSYMUTIL --arch=x86_64 "${binary}" || STATUS=$?
                if [ ${STATUS} == 0 ] && [ -d "${binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${binary}.dSYM"  2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}-${CURRENT_DATE}${BPAD_TAG}-Mac-x86_64.sym" | head -10
                    rm -rf "${binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 ${binary} failed with exit status ${STATUS}"
                fi
                STATUS=0
                $DSYMUTIL --arch=i386 "${binary}" || STATUS=$?
                if [ ${STATUS} == 0 ] && [ -d "${binary}.dSYM" ]; then
                    # dump_syms produces many errors. redirect stderr to stdout (pipe), then stdout to file
                    $DUMP_SYMS "${binary}.dSYM" 2>&1 > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${bin}-${CURRENT_DATE}${BPAD_TAG}-Mac-i386.sym" | head -10
                    rm -rf "${binary}.dSYM"
                else
                    echo "Warning: $DSYMUTIL --arch=x86_64 ${binary} failed with exit status ${STATUS}"
                fi
            fi
	fi
    done
fi

if [ "${STRIP}" = 1 ]; then
    echo "*** Strip executables..."
    for bin in "${natronbins[@]}" "${otherbins[@]}"; do
        binary="${package}/Contents/MacOS/${bin}";

        if [ -x "${binary}" ] && [ "$COMPILE_TYPE" != "debug" ]; then
            echo "* stripping ${binary}";
            # Retain the original binary for QA and use with the util 'atos'
            #mv -f "${binary}" "${binary}_FULL";
            if [ "${BITS}" = "Universal" ] && lipo "${binary}" -verify_arch x86_64 i386; then
                # Extract each arch into a "thin" binary for stripping
                lipo "${binary}" -thin x86_64 -output "${binary}_x86_64";
                lipo "${binary}" -thin i386   -output "${binary}_i386";

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
if [ "${LIBGCC}" = 1 ]; then
    mkdir -p "${package}/Contents/Frameworks/libgcc"
    for l in ${gcclibs}; do
        lib="lib${l}.dylib"
        if [ -f "${package}/Contents/Frameworks/${lib}" ]; then
            ln -sf "../${lib}" "${package}/Contents/Frameworks/libgcc/${lib}"
        fi
    done
    mv "${package}/Contents/MacOS/Natron" "${package}/Contents/MacOS/Natron-driver"
    mv "${package}/Contents/MacOS/NatronRenderer" "${package}/Contents/MacOS/NatronRenderer-driver"
    # MACOSX_DEPLOYMENT_TARGET set in common.sh
    if [ "${BITS}" = "Universal" ]; then
        ${CC} -DLIBGCC -arch i386 -arch x86_64 -mmacosx-version-min=10.6 -isysroot /Developer/SDKs/MacOSX10.6.sdk "${INC_PATH}/natron/NatronDyLd.c" -s -o "${package}/Contents/MacOS/Natron"
    else
        ${CC} -DLIBGCC "${INC_PATH}/natron/NatronDyLd.c" -s -o "${package}/Contents/MacOS/Natron"
    fi
    cp "${package}/Contents/MacOS/Natron" "${package}/Contents/MacOS/NatronRenderer"
fi

# copy libc++ if necessary (on OSX 10.6 Snow Leopard with libc++ only)
# DYLD_LIBRARY_PATH should be set by NatronDyLd.c
if [ "$COPY_LIBCXX" = 1 ]; then
    mkdir -p "${package}/Contents/Frameworks/libcxx"
    cp /usr/lib/libc++.1.dylib "${package}/Contents/Frameworks/libcxx"
    cp /usr/lib/libc++abi.dylib "${package}/Contents/Frameworks/libcxx"
    mv "${package}/Contents/MacOS/Natron" "${package}/Contents/MacOS/Natron-driver"
    mv "${package}/Contents/MacOS/NatronRenderer" "${package}/Contents/MacOS/NatronRenderer-driver"
    # MACOSX_DEPLOYMENT_TARGET set in common.sh
    if [ "${BITS}" = "Universal" ]; then
        ${CC} -DLIBCXX -arch i386 -arch x86_64 -mmacosx-version-min=10.6 -isysroot /Developer/SDKs/MacOSX10.6.sdk "${INC_PATH}/natron/NatronDyLd.c" -o "${package}/Contents/MacOS/Natron"
    else
        ${CC} -DLIBCXX "${INC_PATH}/natron/NatronDyLd.c" -o "${package}/Contents/MacOS/Natron"
    fi
    strip "${package}/Contents/MacOS/Natron"
    cp "${package}/Contents/MacOS/Natron" "${package}/Contents/MacOS/NatronRenderer"
fi


export PY_BIN="${SDK_HOME}/bin/python${PYVER:-}"
export PYDIR="${pkglib}/Python.framework/Versions/${PYVER}/lib/python${PYVER}"
. "${CWD}/zip-python.sh"
NATRON_PYTHON="${package}/Contents/MacOS/natron-python"

# Install pip
if [ -x "${NATRON_PYTHON}" ]; then
    #"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${CODE_SIGN_MAIN_OPTS[@]}" "${NATRON_PYTHON}"
    #"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${package}/Contents/Frameworks/Python.framework" "${DYNLOAD}/"*.so
    echo "*** Installing pip"
    pushd "${PYDIR}/../.."
    if [ "${PYV}" = "2" ]; then
        ${CURL} --remote-name --insecure "https://bootstrap.pypa.io/pip/${PYVER}/get-pip.py"
    else
        ${CURL} --remote-name --insecure "https://bootstrap.pypa.io/get-pip.py"
    fi
    PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
    #"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" get-pip.py
    "${PY_EXE}" get-pip.py --target "${PYDIR}/site-packages"
    rm get-pip.py
    # Install qtpy
    if [ "${QT_VERSION_MAJOR}" = 4 ]; then
        # Qt4 support was dropped after QtPy 1.11.2
        "${NATRON_PYTHON}" -m pip install qtpy==1.11.2
        # qtpy bug fix for Qt4
        ${GSED} -i "s/^except ImportError:/except (ImportError, PythonQtError):/" "${PYDIR}/site-packages/qtpy/__init__.py"
    else
        "${PY_EXE}" -m pip install qtpy --upgrade --target "${PYDIR}/site-packages"
    fi
    # Useful Python packages
    "${PY_EXE}" -m pip install future six --upgrade --target "${PYDIR}/site-packages"
    # No psutil wheel is available on 10.6, and I don't know how to build a universal wheel
    case "${macosx}" in
    9|10|11|12)
        # 10.5-10.8
        true
        ;;
    *)
        "${PY_EXE}" -m pip install psutil --upgrade --target "${PYDIR}/site-packages"
        ;;
    esac
    # Run extra user provided pip install scripts
    if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
        "${PY_EXE}" "$EXTRA_PYTHON_MODULES_SCRIPT" || true
    fi
    # "${PY_EXE}" -m compileall -f "${PYDIR}/site-packages"
    "${PY_EXE}" -m compileall -f -q "${PYDIR}"
fi

# Must be done before signing frameworks, because these are sealed resources
if [ -x "${NATRON_PYTHON}" ]; then
    echo "* Signing python shared objects"
    find "${DYNLOAD}" -iname '*.so' -or -iname '*.dylib'| while read libfile; do
        "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${libfile}"
    done
    find "${PYDIR}/site-packages" -iname '*.so' -or -iname '*.dylib'| while read libfile; do
        "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${libfile}"
    done
    rm -rf "${PYDIR}/__pycache__" "${PYDIR}/*/__pycache__" || true
fi

echo "*** Signing frameworks"
pushd "${package}/Contents/Frameworks"
if [ "${QT_VERSION_MAJOR}" = 4 ]; then
    for f in "${qt_libs[@]}"; do
        if [ -e "${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}" ]; then
            "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${f}.framework/Versions/${QT_VERSION_MAJOR}/${f}"
        fi
    done
    for f in *.framework; do
        "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "$f" || true # fails on Qt4 frameworks
    done
else
    "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" ./*.framework
fi
popd

echo "*** Signing libraries"
(cd "${package}/Contents/Frameworks"; "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" ./*.dylib)

echo "* Signing OFX plugins"
(cd "${package}/Contents/Plugins/OFX/Natron"; "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" ./*.bundle)

echo "*** Signing extra binaries"
(cd "${package}/Contents/MacOS"; "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${CODE_SIGN_MAIN_OPTS[@]}" !(Natron))
echo "*** Signing app"
"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${CODE_SIGN_MAIN_OPTS[@]}" "${package}"

if [ -f "${pkglib}/libc++.1.dylib" ] || [ -f "${pkglib}/libc++abi.1.dylib" ]; then
    echo "Error: ${pkglib}/libc++.1.dylib or ${pkglib}/libc++abi.1.dylib was copied by mistake"
    exit 1
fi


# # "${CODESIGN}" again for safety
# if [ "${QT_VERSION_MAJOR}" = 5 ]; then
#     # extra "${CODESIGN}" using macdeployqt
#     # macdeployqt only works if the package name has the same name as the executable inside:
#     app_for_macdeployqt="$(dirname "${package}")/Natron.app"
#     [ -f  "${app_for_macdeployqt}" ] && rm "${app_for_macdeployqt}"
#     # make a temporary link to Natron.app
#     ln -sf "${package}" "${app_for_macdeployqt}"
#     echo Executing: "${QTDIR}"/bin/macdeployqt "${app_for_macdeployqt}" "${MACDEPLOYQT_OPTS[@]}"
#     "${QTDIR}"/bin/macdeployqt "${app_for_macdeployqt}" "${MACDEPLOYQT_OPTS[@]}"
#     # remove temp link
#     rm  "${app_for_macdeployqt}"
# fi
# "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" \
#  "${package}/Contents/Frameworks"/*.framework \
#  "${package}/Contents/Frameworks"/*.dylib \
#  "${DYNLOAD}/"*.so \
#  "${PYDIR}/site-packages/"*/*.so
# "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${CODE_SIGN_MAIN_OPTS[@]}" \
#  "${package}" \
#  "${package}/Contents/MacOS"/*

# Generate documentation
cd "${CWD}"
echo "*** Generating doc"
bash gen-natron-doc.sh

cd "${TMP_BINARIES_PATH}"


# At this point we can run Natron unit tests to check that the deployment is ok.
echo "*** Run basic unit tests"
# Check manually that the Tests file is there, because gtimeout crashes if binary is not present
if [ ! -e "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests" ]; then
    echo "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests is not present! This is a bug"
    exit 1
fi
# Tests segfault on exit on Qt5
${TIMEOUT} -s KILL 1800 "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests" || [ "${QT_VERSION_MAJOR}" = 5 ] 
rm "${PORTABLE_DIRNAME}.app/Contents/MacOS/Tests"

# Issue with __pycache__ being generated inside the python directories:
# https://developer.apple.com/forums/thread/75060
# This may prevent further runs of Natron...
if [ -x "${NATRON_PYTHON}" ]; then
    rm -rf "${PYDIR}/__pycache__" "${PYDIR}/*/__pycache__" || true
    "${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${package}/Contents/Frameworks/Python.framework"
    # Make the python directories read-only so that  no __pycache__ is created,
    # because that would break the signature.
    chmod -R a-w "${PYDIR}"
fi

echo "*** Signing app again to seal resources"
"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" "${CODE_SIGN_MAIN_OPTS[@]}" "${package}"

echo "*** Checking app signature"
"${CODESIGN}" --verify --deep --strict --verbose=2 "${package}"


echo "*** Creating the disk image"
# Make the dmg
APP_NAME=Natron

DMG_FINAL="${INSTALLER_BASENAME}.dmg"
DMG_TMP="tmp${DMG_FINAL}"


if [ -f "${TMP_BINARIES_PATH}/splashscreen.png" ]; then
    DMG_BACK="${TMP_BINARIES_PATH}/splashscreen.png"
else 
    DMG_BACK="${TMP_BINARIES_PATH}/splashscreen.jpg"
fi

if [ ! -f "${DMG_BACK}" ]; then
    echo "Could not find ${DMG_BACK}, make sure it was copied properly"
fi

if [ -e /Volumes/"${APP_NAME}" ]; then
    umount -f /Volumes/"${APP_NAME}"
fi
if [ -e "${DMG_TMP}" ]; then
    rm -f "${DMG_TMP}"
fi
if [ -e "${DMG_FINAL}" ]; then
    rm -f "${DMG_FINAL}"
fi

# Name the portable archive to the name of the application
mv "${PORTABLE_DIRNAME}.app" "${APP_NAME}.app"
hdiutil create -srcfolder "${APP_NAME}.app" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -volname "${APP_NAME}" "${DMG_TMP}"
DIRNAME="$(hdiutil attach "${DMG_TMP}" | grep -F /dev/disk| grep -F Apple_HFS|${GSED} -e 's@.*/Volumes@/Volumes@')"
if [ -z "${DIRNAME:-}" ]; then
    echo "Error: cannot find disk image mount point"
    exit 1
fi
DISK="$(echo "$DIRNAME" | ${GSED} -e s@/Volumes/@@)"

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
dmg_width=$(identify -format '%w' "${DMG_BACK}")
dmg_height=$(identify -format '%h' "${DMG_BACK}")
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
if [ "${FAIL}" = "1" ]; then
    FAIL=0
    hdiutil attach "${DMG_TMP}"
    osascript tmpScript
fi

rm tmpScript

# convert to compressed image, delete temp image
# UDBZ (bzip2) is supported on OS X >= 10.4
hdiutil convert "${DMG_TMP}" -format UDBZ -o "${DMG_FINAL}"

"${CODESIGN}" "${CODE_SIGN_OPTS[@]}" -i "${BUNDLE_ID}" "${DMG_FINAL}"

# Rename to the original portable dir name so that the unit tests script can be the same
# than the one used for other platforms
mv "${APP_NAME}.app" "${PORTABLE_DIRNAME}.app"

rm -rf "${DMG_TMP}"
#rm -rf splashscreen.*


# echo "* Checking that the image can be mounted"
# DIRNAME=`hdiutil attach "${DMG_FINAL}" | grep -F /dev/disk | grep -F Apple_HFS|${GSED} -e 's@.*/Volumes@/Volumes@'`
# if [ -z "${DIRNAME:-}" ]; then
#     echo "Error: cannot find disk image mount point for '${DMG_FINAL}'"
#     exit 1
# else
#     DISK=`echo $DIRNAME | ${GSED} -e s@/Volumes/@@`
#     hdiutil eject "/Volumes/${DISK}"
# fi
if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi
mkdir -p "${BUILD_ARCHIVE_DIRECTORY}"
mv "${DMG_FINAL}" "${BUILD_ARCHIVE_DIRECTORY}/"

echo "*** Artifacts:"
ls -R  "${BUILD_ARCHIVE_DIRECTORY}"
echo "*** OSX installer: done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
