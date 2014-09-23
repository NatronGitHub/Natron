#!/bin/bash -x

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
CONFIG=release
NATRONBRANCH=tags/v1.0.0-rc1
OFXBRANCH=tags/Natron-1.0.0-RC1

## check out the workshop branch
#CONFIG=debug
#NATRONBRANCH=workshop
#OFXBRANCH=master

# required macports ports (some may be for TuttleOFX)
PORTS="scons boost jpeg openexr ffmpeg openjpeg15 libcaca freetype glew lcms swig ImageMagick lcms2 libraw nasm opencolorio openimageio swig-python py27-numpy flex bison openexr opencv qt4-mac"

PORTSOK=yes
for p in $PORTS; do
 if port installed $p | fgrep $p > /dev/null; then
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
MAKEJFLAGS="-j4"
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

if [ -e build ]; then
  echo "Directory 'build' already exists, please remove it"
  exit 1
fi

mkdir build
cd build

git clone https://github.com/MrKepzie/openfx-io.git
(cd openfx-io; git checkout "$OFXBRANCH")
git clone https://github.com/devernay/openfx-misc.git
(cd openfx-misc; git checkout "$OFXBRANCH")
git clone https://github.com/MrKepzie/Natron.git
(cd Natron; git checkout "$NATRONBRANCH")
for i in openfx-io openfx-io/IOSupport/SequenceParsing openfx-misc Natron Natron/libs/SequenceParsing; do (echo $i;cd $i; git submodule update -i -r); done

#compile Natron
cd Natron
cat > config.pri <<EOF
boost {
    INCLUDEPATH += /opt/local/include
    LIBS += -L/opt/local/lib -lboost_serialization-mt -lboost_thread-mt -lboost_system-mt
}
EOF
qmake -r CONFIG+="$CONFIG" CONFIG+=`echo $BITS| awk '{print tolower($0)}'` CONFIG+=noassertions
make $MAKEJFLAGS || exit
macdeployqt App/${APP} || exit
cp Renderer/NatronRenderer App/${APP}/Contents/MacOS
bin=App/${APP}/Contents/MacOS/NatronRenderer
for l in boost_serialization-mt boost_thread-mt boost_system-mt expat.1 cairo.2; do
  lib=lib${l}.dylib
  install_name_tool -change /opt/local/lib/$lib @executable_path/../Frameworks/$lib $bin
done
for f in QtNetwork QtCore; do
  install_name_tool -change /opt/local/Library/Frameworks/${f}.framework/Versions/4/${f} @executable_path/../Frameworks/${f}.framework/Versions/4/${f} $bin
done
if otool -L App/${APP}/Contents/MacOS/NatronRenderer  |fgrep /opt/local; then
  echo "Error: MacPorts libraries remaining in $bin, please check"
  exit 1
fi
cd ..

PLUGINDIR=Natron/App/${APP}/Contents/Plugins
if [ ! -d $PLUGINDIR ]; then
  echo "Error: plugin directory '$PLUGINDIR' does not exist"
  exit 1
fi

#compile openfx-io
cd openfx-io
make BITS=$BITS CONFIG=$CONFIG OCIO_HOME=/opt/local OIIO_HOME=/opt/local $MAKEJFLAGS || exit
mv IO/$OS-$BITS-$CONFIG/IO.ofx.bundle "../$PLUGINDIR" || exit
cd ..

#compile openfx-misc
cd openfx-misc
make BITS=$BITS CONFIG=$CONFIG $MAKEJFLAGS || exit
mv Misc/$OS-$BITS-$CONFIG/Misc.ofx.bundle "../$PLUGINDIR" || exit
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
