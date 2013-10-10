#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Powiter
TEMPLATE = app
CONFIG += app warn_on c++11
CONFIG += moc rcc
CONFIG += openexr freetype2 ftgl boost ffmpeg eigen2 opengl qt expat #sanitizer
QT += gui core opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent


win32{
    CONFIG += glew
#ofx needs WINDOWS def
#microsoft compiler needs _MBCS to compile with the multi-byte character set.
    DEFINES += WINDOWS _MBCS COMPILED_FROM_DSP XML_STATIC OPENEXR_DLL FTGL_LIBRARY_STATIC NOMINMAX
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
     !macx {
         LIBS +=  -lGLU -ldl
     }
} #unix

CONFIG(debug, debug|release){
warning("Compiling in DEBUG mode.")
    DEFINES += POWITER_DEBUG
}


# When compiler is GCC check for at least version 4.7
*g++*{
  QMAKE_CXXFLAGS_RELEASE += -O3
  QMAKE_CXXFLAGS_WARN_ON += -Wextra -Wno-c++11-extensions
  GCCVer = $$system($$QMAKE_CXX --version)
  contains(GCCVer,[0-3]\\.[0-9]+.*) {
    error("At least GCC 4.6 is required.")
  } else {
    contains(GCCVer,4\\.[0-5].*) {
      error("At least GCC 4.6 is required.")
    } else {
      contains(GCCVer,4\\.6.*) {
        QMAKE_CXXFLAGS += -std=c++0x
      } else {
        QMAKE_CXXFLAGS += -std=c++11
      }
    }
  }
}
*clang* {
sanitizer{
    QMAKE_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls
    QMAKE_LFLAGS += -fsanitize=address -g
}
  QMAKE_CXXFLAGS_RELEASE += -O3
  QMAKE_CXXFLAGS_WARN_ON += -Wextra -Wno-c++11-extensions
  QMAKE_CXXFLAGS += -std=c++11
}

CONFIG(release, debug|release){
    release:DESTDIR = "build_$$TARGET/release"
}
CONFIG(debug, debug|release){
    debug:  DESTDIR = "build_$$TARGET/debug"
}

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
    Engine/ChannelSet.cpp \
    Engine/Format.cpp \
    Engine/Hash64.cpp \
    Engine/ImageFetcher.cpp \
    Engine/Knob.cpp \
    Engine/Lut.cpp \
    Engine/MemoryFile.cpp \
    Engine/Model.cpp \
    Engine/Node.cpp \
    Engine/OfxClipInstance.cpp \
    Engine/OfxHost.cpp \
    Engine/OfxImageEffectInstance.cpp \
    Engine/OfxNode.cpp \
    Engine/OfxOverlayInteract.cpp \
    Engine/OfxParamInstance.cpp \
    Engine/Row.cpp \
    Engine/Settings.cpp \
    Engine/TimeLine.cpp \
    Engine/Timer.cpp \
    Engine/VideoEngine.cpp \
    Engine/ViewerNode.cpp \
    Global/AppManager.cpp \
    Global/LibraryBinary.cpp \
    Global/main.cpp \
    Gui/ComboBox.cpp \
    Gui/Edge.cpp \
    Gui/Gui.cpp \
    Gui/InfoViewerWidget.cpp \
    Gui/KnobGui.cpp \
    Gui/NodeGraph.cpp \
    Gui/NodeGui.cpp \
    Gui/ScaleSlider.cpp \
    Gui/SequenceFileDialog.cpp \
    Gui/SettingsPanel.cpp \
    Gui/SpinBox.cpp \
    Gui/TabWidget.cpp \
    Gui/TextRenderer.cpp \
    Gui/Texture.cpp \
    Gui/TimeLineGui.cpp \
    Gui/ViewerGL.cpp \
    Gui/ViewerTab.cpp \
    Readers/Read.cpp \
    Readers/ReadExr.cpp \
    Readers/ReadFfmpeg_deprecated.cpp \
    Readers/ReadQt.cpp \
    Readers/Reader.cpp \
    Writers/Write.cpp \
    Writers/WriteExr.cpp \
    Writers/WriteQt.cpp \
    Writers/Writer.cpp \
    libs/OpenFX/HostSupport/src/ofxhBinary.cpp \
    libs/OpenFX/HostSupport/src/ofxhClip.cpp \
    libs/OpenFX/HostSupport/src/ofxhHost.cpp \
    libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp \
    libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp \
    libs/OpenFX/HostSupport/src/ofxhInteract.cpp \
    libs/OpenFX/HostSupport/src/ofxhMemory.cpp \
    libs/OpenFX/HostSupport/src/ofxhParam.cpp \
    libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp \
    libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp \
    libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp \
    libs/OpenFX/HostSupport/src/ofxhUtilities.cpp



HEADERS += \
    Engine/Cache.h \
    Engine/Box.h \
    Engine/ChannelSet.h \
    Engine/Format.h \
    Engine/Hash64.h \
    Engine/ImageFetcher.h \
    Engine/Knob.h \
    Engine/LRUcache.h \
    Engine/Lut.h \
    Engine/MemoryFile.h \
    Engine/Model.h \
    Engine/Node.h \
    Engine/OfxClipInstance.h \
    Engine/OfxHost.h \
    Engine/OfxImageEffectInstance.h \
    Engine/OfxNode.h \
    Engine/OfxOverlayInteract.h \
    Engine/OfxParamInstance.h \
    Engine/Row.h \
    Engine/Settings.h \
    Engine/Singleton.h \
    Engine/Sleeper.h \
    Engine/TimeLine.h \
    Engine/Timer.h \
    Engine/VideoEngine.h \
    Engine/FrameEntry.h \
    Engine/ViewerNode.h \
    Global/AppManager.h \
    Global/Enums.h \
    Global/GLIncludes.h \
    Global/GlobalDefines.h \
    Global/LibraryBinary.h \
    Global/Macros.h \
    Global/MemoryInfo.h \
    Gui/Button.h \
    Gui/ComboBox.h \
    Gui/Edge.h \
    Gui/Gui.h \
    Gui/InfoViewerWidget.h \
    Gui/KnobGui.h \
    Gui/LineEdit.h \
    Gui/NodeGraph.h \
    Gui/NodeGui.h \
    Gui/ScaleSlider.h \
    Gui/SequenceFileDialog.h \
    Gui/SettingsPanel.h \
    Gui/Shaders.h \
    Gui/SpinBox.h \
    Gui/TabWidget.h \
    Gui/TextRenderer.h \
    Gui/Texture.h \
    Gui/TimeLineGui.h \
    Gui/ViewerGL.h \
    Gui/ViewerTab.h \
    Readers/Read.h \
    Readers/ReadExr.h \
    Readers/ReadFfmpeg_deprecated.h \
    Readers/ReadQt.h \
    Readers/Reader.h \
    Writers/Write.h \
    Writers/WriteExr.h \
    Writers/WriteQt.h \
    Writers/Writer.h \
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

