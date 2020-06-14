# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2020 The Natron developers
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

# Use this for binaries in App.pro, Renderer.pro etc...this way
# CONFIG += static-gui static-engine static-host-support static-breakpadclient static-ceres static-libmv

libmv-flags {
CONFIG += ceres-flags
INCLUDEPATH += $$PWD/libs/libmv/third_party
INCLUDEPATH += $$PWD/libs/libmv
}

glad-flags {
    CONFIG(debug, debug|release):   INCLUDEPATH += $$PWD/Global/gladDeb/include
    CONFIG(release, debug|release): INCLUDEPATH += $$PWD/Global/gladRel/include
}

openmvg-flags {
CONFIG += ceres-flags boost
# Make openMVG use openmp
!macx:*g++* {
    CONFIG += openmp
}
openmp {
    DEFINES += OPENMVG_USE_OPENMP
}

DEFINES += OPENMVG_HAVE_BOOST

# Do not use any serialization in openmvg (cereal, ply, stlplus ...)
DEFINES += OPENMVG_NO_SERIALIZATION

# Use this to use OsiMskSolverInterface.cpp
#DEFINES += OPENMVG_HAVE_MOSEK

c++11 {
   DEFINES += OPENMVG_STD_UNORDERED_MAP
   DEFINES += OPENMVG_STD_SHARED_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += OPENMVG_BOOST_UNORDERED_MAP
   DEFINES += OPENMVG_BOOST_SHARED_PTR
   DEFINES += OPENMVG_NO_UNIQUE_PTR
}

INCLUDEPATH += $$PWD/libs/openMVG/openMVG
INCLUDEPATH += $$PWD/libs/openMVG
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Clp/src
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Clp/src/OsiClip
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/CoinUtils/src
INCLUDEPATH += $$PWD/libs/openMVG/dependencies/osi_clp/Osi/src/Osi
INCLUDEPATH += $$PWD/libs/flann/src/cpp
#INCLUDEPATH += $$PWD/libs/lemon

}

ceres-flags {
CONFIG += glog-flags
DEFINES += CERES_HAVE_PTHREAD CERES_NO_SUITESPARSE CERES_NO_CXSPARSE CERES_HAVE_RWLOCK
# Comment to make ceres use a lapack library
DEFINES += CERES_NO_LAPACK
!macx:*g++* {
    CONFIG += openmp
}
openmp {
    DEFINES += CERES_USE_OPENMP
}
#If undefined, make sure to add to sources all the files in ceres/internal/ceres/generated
DEFINES += CERES_RESTRICT_SCHUR_SPECIALIZATION
DEFINES += WITH_LIBMV_GUARDED_ALLOC GOOGLE_GLOG_DLL_DECL= LIBMV_NO_FAST_DETECTOR=
c++11 {
   DEFINES += CERES_STD_UNORDERED_MAP
   DEFINES += CERES_STD_SHARED_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += CERES_BOOST_SHARED_PTR
   DEFINES += CERES_BOOST_UNORDERED_MAP
}
INCLUDEPATH += $$PWD/libs/ceres/config
INCLUDEPATH += $$PWD/libs/ceres/include
INCLUDEPATH += $$PWD/libs/ceres/internal
INCLUDEPATH += $$PWD/libs/Eigen3
win32-msvc* {
    CONFIG(64bit) {
        QMAKE_LFLAGS += /MACHINE:X64
    } else {
        QMAKE_LFLAGS += /MACHINE:X86
    }
}
}

glog-flags {
CONFIG += gflags-flags
DEFINES += GOOGLE_GLOG_DLL_DECL=
INCLUDEPATH += $$PWD/libs/gflags
INCLUDEPATH += $$PWD/libs/gflags/src
INCLUDEPATH += $$PWD/libs/gflags/src/gflags
INCLUDEPATH += $$PWD/libs/glog/src
win32* {
     INCLUDEPATH += $$PWD/libs/glog/src/windows

    # wingdi.h defines ERROR to be 0. When we call LOG(ERROR), it gets
    # substituted with 0, and it expands to COMPACT_GOOGLE_LOG_0. To allow us
    # to keep using this syntax, we define this macro to do the same thing
    # as COMPACT_GOOGLE_LOG_ERROR.
     DEFINES += GLOG_NO_ABBREVIATED_SEVERITIES
}
!win32* {
    INCLUDEPATH += $$PWD/libs/glog/src
}
win32-msvc* {
    CONFIG(64bit) {
        QMAKE_LFLAGS += /MACHINE:X64
    } else {
        QMAKE_LFLAGS += /MACHINE:X86
    }
}
}

gflags-flags {
INCLUDEPATH += $$PWD/libs/gflags
INCLUDEPATH += $$PWD/libs/gflags/src
INCLUDEPATH += $$PWD/libs/gflags/src/gflags
}

libtess-flags {
INCLUDEPATH += $$PWD/libs/libtess
}

