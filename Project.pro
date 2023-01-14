# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier
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

TEMPLATE = subdirs

CONFIG(enable-breakpad) {
    include(breakpadpro.pri)
}

SUBDIRS += \
    HostSupport \
    gflags \
    glog \
    ceres \
    libmv \
    openMVG \
    qhttpserver \
    hoedown \
    libtess \
    Engine \
    Renderer \
    Gui \
    Tests \
    PythonBin \
    App

# where to find the sub projects - give the folders
gflags.subdir      = libs/gflags
glog.subdir        = libs/glog
ceres.subdir       = libs/ceres
libmv.subdir       = libs/libmv
openMVG.subdir     = libs/openMVG
qhttpserver.subdir = libs/qhttpserver
hoedown.subdir     = libs/hoedown
libtess.subdir     = libs/libtess

# what subproject depends on others
glog.depends = gflags
ceres.depends = glog gflags
libmv.depends = gflags ceres
openMVG.depends = ceres
Engine.depends = libmv openMVG HostSupport libtess ceres
Renderer.depends = Engine
Gui.depends = Engine qhttpserver
Tests.depends = Gui Engine
App.depends = Gui Engine

OTHER_FILES += \
    Global/Enums.h \
    Global/GLIncludes.h \
    Global/GlobalDefines.h \
    Global/KeySymbols.h \
    Global/Macros.h \
    Global/ProcInfo.h \
    Global/QtCompat.h \
    global.pri \
    config.pri

include(global.pri)
include(libs.pri)

isEmpty(CONFIG_SET) {
    message("You did not select a config option for the build. Defaulting to Devel. You can choose among  (snapshot | alpha | beta | RC | stable | custombuild). For custombuild you need to define the environment variable BUILD_USER_NAME. Also you can give a revision number to the version of Natron with the environment variable BUILD_NUMBER (e.g: RC1, RC2 etc...)")
}

*-xcode {
  # Qt 4.8.5's XCode generator has a bug and places moc_*.cpp files next to the sources instead of inside the build dir
  # However, setting the MOC_DIR doesn't fix that (Xcode build fails)
  # Simple rtule: don't use Xcode
  #MOC_DIR = $$OUT_PWD
  warning("Xcode generator wrongly places the moc files in the source directory. You thus cannot compile with different Qt versions using Xcode.")
}

CONFIG(debug, debug|release){
    message("Compiling in DEBUG mode.")
} else {
    message("Compiling in RELEASE mode.")
}

addresssanitizer {
  *g++* | *clang* {
    message("Compiling with AddressSanitizer (for gcc >= 4.8 and clang). Set the ASAN_SYMBOLIZER_PATH environment variable to point to the llvm-symbolizer binary, or make sure llvm-symbolizer in in your PATH.")
    message("For example, with Qt4 on macOS:")
    message("- with MacPorts:")
    message("  sudo port install clang-5.0")
    message("  export ASAN_SYMBOLIZER_PATH=/opt/local/bin/llvm-symbolizer-mp-5.0")
    message("  qmake QMAKE_CC=clang-mp-5.0 QMAKE_CXX='clang++-mp-5.0 -stdlib=libc++' QMAKE_LINK='clang++-mp-5.0 -stdlib=libc++' QMAKE_OBJECTIVE_CC='clang-mp-5.0 -stdlib=libc++' QMAKE_OBJECTIVE_CXX='clang++-mp-5.0 -stdlib=libc++' CONFIG+=addresssanitizer ...")
    message("- with homebrew:")
    message("  brew install llvm")
    message("  export ASAN_SYMBOLIZER_PATH=/usr/local/opt/llvm@11/bin/llvm-symbolizer")
    message("  qmake QMAKE_CC=/usr/local/opt/llvm@11/bin/clang QMAKE_CXX='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++' QMAKE_LINK='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++' QMAKE_OBJECTIVE_CC='/usr/local/opt/llvm@11/bin/clang -stdlib=libc++' QMAKE_OBJECTIVE_CXX='/usr/local/opt/llvm@11/bin/clang++ -stdlib=libc++' CONFIG+=addresssanitizer ...")
    message("see http://clang.llvm.org/docs/AddressSanitizer.html")
  }
}
