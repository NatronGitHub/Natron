#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Engine
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc
CONFIG += boost qt
QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

warning("Engine is still using QtGui")
#QT -= gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


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

DEPENDPATH += $$PWD/../Global

SOURCES += \
    AppManager.cpp \
    ChannelSet.cpp \
    Curve.cpp \
    CurveSerialization.cpp \
    EffectInstance.cpp \
    Hash64.cpp \
    Image.cpp \
    Knob.cpp \
    KnobSerialization.cpp \
    KnobFactory.cpp \
    KnobFile.cpp \
    KnobTypes.cpp \
    LibraryBinary.cpp \
    Log.cpp \
    Lut.cpp \
    MemoryFile.cpp \
    Node.cpp \
    NodeSerialization.cpp \
    OfxClipInstance.cpp \
    OfxHost.cpp \
    OfxImageEffectInstance.cpp \
    OfxEffectInstance.cpp \
    OfxMemory.cpp \
    OfxOverlayInteract.cpp \
    OfxParamInstance.cpp \
    PluginMemory.cpp \
    Project.cpp \
    ProjectPrivate.cpp \
    ProjectSerialization.cpp \
    Row.cpp \
    Settings.cpp \
    TimeLine.cpp \
    Timer.cpp \
    VideoEngine.cpp \
    ViewerInstance.cpp

HEADERS += \
    AppManager.h \
    Cache.h \
    Curve.h \
    CurveSerialization.h \
    CurvePrivate.h \
    Rect.h \
    ChannelSet.h \
    EffectInstance.h \
    Format.h \
    FrameEntry.h \
    Hash64.h \
    ImageInfo.h \
    Image.h \
    Interpolation.h \
    Knob.h \
    KnobSerialization.h \
    KnobFactory.h \
    KnobFile.h \
    KnobTypes.h \
    LibraryBinary.h \
    Log.h \
    LRUHashTable.h \
    Lut.h \
    MemoryFile.h \
    Node.h \
    NodeSerialization.h \
    OfxClipInstance.h \
    OfxHost.h \
    OfxImageEffectInstance.h \
    OfxEffectInstance.h \
    OfxOverlayInteract.h \
    OfxMemory.h \
    OfxParamInstance.h \
    OverlaySupport.h \
    PluginMemory.h \
    Project.h \
    ProjectPrivate.h \
    ProjectSerialization.h \
    Row.h \
    Settings.h \
    Singleton.h \
    TextureRect.h \
    TextureRectSerialization.h \
    TimeLine.h \
    Timer.h \
    Variant.h \
    VideoEngine.h \
    ViewerInstance.h

# dirty dirty gui stuff is mixed with Engine

DEPENDPATH += $$PWD/../Gui

SOURCES += \
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
    ../Gui/FromQtEnums.cpp \
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
    ../Gui/QtDecoder.cpp \
    ../Gui/QtEncoder.cpp \
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
    ../Gui/ViewerTab.cpp

HEADERS += \
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
    ../Gui/FromQtEnums.h \
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
    ../Gui/QtDecoder.h \
    ../Gui/QtEncoder.h \
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
    ../Gui/ViewerTab.h
