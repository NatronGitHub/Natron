# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****

TARGET = Gui
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc rcc
CONFIG += boost glew opengl qt cairo python shiboken pyside
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

include(../global.pri)
include(../config.pri)

precompile_header {
  # Use Precompiled headers (PCH)
  # we specify PRECOMPILED_DIR, or qmake places precompiled headers in Natron/c++.pch, thus blocking the creation of the Unix executable
  PRECOMPILED_DIR = pch
  PRECOMPILED_HEADER = pch.h
}


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

INCLUDEPATH += $$PWD/../Engine
INCLUDEPATH += $$PWD/../Engine/NatronEngine
INCLUDEPATH += $$PWD/../Global

#To overcome wrongly generated #include <...> by shiboken
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/NatronGui
DEPENDPATH += $$PWD/NatronGui

win32-msvc* {
        CONFIG(64bit) {
                QMAKE_LFLAGS += /MACHINE:X64
        } else {
                QMAKE_LFLAGS += /MACHINE:X86
        }
}

SOURCES += \
    AboutWindow.cpp \
    ActionShortcuts.cpp \
    AddKnobDialog.cpp \
    AnimatedCheckBox.cpp \
    AnimationButton.cpp \
    AutoHideToolBar.cpp \
    BackdropGui.cpp \
    Button.cpp \
    ChannelsComboBox.cpp \
    ClickableLabel.cpp \
    ComboBox.cpp \
    CurveEditor.cpp \
    CurveEditorUndoRedo.cpp \
    CurveGui.cpp \
    CurveWidget.cpp \
    CurveWidgetDialogs.cpp \
    CurveWidgetPrivate.cpp \
    CustomParamInteract.cpp \
    DockablePanel.cpp \
    DockablePanelPrivate.cpp \
    DockablePanelTabWidget.cpp \
    DopeSheet.cpp \
    DopeSheetView.cpp \
    DopeSheetEditorUndoRedo.cpp \
    DopeSheetHierarchyView.cpp \
    DopeSheetEditor.cpp \
    DotGui.cpp \
    Edge.cpp \
    EditExpressionDialog.cpp \
    EditScriptDialog.cpp \
    ExportGroupTemplateDialog.cpp \
    FloatingWidget.cpp \
    QtEnumConvert.cpp \
    GroupBoxLabel.cpp \
    Gui.cpp \
    Gui05.cpp \
    Gui10.cpp \
    Gui15.cpp \
    Gui20.cpp \
    Gui30.cpp \
    Gui40.cpp \
    Gui50.cpp \
    GuiApplicationManager.cpp \
    GuiApplicationManager10.cpp \
    GuiApplicationManagerPrivate.cpp \
    GuiAppInstance.cpp \
    GuiAppWrapper.cpp \
    GuiPrivate.cpp \
    Histogram.cpp \
    HostOverlay.cpp \
    InfoViewerWidget.cpp \
    KnobGui.cpp \
    KnobGui10.cpp \
    KnobGui20.cpp \
    KnobGuiFactory.cpp \
    KnobGuiPrivate.cpp \
    KnobGuiFile.cpp \
    FileTypeMainWindow_win.cpp \
    KnobGuiButton.cpp \
    KnobGuiInt.cpp \
    KnobGuiBool.cpp \
    KnobGuiDouble.cpp \
    KnobGuiChoice.cpp \
    KnobGuiSeparator.cpp \
    KnobGuiColor.cpp \
    KnobGuiString.cpp \
    KnobGuiGroup.cpp \
    KnobGuiParametric.cpp \
    KnobUndoCommand.cpp \
    Label.cpp \
    LineEdit.cpp \
    LinkToKnobDialog.cpp \
    LogWindow.cpp \
    ManageUserParamsDialog.cpp \
    MessageBox.cpp \
    Menu.cpp \
    MultiInstancePanel.cpp \
    NewLayerDialog.cpp \
    NodeBackdropSerialization.cpp \
    NodeCreationDialog.cpp \
    NodeGraph.cpp \
    NodeGraph05.cpp \
    NodeGraph10.cpp \
    NodeGraph13.cpp \
    NodeGraph15.cpp \
    NodeGraph20.cpp \
    NodeGraph25.cpp \
    NodeGraph30.cpp \
    NodeGraph35.cpp \
    NodeGraph40.cpp \
    NodeGraph45.cpp \
    NodeGraphPrivate.cpp \
    NodeGraphPrivate10.cpp \
    NodeGraphTextItem.cpp \
    NodeGraphUndoRedo.cpp \
    NodeGui.cpp \
    NodeGuiSerialization.cpp \
    NodeSettingsPanel.cpp \
    PanelWidget.cpp \
    PickKnobDialog.cpp \
    PreferencesPanel.cpp \
    PreviewThread.cpp \
    ProjectGui.cpp \
    ProjectGuiSerialization.cpp \
    PropertiesBinWrapper.cpp \
    PythonPanels.cpp \
    QtDecoder.cpp \
    QtEncoder.cpp \
    RenderingProgressDialog.cpp \
    RenderStatsDialog.cpp \
    ResizableMessageBox.cpp \
    RightClickableWidget.cpp \
    RotoGui.cpp \
    RotoPanel.cpp \
    RotoUndoCommand.cpp \
    ScaleSliderQWidget.cpp \
    ScriptEditor.cpp \
    ScriptTextEdit.cpp \
    SequenceFileDialog.cpp \
    Shaders.cpp \
    SerializableWindow.cpp \
    ShortCutEditor.cpp \
    SpinBox.cpp \
    SpinBoxValidator.cpp \
    SplashScreen.cpp \
    Splitter.cpp \
    TabGroup.cpp \
    TableModelView.cpp \
    TabWidget.cpp \
    TextRenderer.cpp \
    Texture.cpp \
    ticks.cpp \
    ToolButton.cpp \
    TimeLineGui.cpp \
    TrackerGui.cpp \
    Utils.cpp \
    VerticalColorBar.cpp \
    ViewerGL.cpp \
    ViewerGLPrivate.cpp \
    ViewerTab.cpp \
    ViewerTab10.cpp \
    ViewerTab20.cpp \
    ViewerTab30.cpp \
    ViewerTab40.cpp \
    ViewerTabPrivate.cpp \
    NatronGui/natrongui_module_wrapper.cpp \
    NatronGui/pyguiapplication_wrapper.cpp \
    NatronGui/guiapp_wrapper.cpp \
    NatronGui/pymodaldialog_wrapper.cpp \
    NatronGui/pypanel_wrapper.cpp \
    NatronGui/pytabwidget_wrapper.cpp \
    NatronGui/pyviewer_wrapper.cpp


