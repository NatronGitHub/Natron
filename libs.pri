# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
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

openmvg-flags {
CONFIG += ceres-flags
# Make openMVG use openmp
#DEFINES += OPENMVG_USE_OPENMP

# Do not use any serialization in openmvg (cereal, ply, stlplus ...)
DEFINES += OPENMVG_NO_SERIALIZATION

# Use this to use OsiMskSolverInterface.cpp
#DEFINES += OPENMVG_HAVE_MOSEK

c++11 {
   DEFINES += OPENMVG_STD_UNORDERED_MAP
   DEFINES += OPENMVG_STD_SHARED_PTR
   DEFINES += OPENMVG_STD_UNIQUE_PTR
} else {
   # Use boost::shared_ptr and boost::unordered_map
   CONFIG += boost
   DEFINES += OPENMVG_BOOST_UNORDERED_MAP
   DEFINES += OPENMVG_BOOST_SHARED_PTR
   DEFINES += OPENMVG_BOOST_UNIQUE_PTR
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
DEFINES += CERES_HAVE_PTHREAD CERES_NO_SUITESPARSE CERES_NO_CXSPARSE CERES_HAVE_RWLOCK
# Comment to make ceres use a lapack library
DEFINES += CERES_NO_LAPACK
# Uncomment to make ceres use openmp
#DEFINES += CERES_USE_OPENMP
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
INCLUDEPATH += $$PWD/libs/gflags
INCLUDEPATH += $$PWD/libs/gflags/src
INCLUDEPATH += $$PWD/libs/gflags/src/gflags
INCLUDEPATH += $$PWD/libs/glog/src
win32* {
     INCLUDEPATH += $$PWD/libs/glog/src/windows
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

################
# Gui
static-gui {
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
            # GLU is required by ViewerGL, but some versions of glew don't link it (e.g. Ubuntu 12.04)
            !macx: LIBS += -lGLU
        }
}
INCLUDEPATH += $$OUT_PWD/../Gui
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


win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/release/ -lEngine
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/x64/debug/ -lEngine
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/release/ -lEngine
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/win32/debug/ -lEngine
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/release/ -lEngine
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/debug/ -lEngine
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Release/ -lEngine
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Debug/ -lEngine
        else:unix: LIBS += -L$$OUT_PWD/../Engine/ -lEngine
}

INCLUDEPATH += $$OUT_PWD/../Engine
DEPENDPATH += $$OUT_PWD/../Engine
INCLUDEPATH += $$OUT_PWD/../libs/SequenceParsing
INCLUDEPATH += $$OUT_PWD/../Global

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

INCLUDEPATH += $$OUT_PWD/../HostSupport
DEPENDPATH += $$OUT_PWD/../HostSupport
#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$OUT_PWD/../libs/OpenFX/include
INCLUDEPATH += $$OUT_PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$OUT_PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$OUT_PWD/..

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
# ceres
static-ceres {
win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ceres/x64/release/ -lceres
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ceres/x64/debug/ -lceres
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ceres/win32/release/ -lceres
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ceres/win32/debug/ -lceres
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ceres/release/ -lceres
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ceres/debug/ -lceres
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../ceres/build/Release/ -lceres
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../ceres/build/Debug/ -lceres
        else:unix: LIBS += -L$$OUT_PWD/../ceres/ -lceres
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/x64/release/libceres.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/x64/debug/libceres.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/win32/release/libceres.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/win32/debug/libceres.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/release/libceres.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/debug/libceres.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/release/libceres.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/debug/libceres.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/build/Release/libceres.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../ceres/build/Debug/libceres.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../ceres/libceres.a
}
} # static-ceres {
################
# LibMV
static-libmv {
INCLUDEPATH += $$OUT_PWD/../libs/libmv
DEPENDPATH += $$OUT_PWD/../libs/libmv

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libmv/x64/release/ -lLibMV
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libmv/x64/debug/ -lLibMV
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libmv/win32/release/ -lLibMV
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libmv/win32/debug/ -lLibMV
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libmv/release/ -lLibMV
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libmv/debug/ -lLibMV
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../libmv/build/Release/ -lLibMV
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../libmv/build/Debug/ -lLibMV
        else:unix: LIBS += -L$$OUT_PWD/../libmv/ -lLibMV
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/x64/release/libLibMV.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/x64/debug/libLibMV.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/win32/release/libLibMV.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/win32/debug/libLibMV.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/release/libLibMV.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/debug/libLibMV.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/release/libLibMV.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/debug/libLibMV.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/build/Release/libLibMV.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../libmv/build/Debug/libLibMV.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../libmv/libLibMV.a
}

} # static-libmv


################
# openMVG
static-openmvg {
INCLUDEPATH += $$OUT_PWD/../libs/openMVG
DEPENDPATH += $$OUT_PWD/../libs/openMVG

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../openMVG/x64/release/ -lopenMVG
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../openMVG/x64/debug/ -lopenMVG
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../openMVG/win32/release/ -lopenMVG
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../openMVG/win32/debug/ -lopenMVG
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../openMVG/release/ -lopenMVG
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../openMVG/debug/ -lopenMVG
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../openMVG/build/Release/ -lopenMVG
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../openMVG/build/Debug/ -lopenMVG
        else:unix: LIBS += -L$$OUT_PWD/../libmv/ -lLibMV
}

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/x64/release/libopenMVG.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/x64/debug/libopenMVG.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/win32/release/libopenMVG.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/win32/debug/libopenMVG.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/release/libopenMVG.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/debug/libopenMVG.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/release/libopenMVG.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/debug/libopenMVG.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/build/Release/libopenMVG.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../openMVG/build/Debug/libopenMVG.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../openMVG/libopenMVG.a
}

} # static-libmv

################
# BreakpadClient

static-breakpadclient {
!disable-breakpad {

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/x64/debug/ -lBreakpadClient
        } else {
                CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/release/ -lBreakpadClient
                CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/win32/debug/ -lBreakpadClient
        }
} else {
        win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/release/ -lBreakpadClient
        else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/debug/ -lBreakpadClient
        else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Release/ -lBreakpadClient
        else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../BreakpadClient/build/Debug/ -lBreakpadClient
        else:unix: LIBS += -L$$OUT_PWD/../BreakpadClient/ -lBreakpadClient
}

BREAKPAD_PATH = $$OUT_PWD/../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH
DEPENDPATH += $$BREAKPAD_PATH

win32-msvc*{
        CONFIG(64bit) {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/x64/debug/libBreakpadClient.lib
        } else {
                CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/release/libBreakpadClient.lib
                CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/win32/debug/libBreakpadClient.lib
        }
} else {
        win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/libBreakpadClient.a
        else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/libBreakpadClient.a
        else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/release/BreakpadClient.lib
        else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/debug/BreakpadClient.lib
        else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Release/libBreakpadClient.a
        else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/build/Debug/libBreakpadClient.a
        else:unix: PRE_TARGETDEPS += $$OUT_PWD/../BreakpadClient/libBreakpadClient.a
}

}
} # static-breakpadclient