################
# Gui
static-gui {
CONFIG += static-engine static-qhttpserver static-hoedown static-libtess
win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/x64/release/ -lGui
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/x64/debug/ -lGui
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/win32/release/ -lGui
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/win32/debug/ -lGui
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/release/ -lGui
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/debug/ -lGui
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Gui/build/Release/ -lGui
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Gui/build/Debug/ -lGui
        else:unix {
            LIBS += -L$$OUT_PWD/../Gui/ -lGui
            ## GLU is required by ViewerGL, but some versions of glew don't link it (e.g. Ubuntu 12.04)
            #!macx: LIBS += -lGLU
        }
}
INCLUDEPATH += $$PWD/Gui
DEPENDPATH += $$OUT_PWD/../Gui

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/x64/release/libGui.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/x64/debug/libGui.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/win32/release/libGui.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/win32/debug/libGui.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/release/libGui.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/debug/libGui.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/release/Gui.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/debug/Gui.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/build/Release/libGui.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Gui/build/Debug/libGui.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Gui/libGui.a
}

}  #static-gui

################
# Engine

static-engine {
CONFIG += static-libmv static-openmvg static-hoedown static-libtess

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/release/ -lEngine -lpsapi
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/debug/ -lEngine -lpsapi
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/release/ -lEngine -lpsapi
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/debug/ -lEngine -lpsapi
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/release/ -lEngine -lpsapi
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/debug/ -lEngine -lpsapi
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Release/ -lEngine
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Debug/ -lEngine
        else:unix: LIBS += -L$$OUT_PWD/../Engine/ -lEngine
}

INCLUDEPATH += $$PWD/Engine
DEPENDPATH += $$OUT_PWD/../Engine
INCLUDEPATH += $$PWD/libs/SequenceParsing
INCLUDEPATH += $$PWD/Global

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/x64/release/libEngine.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/x64/debug/libEngine.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/win32/release/libEngine.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/win32/debug/libEngine.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/libEngine.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/libEngine.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/Engine.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/Engine.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Release/libEngine.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Debug/libEngine.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Engine/libEngine.a
}
} #static-engine

################
# HostSupport

static-host-support {

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/x64/release/ -lHostSupport
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/x64/debug/ -lHostSupport
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/win32/release/ -lHostSupport
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/win32/debug/ -lHostSupport
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/release/ -lHostSupport
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/debug/ -lHostSupport
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Release/ -lHostSupport
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Debug/ -lHostSupport
        else:unix: LIBS += -L$$OUT_PWD/../HostSupport/ -lHostSupport
}

INCLUDEPATH += $$PWD/HostSupport
DEPENDPATH += $$OUT_PWD/../HostSupport
#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/libs/OpenFX/include
INCLUDEPATH += $$PWD/libs/OpenFX_extensions
INCLUDEPATH += $$PWD/libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/x64/release/libHostSupport.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/x64/debug/libHostSupport.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/win32/release/libHostSupport.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/win32/debug/libHostSupport.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/libHostSupport.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/libHostSupport.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/HostSupport.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/HostSupport.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Release/libHostSupport.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Debug/libHostSupport.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/libHostSupport.a
}
} #static-host-support

################
# LibMV
static-libmv {
CONFIG += static-ceres
INCLUDEPATH += $$PWD/libs/libmv
DEPENDPATH += $$OUT_PWD/../libs/libmv

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/x64/release/ -lLibMV
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/x64/debug/ -lLibMV
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/win32/release/ -lLibMV
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/win32/debug/ -lLibMV
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/release/ -lLibMV
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/debug/ -lLibMV
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/build/Release/ -lLibMV
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libmv/build/Debug/ -lLibMV
        else:unix: LIBS += -L$$OUT_PWD/../libs/libmv/ -lLibMV
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/x64/release/libLibMV.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/x64/debug/libLibMV.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/win32/release/libLibMV.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/win32/debug/libLibMV.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/release/libLibMV.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/debug/libLibMV.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/release/libLibMV.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/debug/libLibMV.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/build/Release/libLibMV.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/build/Debug/libLibMV.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/libmv/libLibMV.a
}

} # static-libmv


################
# openMVG
static-openmvg {
CONFIG += static-ceres
INCLUDEPATH += $$PWD/libs/openMVG
DEPENDPATH += $$OUT_PWD/../libs/openMVG

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/x64/release/ -lopenMVG
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/x64/debug/ -lopenMVG
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/win32/release/ -lopenMVG
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/win32/debug/ -lopenMVG
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/release/ -lopenMVG
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/debug/ -lopenMVG
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/build/Release/ -lopenMVG
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/openMVG/build/Debug/ -lopenMVG
        else:unix: LIBS += -L$$OUT_PWD/../libs/openMVG/ -lopenMVG
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/x64/release/libopenMVG.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/x64/debug/libopenMVG.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/win32/release/libopenMVG.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/win32/debug/libopenMVG.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/release/libopenMVG.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/debug/libopenMVG.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/release/libopenMVG.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/debug/libopenMVG.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/build/Release/libopenMVG.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/build/Debug/libopenMVG.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/openMVG/libopenMVG.a
}

} # static-openmvg