HEADERS += \
    AboutWindow.h \
    ActionShortcuts.h \
    AddKnobDialog.h \
    AnimatedCheckBox.h \
    AnimationButton.h \
    AutoHideToolBar.h \
    BackdropGui.h \
    Button.h \
    ChannelsComboBox.h \
    ClickableLabel.h \
    ComboBox.h \
    CurveEditor.h \
    CurveEditorUndoRedo.h \
    CurveGui.h \
    CurveSelection.h \
    CurveWidget.h \
    CurveWidgetDialogs.h \
    CurveWidgetPrivate.h \
    CustomParamInteract.h \
    DockablePanel.h \
    DockablePanelPrivate.h \
    DockablePanelTabWidget.h \
    DopeSheet.h \
    DopeSheetView.h \
    DopeSheetEditorUndoRedo.h \
    DopeSheetHierarchyView.h \
    DopeSheetEditor.h \
    DotGui.h \
    Edge.h \
    EditExpressionDialog.h \
    EditScriptDialog.h \
    ExportGroupTemplateDialog.h \
    FileTypeMainWindow_win.h \
    FloatingWidget.h \
    QtEnumConvert.h \
    GroupBoxLabel.h \
    Gui.h \
    GuiApplicationManager.h \
    GuiApplicationManagerPrivate.h \
    GuiAppInstance.h \
    GuiAppWrapper.h \
    GuiDefines.h \
    GuiFwd.h \
    GuiMacros.h \
    GuiPrivate.h \
    GlobalGuiWrapper.h \
    Histogram.h \
    HostOverlay.h \
    InfoViewerWidget.h \
    KnobGui.h \
    KnobGuiFactory.h \
    KnobGuiFile.h \
    KnobGuiButton.h \
    KnobGuiInt.h \
    KnobGuiBool.h \
    KnobGuiDouble.h \
    KnobGuiChoice.h \
    KnobGuiSeparator.h \
    KnobGuiColor.h \
    KnobGuiString.h \
    KnobGuiGroup.h \
    KnobGuiParametric.h \
    KnobUndoCommand.h \
    Label.h \
    LineEdit.h \
    LinkToKnobDialog.h \
    LogWindow.h \
    ManageUserParamsDialog.h \
    MessageBox.h \
    Menu.h \
    MultiInstancePanel.h \
    NewLayerDialog.h \
    NodeBackdropSerialization.h \
    NodeClipBoard.h \
    NodeCreationDialog.h \
    NodeGraph.h \
    NodeGraphPrivate.h \
    NodeGraphTextItem.h \
    NodeGraphUndoRedo.h \
    NodeGui.h \
    NodeGuiSerialization.h \
    NodeSettingsPanel.h \
    PanelWidget.h \
    PickKnobDialog.h \
    PreferencesPanel.h \
    PreviewThread.h \
    ProjectGui.h \
    ProjectGuiSerialization.h \
    PropertiesBinWrapper.h \
    Pyside_Gui_Python.h \
    PythonPanels.h \
    QtDecoder.h \
    QtEncoder.h \
    RegisteredTabs.h \
    RenderingProgressDialog.h \
    RenderStatsDialog.h \
    ResizableMessageBox.h \
    RightClickableWidget.h \
    RotoGui.h \
    RotoPanel.h \
    RotoUndoCommand.h \
    ScaleSliderQWidget.h \
    ScriptEditor.h \
    ScriptTextEdit.h \
    SequenceFileDialog.h \
    Shaders.h \
    SerializableWindow.h \
    ShortCutEditor.h \
    SpinBox.h \
    SpinBoxValidator.h \
    SplashScreen.h \
    Splitter.h \
    TabGroup.h \
    TableModelView.h \
    TabWidget.h \
    TextRenderer.h \
    Texture.h \
    ticks.h \
    TimeLineGui.h \
    ToolButton.h \
    TrackerGui.h \
    Utils.h \
    VerticalColorBar.h \
    ViewerGL.h \
    ViewerGLPrivate.h \
    ViewerTab.h \
    ViewerTabPrivate.h \
    ZoomContext.h \
    ../libs/OpenFX/include/ofxCore.h \
    ../libs/OpenFX/include/ofxDialog.h \
    ../libs/OpenFX/include/ofxImageEffect.h \
    ../libs/OpenFX/include/ofxInteract.h \
    ../libs/OpenFX/include/ofxKeySyms.h \
    ../libs/OpenFX/include/ofxMemory.h \
    ../libs/OpenFX/include/ofxMessage.h \
    ../libs/OpenFX/include/ofxMultiThread.h \
    ../libs/OpenFX/include/ofxNatron.h \
    ../libs/OpenFX/include/ofxOpenGLRender.h \
    ../libs/OpenFX/include/ofxParam.h \
    ../libs/OpenFX/include/ofxParametricParam.h \
    ../libs/OpenFX/include/ofxPixels.h \
    ../libs/OpenFX/include/ofxProgress.h \
    ../libs/OpenFX/include/ofxProperty.h \
    ../libs/OpenFX/include/ofxSonyVegas.h \
    ../libs/OpenFX/include/ofxTimeLine.h \
    ../libs/OpenFX/include/nuke/camera.h \
    ../libs/OpenFX/include/nuke/fnOfxExtensions.h \
    ../libs/OpenFX/include/nuke/fnPublicOfxExtensions.h \
    ../libs/OpenFX/include/tuttle/ofxReadWrite.h \
    ../libs/OpenFX_extensions/ofxhParametricParam.h \
    NatronGui/natrongui_python.h \
    NatronGui/pyguiapplication_wrapper.h \
    NatronGui/guiapp_wrapper.h \
    NatronGui/pymodaldialog_wrapper.h \
    NatronGui/pypanel_wrapper.h \
    NatronGui/pytabwidget_wrapper.h \
    NatronGui/pyviewer_wrapper.h


RESOURCES += \
    GuiResources.qrc

# for i in `find Resources -type f |sort |uniq`; do fgrep -q "$i" GuiResources.qrc || echo "$i"; done |fgrep -v .git |fgrep -v .DS_Store
OTHER_FILES += \
Resources/Fonts/Apache_License.txt \
Resources/Images/Other/natron_picto_tools.svg \
Resources/Images/Other/natron_picto_viewer.svg \
Resources/Images/natronIcon.svg \
Resources/Images/natronIcon256_osx.icns \
Resources/Images/natronIcon256_windows.ico \
Resources/Images/splashscreen.svg \


macx {
OBJECTIVE_SOURCES += \
    QtMac.mm
}
