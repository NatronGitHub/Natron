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
# CONFIG += static-gui static-engine static-host-support static-breakpadclient

################
# Gui
static-gui {
CONFIG += static-engine
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
