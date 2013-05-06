TARGET = Powiter
TEMPLATE = app
CONFIG += console link_prl
QT+= core opengl gui

SOURCES += \
    ../../../../Superviser/main.cpp

INCLUDEPATH += $$PWD/../../../../
INCLUDEPATH += /usr/include/freetype2

INCLUDEPATH +=$$PWD/../../../../libs/OpenExr_linux/include
DEPENDPATH += $$PWD/../../../../libs/OpenExr_linux/include



INCLUDEPATH += /usr/include
DEPENDPATH += /usr/include


unix: PRE_TARGETDEPS += $$PWD/../../build/PowiterLib/libPowiterLib.a
LIBS += -L$$PWD/../../build/PowiterLib/ -lPowiterLib
