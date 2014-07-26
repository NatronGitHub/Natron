#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Gui
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc rcc
CONFIG += boost glew opengl qt cairo
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
    CustomParamInteract.cpp \
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
    MultiInstancePanel.cpp \
    NodeBackDrop.cpp \
    NodeBackDropSerialization.cpp \
    NodeGraph.cpp \
    NodeGraphUndoRedo.cpp \
    NodeGui.cpp \
    NodeGuiSerialization.cpp \
    PreferencesPanel.cpp \
    ProjectGui.cpp \
    ProjectGuiSerialization.cpp \
    QtDecoder.cpp \
    QtEncoder.cpp \
    RenderingProgressDialog.cpp \
    RotoGui.cpp \
    RotoPanel.cpp \
    RotoUndoCommand.cpp \
    ScaleSliderQWidget.cpp \
    SequenceFileDialog.cpp \
    Shaders.cpp \
    SpinBox.cpp \
    SplashScreen.cpp \
    Splitter.cpp \
    TableModelView.cpp \
    TabWidget.cpp \
    TextRenderer.cpp \
    Texture.cpp \
    ticks.cpp \
    ToolButton.cpp \
    TimeLineGui.cpp \
    TrackerGui.cpp \
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
    CustomParamInteract.h \
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
    MultiInstancePanel.h \
    NodeBackDrop.h \
    NodeBackDropSerialization.h \
    NodeGraph.h \
    NodeGraphUndoRedo.h \
    NodeGui.h \
    NodeGuiSerialization.h \
    PreferencesPanel.h \
    ProjectGui.h \
    ProjectGuiSerialization.h \
    QtDecoder.h \
    QtEncoder.h \
    RenderingProgressDialog.h \
    RotoGui.h \
    RotoPanel.h \
    RotoUndoCommand.h \
    ScaleSliderQWidget.h \
    SequenceFileDialog.h \
    Shaders.h \
    SpinBox.h \
    SplashScreen.h \
    Splitter.h \
    TableModelView.h \
    TabWidget.h \
    TextRenderer.h \
    Texture.h \
    ticks.h \
    TimeLineGui.h \
    ToolButton.h \
    TrackerGui.h \
    ViewerGL.h \
    ViewerTab.h \
    ZoomContext.h


RESOURCES += \
    GuiResources.qrc
