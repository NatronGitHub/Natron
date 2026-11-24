# Source this file from a shell to setup the environment for Natron
# compilation and tests where the Natron source tree is:
# . path_to_Natron_sources/tools/utils/natron-sdk-setup-linux.sh
SCRIPTPATH="$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )"
NATRON="$( dirname "$( dirname "$SCRIPTPATH" )" )"
TOP_SOURCES="$( dirname "${NATRON}" )"
# Where the Natron SDK is (the Rocky Linux 9 SDK, see INSTALL_LINUX.md).
# Its Python is 3.11, and PySide2/Shiboken2 are in its site-packages.
SDK=/opt/Natron-sdk
QTDIR="$SDK/qt5"
FFMPEG="$SDK/ffmpeg-gpl2"
LIBRAW="$SDK/libraw-gpl2"
OSMESA="$SDK/osmesa"
# The SDK is built with gcc-toolset-14 (GCC_TOOLSET in tools/jenkins/include/scripts/build-Linux-sdk.sh)
GCC_TOOLSET="${GCC_TOOLSET:-14}"
if [ -f "/opt/rh/gcc-toolset-${GCC_TOOLSET}/enable" ]; then
    # shellcheck source=/dev/null
    . "/opt/rh/gcc-toolset-${GCC_TOOLSET}/enable"
fi
PATH="$SDK/bin:$QTDIR/bin:$FFMPEG/bin:$LIBRAW/bin:$PATH"
LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$FFMPEG/lib:$LIBRAW/lib:$OSMESA/lib"
LD_LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$FFMPEG/lib:$LIBRAW/lib"
LD_RUN_PATH="$SDK/lib:$QTDIR/lib:$FFMPEG/lib:$LIBRAW/lib"
CPATH="$SDK/include:$QTDIR/include:$FFMPEG/include:$LIBRAW/include:$OSMESA/include"
PKG_CONFIG_PATH="$SDK/lib/pkgconfig:$OSMESA/lib/pkgconfig:$QTDIR/lib/pkgconfig:$FFMPEG/lib/pkgconfig:$LIBRAW/lib/pkgconfig"
export PATH QTDIR LIBRARY_PATH LD_LIBRARY_PATH LD_RUN_PATH CPATH PKG_CONFIG_PATH
NATRON_PLUGIN_PATH="$NATRON/Gui/Resources/PyPlugs"
OFX_PLUGIN_PATH="$HOME/Development/OFX/Plugins"
OCIO="$HOME/Development/Natron-gforge/OpenColorIO-Configs/blender/config.ocio"
export NATRON_PLUGIN_PATH OFX_PLUGIN_PATH OCIO
export LANG="C"
echo "###################################################################"
echo "# Shell is now setup to compile and run Natron and OpenFX plugins #"
echo "###################################################################"
echo "# Compile OFX plugins with:"
echo "cd \"$TOP_SOURCES\""
echo 'for d in openfx-io openfx-misc openfx-arena openfx-gmic; do'
echo ' (cd $d && make CONFIG=release)'
echo 'done'
echo "# Create links to plugins in $HOME/OFX/Plugins:"
echo "mkdir -p \"$HOME/OFX/Plugins\""
echo "cd \"$TOP_SOURCES\""
echo 'ln -s $PWD/openfx-*/*-release/*.ofx.bundle "$HOME/OFX/Plugins"'
echo "# Compile Natron with:"
echo "cd \"$NATRON\""
echo 'cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$SDK/qt5;$SDK" -DPython3_EXECUTABLE="$SDK/bin/python3" && cmake --build build -j4'
echo "# Run Natron with:"
echo "cd \"$NATRON\""
echo 'env FONTCONFIG_FILE="$NATRON/Gui/Resources/etc/fonts/fonts.conf" \'
echo ' OCIO="$NATRON/OpenColorIO-Configs/blender/config.ocio" \'
echo ' OFX_PLUGIN_PATH="$HOME/OFX/Plugins" \'
echo ' NATRON_PLUGIN_PATH="$NATRON/Gui/Resources/PyPlugs" \'
echo ' build/App/Natron'
echo "# Run unit tests from https://github.com/NatronGitHub/Natron-Tests with:"
echo "cd \"$TOP_SOURCES/Natron-Tests\""
echo 'env FONTCONFIG_FILE="$NATRON/Gui/Resources/etc/fonts/fonts.conf" \'
echo ' OCIO="$NATRON/OpenColorIO-Configs/blender/config.ocio" \'
echo ' OFX_PLUGIN_PATH="$HOME/OFX/Plugins" \'
echo ' NATRON_PLUGIN_PATH="$NATRON/Gui/Resources/PyPlugs" \'
echo ' FFMPEG="$SDK/ffmpeg" \'
echo ' ./runTestsOne.sh "$NATRON/build/Renderer/NatronRenderer"'
