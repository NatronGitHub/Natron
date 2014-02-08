TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG += moc rcc
CONFIG += boost glew opengl qt expat debug coverage
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets concurrent

INCLUDEPATH += google-test/include
INCLUDEPATH += google-test
INCLUDEPATH += google-mock/include
INCLUDEPATH += google-mock


DEFINES += NATRON_ENABLE_MOCK

QMAKE_CLEAN += ofxTestLog.txt test_dot_generator0.jpg

*g++* {
  QMAKE_CXXFLAGS += -ftemplate-depth-1024
  QMAKE_CXXFLAGS_WARN_ON += -Wextra
  c++11 {
    # check for at least version 4.7
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
}

universal {
  CONFIG += x86 x86_64
}


win32{
#ofx needs WINDOWS def
#microsoft compiler needs _MBCS to compile with the multi-byte character set.
    DEFINES += WINDOWS _MBCS COMPILED_FROM_DSP XML_STATIC  NOMINMAX
    DEFINES -= _UNICODE UNICODE
RC_FILE += Natron.rc
}

unix {
     #  on Unix systems, only the "boost" option needs to be defined in config.pri
     QT_CONFIG -= no-pkg-config
     CONFIG += link_pkgconfig
     glew:      PKGCONFIG += glew
     expat:     PKGCONFIG += expat
     !macx {
         LIBS +=  -lGLU -ldl
     }
} #unix

*-xcode {
  # redefine cxx flags as qmake tends to automatically add -O2 to xcode projects
  QMAKE_CFLAGS -= -O2
  QMAKE_CXXFLAGS -= -O2
  QMAKE_CXXFLAGS += -ftemplate-depth-1024

  # Qt 4.8.5's XCode generator has a bug and places moc_*.cpp files next to the sources instead of inside the build dir
  # However, setting the MOC_DIR doesn't fix that (Xcode build fails)
  # Simple rtule: don't use Xcode
  #MOC_DIR = $$OUT_PWD
  warning("Xcode generator wrongly places the moc files in the source directory. You thus cannot compile with different Qt versions using Xcode.")
}

*clang* {
  QMAKE_CXXFLAGS += -ftemplate-depth-1024
  sanitizer{
    QMAKE_CXXFLAGS += -fsanitize=address -fsanitize-undefined-trap-on-error -fno-omit-frame-pointer -fno-optimize-sibling-calls
    QMAKE_LFLAGS += -fsanitize=address -g
  }
  !sanitizer{
    QMAKE_CXXFLAGS_RELEASE += -O3
  }
  QMAKE_CXXFLAGS_WARN_ON += -Wextra -Wno-c++11-extensions
  c++11 {
    QMAKE_CXXFLAGS += -std=c++11
  }
}

coverage {
  QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
  QMAKE_LFLAGS += -fprofile-arcs -ftest-coverage
  QMAKE_CLEAN += $(OBJECTS_DIR)/*.gcda $(OBJECTS_DIR)/*.gcno
}

include(../config.pri)


DEFINES += OFX_EXTENSIONS_NUKE OFX_EXTENSIONS_TUTTLE OFX_EXTENSIONS_VEGAS OFX_SUPPORTS_PARAMETRIC OFX_EXTENSIONS_NATRON OFX_EXTENSIONS_TUTTLE
#DEFINES += OFX_SUPPORTS_MULTITHREAD

SOURCES += \
    google-test/src/gtest-all.cc \
    google-test/src/gtest_main.cc \
    google-mock/src/gmock-all.cc \
    ../Engine/ChannelSet.cpp \
    ../Engine/Curve.cpp \
    ../Engine/CurveSerialization.cpp \
    ../Engine/EffectInstance.cpp \
    ../Engine/Hash64.cpp \
    ../Engine/Image.cpp \
    ../Engine/Knob.cpp \
    ../Engine/KnobSerialization.cpp \
    ../Engine/KnobFactory.cpp \
    ../Engine/KnobFile.cpp \
    ../Engine/KnobTypes.cpp \
    ../Engine/Log.cpp \
    ../Engine/Lut.cpp \
    ../Engine/MemoryFile.cpp \
    ../Engine/Node.cpp \
    ../Engine/NodeSerialization.cpp \
    ../Engine/OfxClipInstance.cpp \
    ../Engine/OfxHost.cpp \
    ../Engine/OfxImageEffectInstance.cpp \
    ../Engine/OfxEffectInstance.cpp \
    ../Engine/OfxMemory.cpp \
    ../Engine/OfxOverlayInteract.cpp \
    ../Engine/OfxParamInstance.cpp \
    ../Engine/PluginMemory.cpp \
    ../Engine/Project.cpp \
    ../Engine/ProjectPrivate.cpp \
    ../Engine/ProjectSerialization.cpp \
    ../Engine/Row.cpp \
    ../Engine/Settings.cpp \
    ../Engine/TimeLine.cpp \
    ../Engine/Timer.cpp \
    ../Engine/VideoEngine.cpp \
    ../Engine/ViewerInstance.cpp \
    ../Global/AppManager.cpp \
    ../Global/LibraryBinary.cpp \
    ../Gui/AboutWindow.cpp \
    ../Gui/AnimatedCheckBox.cpp \
    ../Gui/AnimationButton.cpp \
    ../Gui/Button.cpp \
    ../Gui/ClickableLabel.cpp \
    ../Gui/ComboBox.cpp \
    ../Gui/CurveEditor.cpp \
    ../Gui/CurveEditorUndoRedo.cpp \
    ../Gui/CurveWidget.cpp \
    ../Gui/DockablePanel.cpp \
    ../Gui/Edge.cpp \
    ../Gui/GroupBoxLabel.cpp \
    ../Gui/Gui.cpp \
    ../Gui/InfoViewerWidget.cpp \
    ../Gui/KnobGui.cpp \
    ../Gui/KnobGuiFactory.cpp \
    ../Gui/KnobGuiFile.cpp \
    ../Gui/KnobGuiTypes.cpp \
    ../Gui/KnobUndoCommand.cpp \
    ../Gui/LineEdit.cpp \
    ../Gui/MenuWithToolTips.cpp \
    ../Gui/NodeGraph.cpp \
    ../Gui/NodeGui.cpp \
    ../Gui/NodeGuiSerialization.cpp \
    ../Gui/PreferencesPanel.cpp \
    ../Gui/ProcessHandler.cpp \
    ../Gui/ProjectGui.cpp \
    ../Gui/ProjectGuiSerialization.cpp \
    ../Gui/ScaleSliderQWidget.cpp \
    ../Gui/SequenceFileDialog.cpp \
    ../Gui/SpinBox.cpp \
    ../Gui/SplashScreen.cpp \
    ../Gui/TabWidget.cpp \
    ../Gui/TextRenderer.cpp \
    ../Gui/Texture.cpp \
    ../Gui/ticks.cpp \
    ../Gui/TimeLineGui.cpp \
    ../Gui/ViewerGL.cpp \
    ../Gui/ViewerTab.cpp \
    ../Readers/QtDecoder.cpp \
    ../Writers/QtEncoder.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhBinary.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhClip.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhHost.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhImageEffect.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhImageEffectAPI.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhInteract.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhMemory.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhParam.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPluginAPICache.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPluginCache.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhPropertySuite.cpp \
    ../libs/OpenFX/HostSupport/src/ofxhUtilities.cpp \
    ../libs/OpenFX_extensions/ofxhParametricParam.cpp \
    BaseTest.cpp \
    Hash64_Test.cpp \
    Image_Test.cpp \
    OfxEffectInstance_Mock.cpp

HEADERS += \
    ../Engine/Cache.h \
    ../Engine/Curve.h \
    ../Engine/CurveSerialization.h \
    ../Engine/CurvePrivate.h \
    ../Engine/Rect.h \
    ../Engine/ChannelSet.h \
    ../Engine/EffectInstance.h \
    ../Engine/Format.h \
    ../Engine/FrameEntry.h \
    ../Engine/Hash64.h \
    ../Engine/ImageInfo.h \
    ../Engine/Image.h \
    ../Engine/Interpolation.h \
    ../Engine/Knob.h \
    ../Engine/KnobSerialization.h \
    ../Engine/KnobFactory.h \
    ../Engine/KnobFile.h \
    ../Engine/KnobTypes.h \
    ../Engine/Log.h \
    ../Engine/LRUHashTable.h \
    ../Engine/Lut.h \
    ../Engine/MemoryFile.h \
    ../Engine/Node.h \
    ../Engine/NodeSerialization.h \
    ../Engine/OfxClipInstance.h \
    ../Engine/OfxHost.h \
    ../Engine/OfxImageEffectInstance.h \
    ../Engine/OfxEffectInstance.h \
    ../Engine/OfxMemory.h \
    ../Engine/OfxOverlayInteract.h \
    ../Engine/OfxParamInstance.h \
    ../Engine/PluginMemory.h \
    ../Engine/Project.h \
    ../Engine/ProjectPrivate.h \
    ../Engine/ProjectSerialization.h \
    ../Engine/Row.h \
    ../Engine/Settings.h \
    ../Engine/Singleton.h \
    ../Engine/TextureRect.h \
    ../Engine/TextureRectSerialization.h \
    ../Engine/TimeLine.h \
    ../Engine/Timer.h \
    ../Engine/Variant.h \
    ../Engine/VideoEngine.h \
    ../Engine/ViewerInstance.h \
    ../Global/AppManager.h \
    ../Global/Enums.h \
    ../Global/GLIncludes.h \
    ../Global/GlobalDefines.h \
    ../Global/LibraryBinary.h \
    ../Global/Macros.h \
    ../Global/MemoryInfo.h \
    ../Global/QtCompat.h \
    ../Gui/AboutWindow.h \
    ../Gui/AnimatedCheckBox.h \
    ../Gui/AnimationButton.h \
    ../Gui/Button.h \
    ../Gui/ClickableLabel.h \
    ../Gui/ComboBox.h \
    ../Gui/CurveEditor.h \
    ../Gui/CurveEditorUndoRedo.h \
    ../Gui/CurveWidget.h \
    ../Gui/DockablePanel.h \
    ../Gui/Edge.h \
    ../Gui/GroupBoxLabel.h \
    ../Gui/Gui.h \
    ../Gui/InfoViewerWidget.h \
    ../Gui/KnobGui.h \
    ../Gui/KnobGuiFactory.h \
    ../Gui/KnobGuiFile.h \
    ../Gui/KnobGuiTypes.h \
    ../Gui/KnobUndoCommand.h \
    ../Gui/LineEdit.h \
    ../Gui/MenuWithToolTips.h \
    ../Gui/NodeGraph.h \
    ../Gui/NodeGui.h \
    ../Gui/NodeGuiSerialization.h \
    ../Gui/PreferencesPanel.h \
    ../Gui/ProcessHandler.h \
    ../Gui/ProjectGui.h \
    ../Gui/ProjectGuiSerialization.h \
    ../Gui/ScaleSliderQWidget.h \
    ../Gui/SequenceFileDialog.h \
    ../Gui/Shaders.h \
    ../Gui/SpinBox.h \
    ../Gui/SplashScreen.h \
    ../Gui/TabWidget.h \
    ../Gui/TextRenderer.h \
    ../Gui/Texture.h \
    ../Gui/ticks.h \
    ../Gui/TimeLineGui.h \
    ../Gui/ViewerGL.h \
    ../Gui/ViewerTab.h \
    ../Readers/QtDecoder.h \
    ../Writers/QtEncoder.h \
    ../libs/OpenFX/HostSupport/include/ofxhBinary.h \
    ../libs/OpenFX/HostSupport/include/ofxhClip.h \
    ../libs/OpenFX/HostSupport/include/ofxhHost.h \
    ../libs/OpenFX/HostSupport/include/ofxhImageEffect.h \
    ../libs/OpenFX/HostSupport/include/ofxhImageEffectAPI.h \
    ../libs/OpenFX/HostSupport/include/ofxhInteract.h \
    ../libs/OpenFX/HostSupport/include/ofxhMemory.h \
    ../libs/OpenFX/HostSupport/include/ofxhParam.h \
    ../libs/OpenFX/HostSupport/include/ofxhPluginAPICache.h \
    ../libs/OpenFX/HostSupport/include/ofxhPluginCache.h \
    ../libs/OpenFX/HostSupport/include/ofxhProgress.h \
    ../libs/OpenFX/HostSupport/include/ofxhPropertySuite.h \
    ../libs/OpenFX/HostSupport/include/ofxhTimeLine.h \
    ../libs/OpenFX/HostSupport/include/ofxhUtilities.h \
    ../libs/OpenFX/HostSupport/include/ofxhXml.h \
    ../libs/OpenFX/include/ofxCore.h \
    ../libs/OpenFX/include/ofxImageEffect.h \
    ../libs/OpenFX/include/ofxInteract.h \
    ../libs/OpenFX/include/ofxKeySyms.h \
    ../libs/OpenFX/include/ofxMemory.h \
    ../libs/OpenFX/include/ofxMessage.h \
    ../libs/OpenFX/include/ofxMultiThread.h \
    ../libs/OpenFX/include/ofxOpenGLRender.h \
    ../libs/OpenFX/include/ofxParam.h \
    ../libs/OpenFX/include/ofxParametricParam.h \
    ../libs/OpenFX/include/ofxPixels.h \
    ../libs/OpenFX/include/ofxProgress.h \
    ../libs/OpenFX/include/ofxProperty.h \
    ../libs/OpenFX/include/ofxTimeLine.h \
    ../libs/OpenFX_extensions/tuttle/ofxGraphAPI.h \
    ../libs/OpenFX_extensions/tuttle/ofxMetadata.h \
    ../libs/OpenFX_extensions/tuttle/ofxParam.h \
    ../libs/OpenFX_extensions/tuttle/ofxParamAPI.h \
    ../libs/OpenFX_extensions/tuttle/ofxReadWrite.h \
    ../libs/OpenFX_extensions/ofxhParametricParam.h \
    ../libs/OpenFX/include/natron/IOExtensions.h \
    BaseTest.h \
    OfxEffectInstance_Mock.h


INCLUDEPATH += ../
INCLUDEPATH += ../libs/OpenFX/include
INCLUDEPATH += ../libs/OpenFX_extensions
INCLUDEPATH += ../libs/OpenFX/HostSupport/include
