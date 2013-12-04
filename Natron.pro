#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Natron
TEMPLATE = app
CONFIG += app warn_on c++11
CONFIG += moc rcc
CONFIG += openexr boost glew opengl qt expat
QT += gui core opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

PRECOMPILED_HEADER = pch.h

win32{
#ofx needs WINDOWS def
#microsoft compiler needs _MBCS to compile with the multi-byte character set.
    DEFINES += WINDOWS _MBCS COMPILED_FROM_DSP XML_STATIC OPENEXR_DLL  NOMINMAX
    DEFINES -= _UNICODE UNICODE
}

log{
    DEFINES += NATRON_LOG
}

unix {
     #  on Unix systems, only the "boost" option needs to be defined in config.pri
     QT_CONFIG -= no-pkg-config
     CONFIG += link_pkgconfig
     openexr:   PKGCONFIG += OpenEXR
     glew:      PKGCONFIG += glew
     expat:     PKGCONFIG += expat
     !macx {
         LIBS +=  -lGLU -ldl
     }
} #unix

CONFIG(debug, debug|release){
warning("Compiling in DEBUG mode.")
    DEFINES += NATRON_DEBUG
}

QMAKE_CXXFLAGS += -ftemplate-depth=500

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
    QMAKE_CXXFLAGS += -fsanitize=address -fsanitize-undefined-trap-on-error -fno-omit-frame-pointer -fno-optimize-sibling-calls
    QMAKE_LFLAGS += -fsanitize=address -g
}
!sanitizer{
  QMAKE_CXXFLAGS_RELEASE += -O3
}
  QMAKE_CXXFLAGS_WARN_ON += -Wextra -Wno-c++11-extensions
  QMAKE_CXXFLAGS += -std=c++11
}


include(config.pri)


#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/libs/OpenFX/include
INCLUDEPATH += $$PWD/libs/OpenFX_extensions
INCLUDEPATH += $$PWD/libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/


DEFINES += OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_VEGAS

SOURCES += \
    Engine/ChannelSet.cpp \
    Engine/Curve.cpp \
    Engine/EffectInstance.cpp \
    Engine/Hash64.cpp \
    Engine/Image.cpp \
    Engine/Knob.cpp \
    Engine/Log.cpp \
    Engine/Lut.cpp \
    Engine/MemoryFile.cpp \
    Engine/Node.cpp \
    Engine/OfxClipInstance.cpp \
    Engine/OfxHost.cpp \
    Engine/OfxImageEffectInstance.cpp \
    Engine/OfxEffectInstance.cpp \
    Engine/OfxOverlayInteract.cpp \
    Engine/OfxParamInstance.cpp \
    Engine/Project.cpp \
    Engine/Row.cpp \
    Engine/Settings.cpp \
    Engine/TimeLine.cpp \
    Engine/Timer.cpp \
    Engine/VideoEngine.cpp \
    Engine/ViewerInstance.cpp \
    Global/AppManager.cpp \
    Global/LibraryBinary.cpp \
    Global/main.cpp \
    Gui/ComboBox.cpp \
    Gui/CurveEditor.cpp \
    Gui/CurveWidget.cpp \
    Gui/Edge.cpp \
    Gui/Gui.cpp \
    Gui/InfoViewerWidget.cpp \
    Gui/KnobGui.cpp \
    Gui/NodeGraph.cpp \
    Gui/NodeGui.cpp \
    Gui/ScaleSlider.cpp \
    Gui/SequenceFileDialog.cpp \
    Gui/DockablePanel.cpp \
    Gui/SpinBox.cpp \
    Gui/TabWidget.cpp \
    Gui/TextRenderer.cpp \
    Gui/Texture.cpp \
    Gui/ticks.cpp \
    Gui/TimeLineGui.cpp \
    Gui/ViewerGL.cpp \
    Gui/ViewerTab.cpp \
    Readers/Decoder.cpp \
    Readers/ExrDecoder.cpp \
    Readers/QtDecoder.cpp \
    Readers/Reader.cpp \
    Writers/Encoder.cpp \
    Writers/ExrEncoder.cpp \
    Writers/QtEncoder.cpp \
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
    Engine/Curve.h \
    Engine/CurveSerialization.h \
    Engine/Rect.h \
    Engine/ChannelSet.h \
    Engine/EffectInstance.h \
    Engine/Format.h \
    Engine/FrameEntry.h \
    Engine/Hash64.h \
    Engine/ImageInfo.h \
    Engine/Image.h \
    Engine/Interpolation.h \
    Engine/Knob.h \
    Engine/KnobFile.h \
    Engine/KnobTypes.h \
    Engine/Log.h \
    Engine/LRUHashTable.h \
    Engine/Lut.h \
    Engine/MemoryFile.h \
    Engine/Node.h \
    Engine/OfxClipInstance.h \
    Engine/OfxHost.h \
    Engine/OfxImageEffectInstance.h \
    Engine/OfxEffectInstance.h \
    Engine/OfxOverlayInteract.h \
    Engine/OfxParamInstance.h \
    Engine/Project.h \
    Engine/Row.h \
    Engine/Settings.h \
    Engine/Singleton.h \
    Engine/TextureRect.h \
    Engine/TextureRectSerialization.h \
    Engine/TimeLine.h \
    Engine/Timer.h \
    Engine/Variant.h \
    Engine/VideoEngine.h \
    Engine/ViewerInstance.h \
    Global/AppManager.h \
    Global/Enums.h \
    Global/GLIncludes.h \
    Global/GlobalDefines.h \
    Global/LibraryBinary.h \
    Global/Macros.h \
    Global/MemoryInfo.h \
    Global/QtCompat.h \
    Gui/Button.h \
    Gui/ComboBox.h \
    Gui/CurveEditor.h \
    Gui/CurveWidget.h \
    Gui/Edge.h \
    Gui/Gui.h \
    Gui/InfoViewerWidget.h \
    Gui/KnobGui.h \
    Gui/LineEdit.h \
    Gui/NodeGraph.h \
    Gui/NodeGui.h \
    Gui/ScaleSlider.h \
    Gui/SequenceFileDialog.h \
    Gui/DockablePanel.h \
    Gui/Shaders.h \
    Gui/SpinBox.h \
    Gui/TabWidget.h \
    Gui/TextRenderer.h \
    Gui/Texture.h \
    Gui/ticks.h \
    Gui/TimeLineGui.h \
    Gui/ViewerGL.h \
    Gui/ViewerTab.h \
    Readers/Decoder.h \
    Readers/ExrDecoder.h \
    Readers/QtDecoder.h \
    Readers/Reader.h \
    Writers/Encoder.h \
    Writers/ExrEncoder.h \
    Writers/QtEncoder.h \
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
    libs/OpenFX_extensions//tuttle/ofxGraphAPI.h \
    libs/OpenFX_extensions//tuttle/ofxMetadata.h \
    libs/OpenFX_extensions//tuttle/ofxParam.h \
    libs/OpenFX_extensions//tuttle/ofxParamAPI.h \
    libs/OpenFX_extensions//tuttle/ofxReadWrite.h \
    Engine/CurvePrivate.h


INSTALLS += target

RESOURCES += \
    Resources.qrc


INSTALLS += data
OTHER_FILES += \
    config.pri

