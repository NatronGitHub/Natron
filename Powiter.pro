#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Powiter
TEMPLATE = app
CONFIG += app
CONFIG += moc rcc
CONFIG += openexr ftgl freetype2 boost ffmpeg eigen2 opengl qt expat debug
QT += gui core opengl concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

win32{
    CONFIG += glew
#ofx needs WINDOWS def
#microsoft compiler needs _MBCS to compile with the multi-byte character set.
    DEFINES += WINDOWS _MBCS COMPILED_FROM_DSP XML_STATIC
    DEFINES -= _UNICODE UNICODE
}

unix {
     #  on Unix systems, only the "boost" option needs to be defined in config.pri
     QT_CONFIG -= no-pkg-config
     CONFIG += link_pkgconfig
     openexr:   PKGCONFIG += OpenEXR
     ftgl:      PKGCONFIG += ftgl
     ffmpeg:    PKGCONFIG += libavcodec libavformat libavutil libswscale libavdevice libavfilter
     freetype2: PKGCONFIG += freetype2
     eigen2:    PKGCONFIG += eigen2
     expat:     PKGCONFIG += expat
} #unix



debug{
warning("Compiling in DEBUG mode.")
}

release:DESTDIR = "build_$$TARGET/release"
debug:  DESTDIR = "build_$$TARGET/debug"

OBJECTS_DIR = "$$DESTDIR/.obj"

#Removed these as the Xcode project would mess-up and it is not important anyway.
MOC_DIR = "$$OBJECTS_DIR"
RCC_DIR = "$$OBJECTS_DIR"
UI_DIR = "$$OBJECTS_DIR"

unix:macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7
unix:macx:QMAKE_LFLAGS += -mmacosx-version-min=10.7
unix:macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
include(config.pri)


#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/libs/OpenFX/include
INCLUDEPATH += $$PWD/libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/


SOURCES += \
    Superviser/main.cpp \
    Gui/arrowGUI.cpp \
    Gui/DAG.cpp \
    Gui/DAGQuickNode.cpp \
    Gui/dockableSettings.cpp \
    Gui/FeedbackSpinBox.cpp \
    Gui/GLViewer.cpp \
    Gui/InfoViewerWidget.cpp \
    Gui/knob.cpp \
    Gui/knob_callback.cpp \
    Gui/mainGui.cpp \
    Gui/node_ui.cpp \
    Gui/ScaleSlider.cpp \
    Gui/textRenderer.cpp \
    Gui/timeline.cpp \
    Gui/viewerTab.cpp \
    Core/Box.cpp \
    Core/channels.cpp \
    Core/DataBuffer.cpp \
    Core/displayFormat.cpp \
    Core/hash.cpp \
    Core/lookUpTables.cpp \
    Core/mappedfile.cpp \
    Core/metadata.cpp \
    Core/model.cpp \
    Core/node.cpp \
    Core/row.cpp \
    Core/settings.cpp \
    Core/Timer.cpp \
    Core/VideoEngine.cpp \
    Core/viewerNode.cpp \
    Reader/Read.cpp \
    Reader/Reader.cpp \
    Reader/readExr.cpp \
    Reader/readffmpeg.cpp \
    Reader/readQt.cpp \
    Superviser/controler.cpp \
    Core/viewercache.cpp \
    Gui/framefiledialog.cpp \
    Writer/Writer.cpp \
    Writer/writeQt.cpp \
    Writer/writeExr.cpp \
    Writer/Write.cpp \
    Gui/tabwidget.cpp \
    Gui/comboBox.cpp \
    Core/abstractCache.cpp \
    Core/imagefetcher.cpp \
    Core/nodecache.cpp \
    libs/OpenFX/HostSupport/src/ofxhUtilities.cpp \
    libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp \
    libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp \
    libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp \
    libs/OpenFX/HostSupport/src/ofxhParam.cpp \
    libs/OpenFX/HostSupport/src/ofxhMemory.cpp \
    libs/OpenFX/HostSupport/src/ofxhInteract.cpp \
    libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp \
    libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp \
    libs/OpenFX/HostSupport/src/ofxhHost.cpp \
    libs/OpenFX/HostSupport/src/ofxhClip.cpp \
    libs/OpenFX/HostSupport/src/ofxhBinary.cpp \
    Gui/texture.cpp \
    Core/ofxnode.cpp \
    Core/ofxclipinstance.cpp \
    Core/ofxparaminstance.cpp


HEADERS += \
    Gui/arrowGUI.h \
    Gui/comboBox.h \
    Gui/DAG.h \
    Gui/DAGQuickNode.h \
    Gui/dockableSettings.h \
    Gui/FeedbackSpinBox.h \
    Gui/GLViewer.h \
    Gui/InfoViewerWidget.h \
    Gui/knob.h \
    Gui/knob_callback.h \
    Gui/mainGui.h \
    Gui/node_ui.h \
    Gui/ScaleSlider.h \
    Gui/shaders.h \
    Gui/textRenderer.h \
    Gui/timeline.h \
    Gui/viewerTab.h \
    Core/Box.h \
    Core/channels.h \
    Core/DataBuffer.h \
    Core/displayFormat.h \
    Core/hash.h \
    Core/lookUpTables.h \
    Core/lutclasses.h \
    Core/mappedfile.h \
    Core/metadata.h \
    Core/model.h \
    Core/node.h \
    Core/referenceCountedObj.h \
    Core/row.h \
    Core/settings.h \
    Core/singleton.h \
    Core/sleeper.h \
    Core/Timer.h \
    Core/VideoEngine.h \
    Core/viewerNode.h \
    Reader/Read.h \
    Reader/Reader.h \
    Reader/readExr.h \
    Reader/readffmpeg.h \
    Reader/readQt.h \
    Superviser/controler.h \
    Superviser/Enums.h \
    Superviser/gl_OsDependent.h \
    Superviser/MemoryInfo.h \
    Superviser/powiterFn.h \
    Core/viewercache.h \
    Gui/framefiledialog.h \
    Gui/tabwidget.h \
    Core/LRUcache.h \
    Core/imagefetcher.h \
    Reader/exrCommons.h \
    Writer/Writer.h \
    Writer/writeQt.h \
    Writer/writeExr.h \
    Writer/Write.h \
    Gui/lineEdit.h \
    Core/abstractCache.h \
    Core/nodecache.h \
    Gui/button.h \
    Gui/texture.h \
    Core/ofxnode.h \
    Core/ofxclipinstance.h \
    Core/ofxparaminstance.h



INSTALLS += target

RESOURCES += \
    Resources.qrc


INSTALLS += data
OTHER_FILES += \
    config.pri

