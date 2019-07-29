# Source this file from a shell to setup the environment for Natron
# compilation and tests where the Natron source tree is:
# . path_to_Natron_sources/tools/utils/natron-sdk-setup-linux.sh
SCRIPTPATH="$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )"
NATRON="$( dirname "$( dirname "$SCRIPTPATH" )" )"
TOP_SOURCES="$( dirname "${NATRON}" )"
# Where the Natron SDK is. It should 
SDK=/opt/Natron-sdk
GCC="$SDK/gcc"
QTDIR="$SDK/qt4"
FFMPEG="$SDK/ffmpeg-gpl2"
LIBRAW="$SDK/libraw-gpl2"
OSMESA="$SDK/osmesa"
PATH="$SDK/bin:$QTDIR/bin:$GCC/bin:$FFMPEG/bin:$LIBRAW_PATH:$PATH"
LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib64:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib:$OSMESA/lib"
LD_LIBRARY_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib64:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib"
LD_RUN_PATH="$SDK/lib:$QTDIR/lib:$GCC/lib:$FFMPEG/lib:$LIBRAW/lib"
CPATH="$SDK/include:$QTDIR/include:$GCC/include:$FFMPEG/include:$LIBRAW/include:$OSMESA/include"
PKG_CONFIG_PATH="$SDK/lib/pkgconfig:$OSMESA/lib/pkgconfig:$QTDIR/lib/pkgconfig:$GCC/lib/pkgconfig:$FFMPEG/lib/pkgconfig:$LIBRAW/lib/pkgconfig"
PYTHONPATH="$QTDIR/lib/python2.7/site-packages/"
export QTDIR LD_LIBRARY_PATH LD_RUN_PATH CPATH PKG_CONFIG_PATH PYTHONPATH
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
echo 'qmake CONFIG+=debug CONFIG+=enable-cairo CONFIG+=enable-osmesa CONFIG+=openmp LLVM_PATH=$SDK/llvm OSMESA_PATH=$SDK/osmesa -r && make'
echo "# Run Natron with:"
echo "cd \"$NATRON\""
echo 'env FONTCONFIG_FILE="$NATRON/Gui/Resources/etc/fonts/fonts.conf" \'
echo ' OCIO="$NATRON/OpenColorIO-Configs/blender/config.ocio" \'
echo ' OFX_PLUGIN_PATH="$HOME/OFX/Plugins" \'
echo ' NATRON_PLUGIN_PATH="$NATRON/Gui/Resources/PyPlugs" \'
echo ' App/Natron'
echo "# Run unit tests from https://github.com/NatronGitHub/Natron-Tests with:"
echo "cd \"$TOP_SOURCES/Natron-Tests\""
echo 'env FONTCONFIG_FILE="$NATRON/Gui/Resources/etc/fonts/fonts.conf" \'
echo ' OCIO="$NATRON/OpenColorIO-Configs/blender/config.ocio" \'
echo ' OFX_PLUGIN_PATH="$HOME/OFX/Plugins" \'
echo ' NATRON_PLUGIN_PATH="$NATRON/Gui/Resources/PyPlugs" \'
echo ' FFMPEG="$SDK/ffmpeg" \'
echo ' ./runTestsOne.sh "$NATRON/Renderer/NatronRenderer"'
