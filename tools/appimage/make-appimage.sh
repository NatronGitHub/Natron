#!/bin/bash

set -e
set -o pipefail

show_usage_and_exit() {
    echo "Usage: $0 <dir>"
    echo "<dir> is the location of an extracted no-installer linux tarball of Natron"
    exit 2
}

if [[ "$1" == "" ]]; then
    show_usage_and_exit
elif [[ ! -e "$1" ]]; then
    echo "Error: $1 not found"
    show_usage_and_exit
elif [[ ! -d "$1" ]]; then
    echo "Error: $1 is not a directory"
    show_usage_and_exit
fi

# "convert" extracted dir to AppDir
# this script is idempotent; existing files will be overwritten with the latest versions, subsequent runs will not change the outcome
pushd "$1" &>/dev/null

mkdir -p usr/share/{applications,icons/hicolor/256x256/apps,mime/packages}

# copy icons for desktop entry and MIME package
cp Resources/pixmaps/* usr/share/icons/hicolor/256x256/apps

# create desktop entry
cat > usr/share/applications/natron.desktop <<\EOF
[Desktop Entry]
Name=Natron
Type=Application
Exec=Natron
Icon=natronIcon256_linux
Categories=Video;
EOF

# create MIME package which the AppImage runtime can deploy
# copied from bin/natron-mime.sh
cat > usr/share/mime/packages/x-natron.xml <<\EOF
<?xml version="1.0" encoding="UTF-8"?>
<mime-info xmlns="http://www.freedesktop.org/standards/shared-mime-info">
    <mime-type type="application/x-natron">
        <comment>Natron Project File</comment>
        <icon name="natronProjectIcon_linux"/>
        <glob-deleteall/>
        <glob pattern="*.ntp"/>
    </mime-type>
</mime-info>
EOF

# copy files to AppDir root (would be done by tools like linuxdeploy, which we don't need for Natron)
cp usr/share/applications/natron.desktop .
cp usr/share/icons/hicolor/256x256/apps/natronIcon256_linux.png .

# fetch and store version number for use in the AppImage name
VERSION="$(bin/Natron --version | head -n1 | rev | awk '{print $1}' | rev || true)"
export VERSION

# create AppRun
[[ ! -f AppRun ]] && ln -s Natron AppRun

popd &>/dev/null

# create AppImage

# storing appimagetool in temporary directory
tempdir="$(mktemp -d /tmp/natron-appimage-XXXXX)"
_cleanup() { rm -rf "$tempdir"; }
trap _cleanup EXIT

pushd "$tempdir" &>/dev/null
wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x appimagetool-x86_64.AppImage
popd &>/dev/null

"$tempdir"/appimagetool-x86_64.AppImage "$1"
