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

#Usage MKJOBS=4 CONFIG=relwithdebinfo BRANCH=workshop PLUGINDIR="..."  ./build-natron.sh

source `pwd`/common.sh || exit 1

cd "$CWD/build" || exit 1

if [ "$BRANCH" == "workshop" ]; then
    NATRON_BRANCH=$BRANCH
else
    NATRON_BRANCH=$NATRON_GIT_TAG
fi

git clone $GIT_NATRON
cd Natron || exit 1
git checkout $NATRON_BRANCH || exit 1
git submodule update -i --recursive || exit 1

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V=`git log|head -1|awk '{print $2}'`
sed -i "" -e "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" $CWD/commits-hash.sh || exit 1
NATRON_MAJOR=`grep "define NATRON_VERSION_MAJOR" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_MINOR=`grep "define NATRON_VERSION_MINOR" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
NATRON_REVISION=`grep "define NATRON_VERSION_REVISION" $TMP/Natron/Global/Macros.h | awk '{print $3}'`
sed -i "" -e "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" "$CWD/commits-hash.sh" || exit 1

echo
echo "Building Natron $NATRON_REL_V from $NATRON_BRANCH on $OS using $MKJOBS threads."
echo
sleep 2

#Update GitVersion to have the correct hash
cp $CWD/GitVersion.h Global/GitVersion.h || exit 1
sed -i "" -e "s#__BRANCH__#${NATRON_BRANCH}#;s#__COMMIT__#${REL_GIT_VERSION}#"  Global/GitVersion.h || exit 1

#Generate config.pri
cat > config.pri <<EOF
boost {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib -lboost_serialization-mt
}
shiboken {
    PKGCONFIG -= shiboken
    INCLUDEPATH += /opt/local/include/shiboken-2.7
    LIBS += -L/opt/local/lib -lshiboken-python2.7.1.2
}
EOF

#Copy OpenColorIO-Config in Natron root as the .pro expects them there to copy it to the application bundle
cp -r $TMP/OpenColorIO-Configs . || exit 1

# Add CONFIG+=snapshot to indicate the build is a snapshot
if [ "$BRANCH" == "workshop" ]; then
    QMAKEEXTRAFLAGS=CONFIG+=snapshot
#else
#    QMAKEEXTRAFLAGS=CONFIG+=gbreakpad Enable when ready
fi


APP=Natron.app

if [ "$COMPILER" = "clang" ]; then
    SPEC=unsupported/macx-clang
else
    SPEC=macx-g++
fi
qmake -r -spec "$SPEC" QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" CONFIG+="$CONFIG" CONFIG+=`echo $BITS| awk '{print tolower($0)}'` CONFIG+=noassertions $QMAKEEXTRAFLAGS || exit 1
make -j${MKJOBS} || exit 1
macdeployqt App/${APP} || exit 1

#Copy and change exec_path of the whole Python framework with libraries
rm -rf App/${APP}/Contents/Frameworks/Python.framework
mkdir -p App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib
rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib
cp -pr /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7 App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib  || exit 1
rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/Resources
cp -pr /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Resources App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7  || exit 1
rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/Python
cp -pr /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Python App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7  || exit 1
chmod 755 App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/Python
ln -sf Versions/2.7/Python App/${APP}/Contents/Frameworks/Python.framework/Python  || exit 1

rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/*
#rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/__pycache__
#rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/*/__pycache__
#FILES=`ls -l opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib|awk '{print $9}'`
#for f in FILES; do
#    #FILE=echo "{$f}" | sed "s/cpython-34.//g"
#    cp -r $f App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/$FILE || exit 1
#done

bin=App/${APP}/Contents/MacOS/Natron
# macdeployqt doesn't deal correctly with libs in /opt/local/lib/libgcc : handle them manually
LIBGCC=0
if otool -L $bin |fgrep /opt/local/lib/libgcc; then
    LIBGCC=1
fi
if [ "$LIBGCC" = "1" ]; then
    for l in gcc_s.1 gomp.1 stdc++.6; do
        lib=lib${l}.dylib
	install_name_tool -change /opt/local/lib/libgcc/$lib @executable_path/../Frameworks/$lib $bin
	for deplib in App/${APP}/Contents/Frameworks/*.dylib; do
	    install_name_tool -change /opt/local/lib/libgcc/$lib @executable_path/../Frameworks/$lib $deplib
	done
        cp /opt/local/lib/libgcc/$lib App/${APP}/Contents/Frameworks/$lib
    done
    # use gcc's libraries everywhere
    for l in gcc_s.1 gomp.1 stdc++.6; do
	lib=lib${l}.dylib
	for deplib in App/${APP}/Contents/Frameworks/*.framework/Versions/*/* App/${APP}/Contents/Frameworks/lib*.dylib; do
            install_name_tool -change /usr/lib/$lib @executable_path/../Frameworks/$lib $deplib
	done
    done
