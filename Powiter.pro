#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Powiter
TEMPLATE = app
CONFIG += app
CONFIG += moc rcc
CONFIG += openexr freetype2 ftgl boost ffmpeg eigen2 opengl qt expat debug
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
INCLUDEPATH += $$PWD/libs/OpenFX_extensions
INCLUDEPATH += $$PWD/libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/


DEFINES += OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE

SOURCES += \
    Superviser/main.cpp \
    Gui/Edge.cpp \
    Gui/NodeGraph.cpp \
    Gui/SettingsPanel.cpp \
    Gui/FeedbackSpinBox.cpp \
    Gui/ViewerGL.cpp \
    Gui/InfoViewerWidget.cpp \
    Gui/Knob.cpp \
    Gui/Gui.cpp \
    Gui/NodeGui.cpp \
    Gui/ScaleSlider.cpp \
    Gui/TextRenderer.cpp \
    Gui/Timeline.cpp \
    Gui/ViewerTab.cpp \
    Core/Box.cpp \
    Core/ChannelSet.cpp \
    Core/DataBuffer.cpp \
    Core/Format.cpp \
    Core/Hash.cpp \
    Core/Lut.cpp \
    Core/MemoryFile.cpp \
    Core/Model.cpp \
    Core/Node.cpp \
    Core/Row.cpp \
    Core/Settings.cpp \
    Core/Timer.cpp \
    Core/VideoEngine.cpp \
    Core/ViewerNode.cpp \
    Reader/Read.cpp \
    Reader/Reader.cpp \
    Reader/ReadExr.cpp \
    Reader/ReadFFMPEG_deprectated.cpp \
    Reader/ReadQt.cpp \
    Superviser/Controler.cpp \
    Core/ViewerCache.cpp \
    Gui/SequenceFileDialog.cpp \
    Writer/Writer.cpp \
    Writer/WriteQt.cpp \
    Writer/WriteExr.cpp \
    Writer/Write.cpp \
    Gui/TabWidget.cpp \
    Gui/ComboBox.cpp \
    Core/AbstractCache.cpp \
    Core/ImageFetcher.cpp \
    Core/Nodecache.cpp \
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
    Gui/Texture.cpp \
    Core/OfxNode.cpp \
    Core/OfxClipInstance.cpp \
    Core/OfxParamInstance.cpp


HEADERS += \
    Gui/Edge.h \
    Gui/ComboBox.h \
    Gui/NodeGraph.h \
    Gui/SettingsPanel.h \
    Gui/FeedbackSpinBox.h \
    Gui/ViewerGL.h \
    Gui/InfoViewerWidget.h \
    Gui/Knob.h \
    Gui/Gui.h \
    Gui/NodeGui.h \
    Gui/ScaleSlider.h \
    Gui/Shaders.h \
    Gui/TextRenderer.h \
    Gui/Timeline.h \
    Gui/ViewerTab.h \
    Core/Box.h \
    Core/ChannelSet.h \
    Core/DataBuffer.h \
    Core/Format.h \
    Core/Hash.h \
    Core/Lut.h \
    Core/MemoryFile.h \
    Core/Model.h \
    Core/Node.h \
    Core/ReferenceCountedObject.h \
    Core/Row.h \
    Core/Settings.h \
    Core/Singleton.h \
    Core/Sleeper.h \
    Core/Timer.h \
    Core/VideoEngine.h \
    Core/ViewerNode.h \
    Reader/Read.h \
    Reader/Reader.h \
    Reader/ReadExr.h \
    Reader/ReadFFMPEG_deprectated.h \
    Reader/ReadQt.h \
    Superviser/Controler.h \
    Superviser/Enums.h \
    Superviser/GLIncludes.h \
    Superviser/MemoryInfo.h \
    Superviser/GlobalDefines.h \
    Core/ViewerCache.h \
    Gui/SequenceFileDialog.h \
    Gui/TabWidget.h \
    Core/LRUcache.h \
    Core/ImageFetcher.h \
    Reader/exrCommons.h \
    Writer/Writer.h \
    Writer/WriteQt.h \
    Writer/WriteExr.h \
    Writer/Write.h \
    Gui/LineEdit.h \
    Core/AbstractCache.h \
    Core/Nodecache.h \
    Gui/Button.h \
    Gui/Texture.h \
    Core/OfxNode.h \
    Core/OfxClipInstance.h \
    Core/OfxParamInstance.h \
    libs/OpenFX/HostSupport/include/ofxhBinary.h \
    libs/OpenFX/HostSupport/include/ofxhClip.h \
    libs/OpenFX/HostSupport/include/ofxhHost.h \
    libs/OpenFX/HostSupport/include/ofxhImageEffect.h \
    libs/OpenFX/HostSupport/include/ofxhImageEffectAPI.h \
    libs/OpenFX/HostSupport/include/ofxhInteract.h \
    libs/OpenFX/HostSupport/include/ofxhMemory.h \
    libs/OpenFX/HostSupport/include/ofxhParam.h \
    libs/OpenFX/HostSupport/include/ofxhPluginAPICache.h \
    libs/OpenFX/HostSupport/include/ofxhPluginCache.h \
    libs/OpenFX/HostSupport/include/ofxhProgress.h \
    libs/OpenFX/HostSupport/include/ofxhPropertySuite.h \
    libs/OpenFX/HostSupport/include/ofxhTimeLine.h \
    libs/OpenFX/HostSupport/include/ofxhUtilities.h \
    libs/OpenFX/HostSupport/include/ofxhXml.h \
    libs/OpenFX/include/ofxCore.h \
    libs/OpenFX/include/ofxImageEffect.h \
    libs/OpenFX/include/ofxInteract.h \
    libs/OpenFX/include/ofxKeySyms.h \
    libs/OpenFX/include/ofxMemory.h \
    libs/OpenFX/include/ofxMessage.h \
    libs/OpenFX/include/ofxMultiThread.h \
    libs/OpenFX/include/ofxOpenGLRender.h \
    libs/OpenFX/include/ofxParam.h \
    libs/OpenFX/include/ofxParametricParam.h \
    libs/OpenFX/include/ofxPixels.h \
    libs/OpenFX/include/ofxProgress.h \
    libs/OpenFX/include/ofxProperty.h \
    libs/OpenFX/include/ofxTimeLine.h \
    libs/OpenFX_extensions//nuke/camera.h \
    libs/OpenFX_extensions//nuke/fnPublicOfxExtensions.h \
    libs/OpenFX_extensions//tuttle/ofxGraphAPI.h \
    libs/OpenFX_extensions//tuttle/ofxMetadata.h \
    libs/OpenFX_extensions//tuttle/ofxParam.h \
    libs/OpenFX_extensions//tuttle/ofxParamAPI.h \
    libs/OpenFX_extensions//tuttle/ofxReadWrite.h


INSTALLS += target

RESOURCES += \
    Resources.qrc


INSTALLS += data
OTHER_FILES += \
    config.pri

