
QT       += core network
QT       -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

TARGET = NatronRenderer
CONFIG += console
CONFIG -= app_bundle
CONFIG += moc
CONFIG += boost qt expat

TEMPLATE = app

#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/..


################
# Engine

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/release/ -lEngine
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/debug/ -lEngine
else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Release/ -lEngine
else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Engine/build/Debug/ -lEngine
else:unix: LIBS += -L$$OUT_PWD/../Engine/ -lEngine

INCLUDEPATH += $$PWD/../Engine
DEPENDPATH += $$PWD/../Engine

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/libEngine.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/libEngine.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/release/Engine.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/debug/Engine.lib
else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Release/libEngine.a
else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Engine/build/Debug/libEngine.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Engine/libEngine.a

################
# HostSupport

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/release/ -lHostSupport
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/debug/ -lHostSupport
else:*-xcode:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Release/ -lHostSupport
else:*-xcode:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../HostSupport/build/Debug/ -lHostSupport
else:unix: LIBS += -L$$OUT_PWD/../HostSupport/ -lHostSupport

INCLUDEPATH += $$PWD/../HostSupport
DEPENDPATH += $$PWD/../HostSupport

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/libHostSupport.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/libHostSupport.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/release/HostSupport.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/debug/HostSupport.lib
else:*-xcode:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Release/libHostSupport.a
else:*-xcode:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/build/Debug/libHostSupport.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../HostSupport/libHostSupport.a

include(../global.pri)
include(../config.pri)

SOURCES += \
    NatronRenderer_main.cpp

INSTALLS += target

