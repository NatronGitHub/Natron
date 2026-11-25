# md4c, the CommonMark Markdown parser, and its HTML renderer (see README.Natron).
# libs/md4c/CMakeLists.txt is the CMake equivalent: keep them in sync.

TEMPLATE = lib
TARGET = md4c
CONFIG+= staticlib
CONFIG-= qt
DEPENDPATH += . src
INCLUDEPATH += . src

include(../../global.pri)
include(../../libs.pri)

# bundled third-party code: no warnings (-w), as NATRON_THIRD_PARTY_NO_WARNINGS in CMake
CONFIG -= warn_on
CONFIG += warn_off

OBJECTS_DIR = build
MOC_DIR = build
DESTDIR = build

# Input
HEADERS += \
           src/entity.h \
           src/md4c.h \
           src/md4c-html.h
SOURCES += \
           src/entity.c \
           src/md4c.c \
           src/md4c-html.c