fi
for f in Python; do
    install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/2.7/${f} @executable_path/../Frameworks/${f}.framework/Versions/2.7/${f} $bin
done

if otool -L App/${APP}/Contents/MacOS/Natron  |fgrep /opt/local; then
    echo "Error: MacPorts libraries remaining in $bin, please check"
    exit 1
fi

mv App/${APP}/Contents/PlugIns App/${APP}/Contents/Plugins || exit 1
rm App/${APP}/Contents/Resources/qt.conf || exit 1

#Make a qt.conf file in Contents/Resources/
cat > App/${APP}/Contents/Resources/qt.conf <<EOF
[Paths]
Plugins = Plugins
EOF

cp Gui/Resources/Stylesheets/mainstyle.qss App/${APP}/Contents/Resources/ || exit 1

cp Renderer/NatronRenderer App/${APP}/Contents/MacOS
bin=App/${APP}/Contents/MacOS/NatronRenderer

#Change @executable_path for NatronRenderer deps
for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2 pyside-python2.7.1.2 shiboken-python2.7.1.2 intl.8; do
    lib=lib${l}.dylib
    install_name_tool -change /opt/local/lib/$lib @executable_path/../Frameworks/$lib $bin
done
for f in QtNetwork QtCore; do
    install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
done
if [ "$LIBGCC" = "1" ]; then
    for l in gcc_s.1 stdc++.6; do
        lib=lib${l}.dylib
	install_name_tool -change /opt/local/lib/libgcc/$lib @executable_path/../Frameworks/$lib $bin
    done
fi

#Copy and change exec_path of the whole Python framework with libraries
for f in Python; do
    install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/2.7/${f} @executable_path/../Frameworks/${f}.framework/Versions/2.7/${f} $bin
done

if otool -L $bin |fgrep /opt/local; then
    echo "Error: MacPorts libraries remaining in $bin, please check"
    exit 1
fi



#Do the same for crash reporter
if [ -f CrashReporter/NatronCrashReporter ]; then
    bin=App/${APP}/Contents/MacOS/NatronCrashReporter
    cp CrashReporter/NatronCrashReporter "$bin"
    for f in QtGui QtNetwork QtCore; do
	install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
    done
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
            lib=lib${l}.dylib
	    install_name_tool -change /opt/local/lib/libgcc/$lib @executable_path/../Frameworks/$lib $bin
	done
    fi

    if otool -L $bin |fgrep /opt/local; then
	echo "Error: MacPorts libraries remaining in $bin, please check"
	exit 1
    fi
fi

if [ -f CrashReporterCLI/NatronRendererCrashReporter ]; then
    bin=App/${APP}/Contents/MacOS/NatronRendererCrashReporter
    cp CrashReporterCLI/NatronRendererCrashReporter "$bin"
    for f in QtNetwork QtCore; do
	install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
    done
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
            lib=lib${l}.dylib
	    install_name_tool -change /opt/local/lib/libgcc/$lib @executable_path/../Frameworks/$lib $bin
	done
    fi

    if otool -L $bin |fgrep /opt/local; then
	echo "Error: MacPorts libraries remaining in $bin, please check"
	exit 1
    fi
fi

#go back to build directory
cd ..

if [ ! -d $PLUGINDIR ]; then
    echo "Error: plugin directory '$PLUGINDIR' does not exist"
    exit 1
fi

#Copy Pyside in the plugin dir
mkdir -p  $PLUGINDIR/PySide

QT_LIBS="QtCore QtGui QtNetwork QtOpenGL QtDeclarative QtHelp QtMultimedia QtScript QtScriptTools QtSql QtSvg QtTest QtUiTools QtXml QtWebKit QtXmlPatterns"

for qtlib in $QT_LIBS ;do
    bin="$PLUGINDIR/PySide/${qtlib}.so"
    cp "/opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/${qtlib}.so" "$bin" || exit 1

    for f in QT_LIBS; do
        install_name_tool -change "/opt/local/Library/Frameworks/${f}.framework/Versions/4/${f}" "@executable_path/../Frameworks/${f}.framework/Versions/4/${f}" "$bin"
    done

    for l in  pyside-python2.7.1.2 shiboken-python2.7.1.2; do
        dylib="lib${l}.dylib"
        install_name_tool -change "/opt/local/lib/$dylib" "@executable_path/../Frameworks/$dylib" "$bin"
    done
    # use gcc's libraries everywhere
    if [ "$LIBGCC" = "1" ]; then
	for l in gcc_s.1 gomp.1 stdc++.6; do
	    lib="lib${l}.dylib"
	    install_name_tool -change "/usr/lib/$lib" "@executable_path/../Frameworks/$lib" "$bin"
	done
    fi
done
cp  /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/__init__.py $PLUGINDIR/PySide  || exit 1
cp  /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/_utils.py $PLUGINDIR/PySide  || exit 1

