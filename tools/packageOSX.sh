#!/bin/bash
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

VOL=Natron
APP=Natron.app
DMG_FINAL=${VOL}.dmg
DMG_BACK=Natron/Gui/Resources/Images/splashscreen.png

if [ $# != 0 ]; then
  echo "Usage: $0"
  echo "Compiles Natron, including openfx-io and openfx-misc, and package it into $DMG_FINAL in the current directory"
  exit 1
fi


## check out a specific tag
#CONFIG=release
#NATRONBRANCH=tags/v1.0.0-rc1
#OFXBRANCH=tags/Natron-1.0.0-RC1

## check out the workshop branch
CONFIG=release
NATRONBRANCH=workshop
OFXBRANCH=master

case $NATRONBRANCH in
    tags/*)
	QMAKEEXTRAFLAGS=
	;;
    *)
	QMAKEEXTRAFLAGS=CONFIG+=snapshot
	;;
esac	     
	     
# required macports ports (first ones are for Natron, then for OFX plugins and TuttleOFX)
PORTS="boost qt4-mac boost glew cairo expat jpeg openexr ffmpeg openjpeg15 libcaca freetype lcms swig ImageMagick lcms2 libraw nasm opencolorio openimageio swig-python py27-numpy flex bison openexr opencv seexpr ctl fontconfig py34-shiboken py34-pyside imagemagick"

PORTSOK=yes
for p in $PORTS; do
 if port installed $p | fgrep "  $p @" > /dev/null; then
   #echo "port $p OK"
   true
 else
   echo "Error: port $p is missing"
   PORTSOK=no
 fi
done

if [ "$PORTSOK" = "no" ]; then
  echo "At least one port from macports is missing. Please install them."
  exit 1
fi

if port installed ffmpeg |fgrep '(active)' |fgrep '+gpl' > /dev/null; then
  echo "ffmpeg port should not be installed with the +gpl2 or +gpl3 variants, please reinstall it using 'sudo port install ffmpeg -gpl2 -gpl3'"
  exit 1
fi

if pkg-config --cflags glew; then
  true
else
  echo "Error: missing pkg-config file for package glew"
  cat <<EOFF
To fix this, execute the following:
sudo cat > /opt/local/lib/pkgconfig/glew.pc << EOF
prefix=/opt/local
exec_prefix=/opt/local/bin
libdir=/opt/local/lib
includedir=/opt/local/include/GL

Name: glew
Description: The OpenGL Extension Wrangler library
Version: 1.10.0
Cflags: -I\${includedir}
Libs: -L\${libdir} -lGLEW -framework OpenGL
Requires:
EOF
EOFF
  exit 1
fi

echo "* Compiling Natron version $NATRONBRANCH with OpenFX plugins version $OFXBRANCH"
echo
if [ -z "$MAKEJFLAGS" ]; then
	MAKEJFLAGS="-j4"
fi

OS=`uname -s`
BITS=64
if [ "$OS" = "Darwin" ]; then
  macosx=`uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|'`
  case "$macosx" in
  [1-8])
    BITS=32
    ;;
  9|10)
    BITS=Universal
    ;;
   esac;
  echo "Building for architecture $BITS"
fi

#if [ -e build ]; then
#  echo "Directory 'build' already exists, please remove it"
#  exit 1
#fi

mkdir build
cd build

git clone https://github.com/MrKepzie/openfx-io.git
(cd openfx-io; git checkout "$OFXBRANCH")
git clone https://github.com/devernay/openfx-misc.git
(cd openfx-misc; git checkout "$OFXBRANCH")
git clone https://github.com/olear/openfx-arena.git
(cd openfx-arena; git checkout "$OFXBRANCH")
git clone https://github.com/MrKepzie/Natron.git
(cd Natron; git checkout "$NATRONBRANCH")
for i in openfx-io openfx-io/IOSupport/SequenceParsing openfx-misc openfx-arena Natron Natron/libs/SequenceParsing; do (echo $i;cd $i; git submodule update -i --recursive); done

#compile Natron
cd Natron
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
qmake -r -spec unsupported/macx-clang CONFIG+=c++11 CONFIG+="$CONFIG" CONFIG+=`echo $BITS| awk '{print tolower($0)}'` CONFIG+=noassertions $QMAKEEXTRAFLAGS
make $MAKEJFLAGS || exit


macdeployqt App/${APP} || exit
mv App/${APP}/Contents/PlugIns App/${APP}/Contents/Plugins || exit 1

rm App/${APP}/Contents/Resources/qt.conf || exit 1
cat > App/${APP}/Contents/Resources/qt.conf <<EOF
[Paths]
Plugins = Plugins
EOF

cp Renderer/NatronRenderer App/${APP}/Contents/MacOS
bin=App/${APP}/Contents/MacOS/NatronRenderer
for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2 pyside-python2.7.1.2 shiboken-python2.7.1.2 intl.8; do
  lib=lib${l}.dylib
  install_name_tool -change /opt/local/lib/$lib @executable_path/../Frameworks/$lib $bin
done
for f in QtNetwork QtCore; do
  install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
done

#Copy the whole Python framework with libraries 

#mkdir -p App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7
for f in Python; do
  install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/2.7/${f} @executable_path/../Frameworks/${f}.framework/Versions/2.7/${f} $bin
done
if otool -L App/${APP}/Contents/MacOS/NatronRenderer  |fgrep /opt/local; then
  echo "Error: MacPorts libraries remaining in $bin, please check"
  exit 1
fi

mkdir -p App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib
cp -r /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7 App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib
cp -r /opt/local/Library/Frameworks/Python.framework/Versions/2.7/Resources App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7
ln -s App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/Python App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib
rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/*
#rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/__pycache__
#rm -rf App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/*/__pycache__

#FILES=$(ls -l opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib|awk '{print $9}')
#for f in FILES; do
#    #FILE=echo "{$f}" | sed "s/cpython-34.//g"
#    cp -r $f App/${APP}/Contents/Frameworks/Python.framework/Versions/2.7/lib/$FILE || exit 1
#done



#Do the same for crash reporter
cp CrashReporter/NatronCrashReporter App/${APP}/Contents/MacOS
bin=App/${APP}/Contents/MacOS/NatronCrashReporter
for f in QtGui QtNetwork QtCore; do
  install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
done

if otool -L App/${APP}/Contents/MacOS/NatronCrashReporter  |fgrep /opt/local; then
  echo "Error: MacPorts libraries remaining in $bin, please check"
  exit 1
fi
cd ..


PLUGINDIR=Natron/App/${APP}/Contents/Plugins
if [ ! -d $PLUGINDIR ]; then
  echo "Error: plugin directory '$PLUGINDIR' does not exist"
  exit 1
fi

#Copy Pyside in the plugin dir
mkdir -p  $PLUGINDIR/PySide

for lib in QtCore QtGui QtNetwork QtOpenGL;do
cp /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/${lib}.so $PLUGINDIR/PySide/${lib}.so
bin=$PLUGINDIR/PySide/${lib}.so
for f in QtCore QtGui QtNetwork QtOpenGL; do
  install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
done
for l in  pyside-python2.7.1.2 shiboken-python2.7.1.2; do
  dylib=lib${l}.dylib
  install_name_tool -change /opt/local/lib/$dylib @executable_path/../Frameworks/$dylib $bin
done
done
cp  /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/__init__.py $PLUGINDIR/PySide
cp  /opt/local/Library/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages/PySide/_utils.py $PLUGINDIR/PySide

#compile openfx-io
cd openfx-io
make CXX=clang++ BITS=$BITS CONFIG=$CONFIG OCIO_HOME=/opt/local OIIO_HOME=/opt/local SEEXPR_HOME=/opt/local $MAKEJFLAGS || exit
mv IO/$OS-$BITS-$CONFIG/IO.ofx.bundle "../$PLUGINDIR" || exit
cd ..

#compile openfx-misc
cd openfx-misc
make CXX=clang++ BITS=$BITS HAVE_CIMG=1 CONFIG=$CONFIG $MAKEJFLAGS || exit
mv Misc/$OS-$BITS-$CONFIG/Misc.ofx.bundle "../$PLUGINDIR" || exit
mv CImg/$OS-$BITS-$CONFIG/CImg.ofx.bundle "../$PLUGINDIR" || exit
cd ..

#compile openfx-arena
cd openfx-arena
make CXX=clang++ USE_PANGO=1 USE_SVG=1 BITS=$BITS CONFIG=$CONFIG $MAKEJFLAGS || exit
mv Bundle/$OS-$BITS-$CONFIG/Arena.ofx.bundle "../$PLUGINDIR" || exit
cd ..

#make the dmg
DMG_TMP=tmp${DMG_FINAL}
test -e /Volumes/"$VOL" && umount -f /Volumes/"$VOL"
test -e "$DMG_TMP" && rm -f "$DMG_TMP"
test -e "$DMG_FINAL" && rm -f "$DMG_FINAL"
hdiutil create -srcfolder Natron/App/"$APP" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -volname "$VOL" "$DMG_TMP"
DIRNAME=`hdiutil attach "$DMG_TMP" |fgrep Apple_HFS|awk '{ print $3, $4 }'`
if [ -z "$DIRNAME" ]; then
  echo "Error: cannot find disk image mount point"
  exit
fi
DISK=`echo $DIRNAME | sed -e s@/Volumes/@@`

# copy the background image
#mkdir "/Volumes/${DISK}/.background"
#DMG_BACK_NAME=`basename ${DMG_BACK}`
#cp ${DMG_BACK} "/Volumes/${DISK}/.background/${DMG_BACK_NAME}"

# already copied the application during "hdiutil create"
#(cd Natron/App; tar cf - ${APP}) | (cd "$DIRNAME"; tar xvf -)

# create an alias to Applications
ln -sf /Applications /Volumes/${DISK}/Applications

# dmg window dimensions
dmg_width=768
dmg_height=432
dmg_topleft_x=200
dmg_topleft_y=200
dmg_bottomright_x=`expr $dmg_topleft_x + $dmg_width`
dmg_bottomright_y=`expr $dmg_topleft_y + $dmg_height`

#create the view of the dmg with the alias to Applications
osascript <<EOT
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
    -- set background picture of theViewOptions to file ".background:${DMG_BACK_NAME}"
    set position of item "$APP" of container window to {120, 180}
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

# convert to compressed image, delete temp image
hdiutil convert "${DMG_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL}" || exit

mv "$DMG_FINAL" ..
cd ..
rm -rf build

echo "$DMG_FINAL" is available
