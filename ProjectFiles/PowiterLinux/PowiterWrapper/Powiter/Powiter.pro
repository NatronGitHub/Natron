TARGET = Powiter
TEMPLATE = app
CONFIG += console
CONFIG += link_prl
QT+= core opengl gui widgets concurrent

SOURCES += \
    ../../../../Superviser/main.cpp

MOC_DIR = $(POWITER_ROOT)/ProjectFiles/GeneratedFiles
message(Now building Powiter...the working directory is:  $(POWITER_ROOT))

INCLUDEPATH += $(QT_INCLUDES)
INCLUDEPATH += $(BOOST_INCLUDES)
INCLUDEPATH += $(POWITER_ROOT)
INCLUDEPATH += $(OPENEXR_INCLUDES)
INCLUDEPATH += $(FREETYPE2_INCLUDES)
INCLUDEPATH += $(FREETYPE_INCLUDES)
INCLUDEPATH += $(FTGL_INCLUDES)
DEPENDPATH += $(OPENEXR_INCLUDES)
DEPENDPATH += $(QT_INCLUDES)
DEPENDPATH += $(FREETYPE2_INCLUDES)
DEPENDPATH += $(FREETYPE_INCLUDES)
DEPENDPATH += $(BOOST_INCLUDES)
DEPENDPATH += $(FTGL_INCLUDES)

#unix: PRE_TARGETDEPS += $(POWITER_ROOT)/ProjectFiles/PowiterLinux/build/PowiterLib/libPowiterLib.a
LIBS += -L$(POWITER_ROOT)/ProjectFiles/PowiterLinux/build/PowiterLib/ -lPowiterLib
