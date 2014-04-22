#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Gui
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc rcc
CONFIG += boost glew opengl qt
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent


precompile_header {
  # Use Precompiled headers (PCH)
  # we specify PRECOMPILED_DIR, or qmake places precompiled headers in Natron/c++.pch, thus blocking the creation of the Unix executable
  PRECOMPILED_DIR = pch
  PRECOMPILED_HEADER = pch.h
}

include(../global.pri)
include(../config.pri)


#OpenFX C api includes and OpenFX c++ layer includes that are located in the submodule under /libs/OpenFX
INCLUDEPATH += $$PWD/../libs/OpenFX/include
DEPENDPATH  += $$PWD/../libs/OpenFX/include
INCLUDEPATH += $$PWD/../libs/OpenFX_extensions
DEPENDPATH  += $$PWD/../libs/OpenFX_extensions
INCLUDEPATH += $$PWD/../libs/OpenFX/HostSupport/include
DEPENDPATH  += $$PWD/../libs/OpenFX/HostSupport/include
INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../libs/SequenceParsing

DEPENDPATH += $$PWD/../Engine
DEPENDPATH += $$PWD/../Global

SOURCES += \
    AboutWindow.cpp \
    AnimatedCheckBox.cpp \
    AnimationButton.cpp \
    Button.cpp \
    ClickableLabel.cpp \
    ComboBox.cpp \
    CurveEditor.cpp \
    CurveEditorUndoRedo.cpp \
    CurveWidget.cpp \
    DockablePanel.cpp \
    Edge.cpp \
    FromQtEnums.cpp \
    GroupBoxLabel.cpp \
    Gui.cpp \
    GuiApplicationManager.cpp \
    GuiAppInstance.cpp \
    Histogram.cpp \
    InfoViewerWidget.cpp \
    KnobGui.cpp \
    KnobGuiFactory.cpp \
    KnobGuiFile.cpp \
    KnobGuiTypes.cpp \
    KnobUndoCommand.cpp \
    LineEdit.cpp \
    MenuWithToolTips.cpp \
    NodeGraph.cpp \
    NodeGui.cpp \
    NodeGuiSerialization.cpp \
    PreferencesPanel.cpp \
    ProjectGui.cpp \
    ProjectGuiSerialization.cpp \
    QtDecoder.cpp \
    QtEncoder.cpp \
    RenderingProgressDialog.cpp \
    RotoGui.cpp \
    ScaleSliderQWidget.cpp \
    SequenceFileDialog.cpp \
    Shaders.cpp \
    SpinBox.cpp \
    SplashScreen.cpp \
    Splitter.cpp \
    TabWidget.cpp \
    TextRenderer.cpp \
    Texture.cpp \
    ticks.cpp \
    ToolButton.cpp \
    TimeLineGui.cpp \
    ViewerGL.cpp \
    ViewerTab.cpp

HEADERS += \
    AboutWindow.h \
    AnimatedCheckBox.h \
    AnimationButton.h \
    Button.h \
    ClickableLabel.h \
    ComboBox.h \
    CurveEditor.h \
    CurveEditorUndoRedo.h \
    CurveWidget.h \
    DockablePanel.h \
    Edge.h \
    FromQtEnums.h \
    GroupBoxLabel.h \
    Gui.h \
    GuiApplicationManager.h \
    GuiAppInstance.h \
    Histogram.h \
    InfoViewerWidget.h \
    KnobGui.h \
    KnobGuiFactory.h \
    KnobGuiFile.h \
    KnobGuiTypes.h \
    KnobUndoCommand.h \
    LineEdit.h \
    MenuWithToolTips.h \
    NodeGraph.h \
    NodeGui.h \
    NodeGuiSerialization.h \
    PreferencesPanel.h \
    ProjectGui.h \
    ProjectGuiSerialization.h \
    QtDecoder.h \
    QtEncoder.h \
    RenderingProgressDialog.h \
    RotoGui.h \
    ScaleSliderQWidget.h \
    SequenceFileDialog.h \
    Shaders.h \
    SpinBox.h \
    SplashScreen.h \
    Splitter.h \
    TabWidget.h \
    TextRenderer.h \
    Texture.h \
    ticks.h \
    TimeLineGui.h \
    ToolButton.h \
    ViewerGL.h \
    ViewerTab.h \
    ZoomContext.h \
    ../Engine/AppInstance.h \
    ../Engine/AppManager.h \
    ../Engine/BlockingBackgroundRender.h \
    ../Engine/Cache.h \
    ../Engine/CacheEntry.h \
    ../Engine/Curve.h \
    ../Engine/CurveSerialization.h \
    ../Engine/CurvePrivate.h \
    ../Engine/ChannelSet.h \
    ../Engine/EffectInstance.h \
    ../Engine/Format.h \
    ../Engine/FrameEntry.h \
    ../Engine/FrameEntrySerialization.h \
    ../Engine/FrameParams.h \
    ../Engine/FrameParamsSerialization.h \
    ../Engine/Hash64.h \
    ../Engine/HistogramCPU.h \
    ../Engine/ImageInfo.h \
    ../Engine/Image.h \
    ../Engine/ImageSerialization.h \
    ../Engine/ImageParams.h \
    ../Engine/ImageParamsSerialization.h \
    ../Engine/Interpolation.h \
    ../Engine/Knob.h \
    ../Engine/KnobSerialization.h \
    ../Engine/KnobFactory.h \
    ../Engine/KnobFile.h \
    ../Engine/KnobTypes.h \
    ../Engine/LibraryBinary.h \
    ../Engine/Log.h \
    ../Engine/LRUHashTable.h \
    ../Engine/Lut.h \
    ../Engine/MemoryFile.h \
    ../Engine/Node.h \
    ../Engine/NonKeyParams.h \
    ../Engine/NonKeyParamsSerialization.h \
    ../Engine/NodeSerialization.h \
    ../Engine/OfxClipInstance.h \
    ../Engine/OfxHost.h \
    ../Engine/OfxImageEffectInstance.h \
    ../Engine/OfxEffectInstance.h \
    ../Engine/OfxOverlayInteract.h \
    ../Engine/OfxMemory.h \
    ../Engine/OfxParamInstance.h \
    ../Engine/OpenGLViewerI.h \
    ../Engine/OverlaySupport.h \
    ../Engine/Plugin.h \
    ../Engine/PluginMemory.h \
    ../Engine/ProcessHandler.h \
    ../Engine/Project.h \
    ../Engine/ProjectPrivate.h \
    ../Engine/ProjectSerialization.h \
    ../Engine/Rect.h \
    ../Engine/Settings.h \
    ../Engine/Singleton.h \
    ../Engine/StandardPaths.h \
    ../Engine/StringAnimationManager.h \
    ../Engine/TextureRect.h \
    ../Engine/TextureRectSerialization.h \
    ../Engine/ThreadStorage.h \
    ../Engine/TimeLine.h \
    ../Engine/Timer.h \
    ../Engine/Variant.h \
    ../Engine/VideoEngine.h \
    ../Engine/ViewerInstance.h \
    ../Global/Enums.h \
    ../Global/GitVersion.h \
    ../Global/GLIncludes.h \
    ../Global/GlobalDefines.h \
    ../Global/KeySymbols.h \
    ../Global/Macros.h \
    ../Global/MemoryInfo.h \
    ../Global/QtCompat.h

RESOURCES += \
    GuiResources.qrc
