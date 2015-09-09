#!/bin/bash

#Usage  ./build-installer.sh

source $(pwd)/common.sh || exit 1

cd $CWD/build || exit 1

#Make the dmg
APP_NAME=Natron
APP=${APP_NAME}.app
DMG_FINAL=${APP_NAME}.dmg
DMG_TMP=tmp${DMG_FINAL}

DMG_BACK=$CWD/build/Natron/Gui/Resources/Images/splashscreen.png


test -e /Volumes/"$APP_NAME" && umount -f /Volumes/"$APP_NAME"
test -e "$DMG_TMP" && rm -f "$DMG_TMP"
test -e "$DMG_FINAL" && rm -f "$DMG_FINAL"
hdiutil create -srcfolder Natron/App/"$APP" -fs HFS+ -fsargs "-c c=64,a=16,e=16" -format UDRW -volname "$APP_NAME" "$DMG_TMP"
DIRNAME=`hdiutil attach "$DMG_TMP" |fgrep Apple_HFS|awk '{ print $3, $4 }'`
if [ -z "$DIRNAME" ]; then
    echo "Error: cannot find disk image mount point"
    exit || 1
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
hdiutil convert "${DMG_TMP}" -format UDZO -imagekey zlib-level=9 -o "${DMG_FINAL}" || exit 1