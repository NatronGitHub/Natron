#!/bin/sh
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier
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

if [ $# -ne 1 ]; then
    echo "$0: Make a Natron.app that doesn't depend on MacPorts (can be used out of the build system too)"
    echo "Usage: $0 App/Natron.app"
    exit 1
fi
app="$1"
if [ ! -d "$app" ]; then
    echo "Error: application directory '$app' does not exist"
    exit 1
fi

MACPORTS="/opt/local"
PYVER="2.7"
SBKVER="1.2"

macdeployqt "${app}" || exit 1

#Copy and change exec_path of the whole Python framework with libraries
rm -rf "${app}/Contents/Frameworks/Python.framework"
mkdir -p "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib"
cp -r "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}" "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}" || exit 1
rm -rf "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Resources"
cp -r "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/Resources" "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Resources" || exit 1
rm -rf "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Python"
cp "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/Python" "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Python" || exit 1
chmod 755 "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Python"
install_name_tool -id "@executable_path/../Frameworks/Python.framework/Versions/${PYVER}/Python" "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/Python"
ln -sf "Versions/${PYVER}/Python" "${app}/Contents/Frameworks/Python.framework/Python" || exit 1

rm -rf "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/site-packages/"*
#rm -rf "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/__pycache__"
#rm -rf "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/*/__pycache__"
#FILES=`ls -l "${MACPORTS}/Library/Frameworks/Python.framework/Versions/${PYVER}/lib|awk" '{print $9}'`
#for f in FILES; do
#    #FILE=echo "{$f}" | sed "s/cpython-34.//g"
#    cp -r "$f" "${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/$FILE" || exit 1
#done

# a few elements of Natron.app/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/lib-dynload may load other libraries
DYNLOAD="${app}/Contents/Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/lib-dynload"
if [ ! -d "${DYNLOAD}" ]; then
    echo "lib-dynload not present"
    exit 1
fi
for mplib in `for i in "${DYNLOAD}"/*.so; do otool -L $i | fgrep "${MACPORTS}"; done |sort|uniq |awk '{print $1}'`; do
    if [ ! -f "$mplib" ]; then
	echo "missing python lib-dynload depend $mplib"
	exit 1
    fi
    lib=`echo $mplib | awk -F / '{print $NF}'`
    if [ ! -f "${app}/Contents/Frameworks/${lib}" ]; then
	cp "$mplib" "${app}/Contents/Frameworks/${lib}"
    fi
    for deplib in "${DYNLOAD}"/*.so; do
	install_name_tool -change "${mplib}" "@executable_path/../Frameworks/$lib" "$deplib"
    done
done


bin="${app}/Contents/MacOS/Natron"
# macdeployqt doesn't deal correctly with libs in ${MACPORTS}/lib/libgcc : handle them manually
LIBGCC=0
if otool -L "$bin" |fgrep "${MACPORTS}/lib/libgcc"; then
    LIBGCC=1
fi
if [ "$LIBGCC" = "1" ]; then
    for l in gcc_s.1 gomp.1 stdc++.6; do
        lib=lib${l}.dylib
        cp "${MACPORTS}/lib/libgcc/$lib" "${app}/Contents/Frameworks/$lib"
	install_name_tool -id "@executable_path/../Frameworks/$lib" "${app}/Contents/Frameworks/$lib"
    done
    for l in gcc_s.1 gomp.1 stdc++.6; do
        lib=lib${l}.dylib
	install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$bin"
	for deplib in "${app}/Contents/Frameworks/"*.dylib; do
	    install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
	done
    done
    # use gcc's libraries everywhere
    for l in gcc_s.1 gomp.1 stdc++.6; do
	lib="lib${l}.dylib"
	for deplib in "${app}/Contents/Frameworks/"*.framework/Versions/*/* "${app}/Contents/Frameworks/"lib*.dylib; do
            test -f "$deplib" && install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$deplib"
	done
    done
fi
for f in Python; do
    install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "$bin"
done

if otool -L "${app}/Contents/MacOS/Natron"  |fgrep "${MACPORTS}"; then
    echo "Error: MacPorts libraries remaining in $bin, please check"
    exit 1
fi

mv "${app}/Contents/PlugIns" "${app}/Contents/Plugins" || exit 1
rm "${app}/Contents/Resources/qt.conf" || exit 1

#Make a qt.conf file in Contents/Resources/
cat > "${app}/Contents/Resources/qt.conf" <<EOF
[Paths]
Plugins = Plugins
EOF

cp "Gui/Resources/Stylesheets/mainstyle.qss" "${app}/Contents/Resources/" || exit 1

cp "Renderer/NatronRenderer" "${app}/Contents/MacOS"
bin="${app}/Contents/MacOS/NatronRenderer"

#Change @executable_path for NatronRenderer deps
for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2 pyside-python${PYVER}.${SBKVER} shiboken-python${PYVER}.${SBKVER} intl.8; do
    lib=lib${l}.dylib
    install_name_tool -change "${MACPORTS}/lib/$lib" "@executable_path/../Frameworks/$lib" "$bin"
done
for f in QtNetwork QtCore; do
    install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$bin"
done
if [ "$LIBGCC" = "1" ]; then
    for l in gcc_s.1 stdc++.6; do
        lib=lib${l}.dylib
	install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$bin"
    done
fi

#Copy and change exec_path of the whole Python framework with libraries
for f in Python; do
    install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/${PYVER}/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/${PYVER}/${f}" "$bin"
done

if otool -L "$bin" |fgrep "${MACPORTS}"; then
    echo "Error: MacPorts libraries remaining in $bin, please check"
    exit 1
fi



#Do the same for crash reporter
if [ -f "CrashReporter/NatronCrashReporter" ]; then
    bin="${app}/Contents/MacOS/NatronCrashReporter"
    cp "CrashReporter/NatronCrashReporter" "$bin"
    for f in QtGui QtNetwork QtCore; do
	install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$bin"
    done
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
            lib="lib${l}.dylib"
	    install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$bin"
	done
    fi

    if otool -L "$bin" |fgrep "${MACPORTS}"; then
	echo "Error: MacPorts libraries remaining in $bin, please check"
	exit 1
    fi
fi

if [ -f "CrashReporterCLI/NatronRendererCrashReporter" ]; then
    bin="${app}/Contents/MacOS/NatronRendererCrashReporter"
    cp "CrashReporterCLI/NatronRendererCrashReporter" "$bin"
    for f in QtNetwork QtCore; do
	install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$bin"
    done
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
            lib=lib${l}.dylib
	    install_name_tool -change "${MACPORTS}/lib/libgcc/$lib" "@executable_path/../Frameworks/$lib" "$bin"
	done
    fi

    if otool -L "$bin" |fgrep "${MACPORTS}"; then
	echo "Error: MacPorts libraries remaining in $bin, please check"
	exit 1
    fi
fi

# install PySide in site-packages
PYSIDE="Frameworks/Python.framework/Versions/${PYVER}/lib/python${PYVER}/site-packages/PySide"
rm -rf "${app}/Contents/${PYSIDE}"
cp -r "${MACPORTS}/Library/${PYSIDE}" "${app}/Contents/${PYSIDE}" || exit 1

QT_LIBS="QtCore QtGui QtNetwork QtOpenGL QtDeclarative QtHelp QtMultimedia QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtXml QtWebKit QtXmlPatterns"

for qtlib in $QT_LIBS ;do
    bin="${app}/Contents/${PYSIDE}/${qtlib}.so"
    install_name_tool -id "@executable_path/../${PYSIDE}/${qtlib}.so" "$bin"
    for f in $QT_LIBS; do
        install_name_tool -change "${MACPORTS}/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$bin"
    done

    for l in  pyside-python${PYVER}.${SBKVER} shiboken-python${PYVER}.${SBKVER}; do
        dylib="lib${l}.dylib"
        install_name_tool -change "${MACPORTS}/lib/$dylib" "@executable_path/../Frameworks/$dylib" "$bin"
    done
    # use gcc's libraries everywhere
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
	    lib="lib${l}.dylib"
	    install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$bin"
	done
    fi
done