################
# ceres
static-ceres {
CONFIG += static-glog
!macx:*g++* {
    CONFIG += openmp
}
win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/x64/release/ -lceres
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/x64/debug/ -lceres
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/win32/release/ -lceres
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/win32/debug/ -lceres
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/release/ -lceres
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/debug/ -lceres
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/build/Release/ -lceres
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/ceres/build/Debug/ -lceres
        else:unix: LIBS += -L$$OUT_PWD/../libs/ceres/ -lceres
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/x64/release/libceres.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/x64/debug/libceres.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/win32/release/libceres.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/win32/debug/libceres.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/release/libceres.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/debug/libceres.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/release/libceres.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/debug/libceres.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/build/Release/libceres.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/build/Debug/libceres.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/ceres/libceres.a
}
} # static-ceres {

################
# glog
static-glog {
CONFIG += static-gflags
win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/x64/release/ -lglog
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/x64/debug/ -lglog
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/win32/release/ -lglog
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/win32/debug/ -lglog
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/release/ -lglog
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/debug/ -lglog
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/build/Release/ -lglog
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/glog/build/Debug/ -lglog
        else:unix: LIBS += -L$$OUT_PWD/../libs/glog/ -lglog
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/x64/release/libglog.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/x64/debug/libglog.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/win32/release/libglog.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/win32/debug/libglog.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/release/libglog.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/debug/libglog.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/release/libglog.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/debug/libglog.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/build/Release/libglog.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/build/Debug/libglog.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/glog/libglog.a
}
} # static-glog {

################
# gflags
static-gflags {
win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/x64/release/ -lgflags
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/x64/debug/ -lgflags
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/win32/release/ -lgflags
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/win32/debug/ -lgflags
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/release/ -lgflags
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/debug/ -lgflags
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/build/Release/ -lgflags
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/gflags/build/Debug/ -lgflags
        else:unix: LIBS += -L$$OUT_PWD/../libs/gflags/ -lgflags
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/x64/release/libgflags.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/x64/debug/libgflags.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/win32/release/libgflags.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/win32/debug/libgflags.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/release/libgflags.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/debug/libgflags.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/release/libgflags.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/debug/libgflags.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/build/Release/libgflags.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/build/Debug/libgflags.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/gflags/libgflags.a
}
} # static-gflags {

################
# qhttpserver

static-qhttpserver {

DEFINES += QHTTP_SERVER_STATIC
INCLUDEPATH += $$PWD/libs/qhttpserver/src
DEPENDPATH += $$OUT_PWD/../libs/qhttpserver/src

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/x64/release/ -lqhttpserver
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/x64/debug/ -lqhttpserver
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/win32/release/ -lqhttpserver
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/win32/debug/ -lqhttpserver
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/ -lqhttpserver
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/ -lqhttpserver
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/src/build/Release/ -lqhttpserver
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/qhttpserver/src/build/Debug/ -lqhttpserver
        else:unix: LIBS += -L$$OUT_PWD/../libs/qhttpserver/build/ -lqhttpserver
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/x64/release/libqhttpserver.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/x64/debug/libqhttpserver.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/win32/release/libqhttpserver.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/win32/debug/libqhttpserver.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/libqhttpserver.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/libqhttpserver.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/release/libqhttpserver.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/debug/libqhttpserver.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/src/build/Release/libqhttpserver.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/src/build/Debug/libqhttpserver.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/qhttpserver/build/libqhttpserver.a
}
} # static-qhttpserver

################
# hoedown

static-hoedown {

INCLUDEPATH += $$PWD/libs/hoedown/src
DEPENDPATH += $$OUT_PWD/../libs/hoedown

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/x64/release/ -lhoedown
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/x64/debug/ -lhoedown
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/win32/release/ -lhoedown
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/win32/debug/ -lhoedown
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/ -lhoedown
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/ -lhoedown
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/Release/ -lhoedown
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/hoedown/build/Debug/ -lhoedown
        else:unix: LIBS += -L$$OUT_PWD/../libs/hoedown/build/ -lhoedown
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/x64/release/hoedown.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/x64/debug/hoedown.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/win32/release/hoedown.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/win32/debug/hoedown.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/libhoedown.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/libhoedown.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/release/libhoedown.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/debug/libhowdown.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/Release/libhoedown.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/Debug/libhoedown.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/hoedown/build/libhoedown.a
}
} # static-hoedown


################
# libtess

static-libtess {

INCLUDEPATH += $$PWD/libs/libtess
DEPENDPATH += $$OUT_PWD/../libs/libtess

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/x64/release/ -ltess
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/x64/debug/ -ltess
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/win32/release/ -ltess
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/win32/debug/ -ltess
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/release/ -ltess
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/debug/ -ltess
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/build/Release/ -ltess
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libs/libtess/build/Debug/ -ltess
        else:unix: LIBS += -L$$OUT_PWD/../libs/libtess/ -ltess
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/x64/release/tess.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/x64/debug/tess.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/win32/release/tess.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/win32/debug/tess.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/release/libtess.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/debug/libtess.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/release/libtess.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/debug/libtess.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/build/Release/libtess.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/build/Debug/libtess.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libs/libtess/libtess.a
}
} # static-libtess
