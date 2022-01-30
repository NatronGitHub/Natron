# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2021 The Natron developers
# (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
CONFIG += boost opengl qt cairo python shiboken pyside 
QT += gui core network

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += concurrent
} else {
    QT += opengl
}

GUI_WRAPPER_DIR = Qt$${QT_MAJOR_VERSION}/NatronGui
ENGINE_WRAPPER_DIR = Qt$${QT_MAJOR_VERSION}/NatronEngine

CONFIG += glad-flags

include(../global.pri)

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
INCLUDEPATH += $$PWD/../Engine/$$ENGINE_WRAPPER_DIR
INCLUDEPATH += $$PWD/../Global

#qhttpserver
INCLUDEPATH += $$PWD/../libs/qhttpserver/src

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
    AutoHideToolBar.cpp \
    BackdropGui.cpp \
    Button.cpp \
    ChannelsComboBox.cpp \
    ClickableLabel.cpp \
    ColoredFrame.cpp \
    ColorSelectorWidget.cpp \
    ComboBox.cpp \
    CurveEditor.cpp \
    CurveEditorUndoRedo.cpp \
    CurveGui.cpp \
    CurveWidget.cpp \
    CurveWidgetDialogs.cpp \
    CurveWidgetPrivate.cpp \
    CustomParamInteract.cpp \
    DialogButtonBox.cpp \
    DockablePanel.cpp \
    DockablePanelPrivate.cpp \
    DockablePanelTabWidget.cpp \
    DocumentationManager.cpp \
    DopeSheet.cpp \
    DopeSheetEditor.cpp \
    DopeSheetEditorUndoRedo.cpp \
    DopeSheetHierarchyView.cpp \
    DopeSheetView.cpp \
    DotGui.cpp \
    Edge.cpp \
    EditExpressionDialog.cpp \
    EditScriptDialog.cpp \
    ExportGroupTemplateDialog.cpp \
    FileTypeMainWindow_win.cpp \
    FloatingWidget.cpp \
    GroupBoxLabel.cpp \
    Gui.cpp \
    Gui05.cpp \
    Gui10.cpp \
    Gui15.cpp \
    Gui20.cpp \
    Gui30.cpp \
    Gui40.cpp \
    Gui50.cpp \
    GuiAppInstance.cpp \
    GuiApplicationManager.cpp \
    GuiApplicationManager10.cpp \
    GuiApplicationManagerPrivate.cpp \
    GuiPrivate.cpp \
    Histogram.cpp \
    HostOverlay.cpp \
    InfoViewerWidget.cpp \
    KnobGui.cpp \
    KnobGui10.cpp \
    KnobGui20.cpp \
    KnobGuiBool.cpp \
    KnobGuiButton.cpp \
    KnobGuiChoice.cpp \
    KnobGuiColor.cpp \
    KnobGuiContainerHelper.cpp \
    KnobGuiFactory.cpp \
    KnobGuiFile.cpp \
    KnobGuiGroup.cpp \
    KnobGuiParametric.cpp \
    KnobGuiPrivate.cpp \
    KnobGuiSeparator.cpp \
    KnobGuiString.cpp \
    KnobGuiTable.cpp \
    KnobGuiValue.cpp \
    KnobUndoCommand.cpp \
    KnobWidgetDnD.cpp \
    Label.cpp \
    LineEdit.cpp \
    LinkToKnobDialog.cpp \
    LogWindow.cpp \
    ManageUserParamsDialog.cpp \
    Menu.cpp \
    MessageBox.cpp \
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
    NodeGraphRectItem.cpp \
    NodeGraphTextItem.cpp \
    NodeGraphUndoRedo.cpp \
    NodeGui.cpp \
    NodeGuiSerialization.cpp \
    NodeSettingsPanel.cpp \
    NodeViewerContext.cpp \
    PanelWidget.cpp \
    PickKnobDialog.cpp \
    PreferencesPanel.cpp \
    PreviewThread.cpp \
    ProgressPanel.cpp \
    ProgressTaskInfo.cpp \
    ProjectGui.cpp \
    ProjectGuiSerialization.cpp \
    PropertiesBinWrapper.cpp \
    PyGuiApp.cpp \
    PythonPanels.cpp \
    QtColorTriangle.cpp \
    QtEnumConvert.cpp \
    RenderStatsDialog.cpp \
    ResizableMessageBox.cpp \
    RightClickableWidget.cpp \
    RotoPanel.cpp \
    ScaleSliderQWidget.cpp \
    ScriptEditor.cpp \
    ScriptTextEdit.cpp \
    SequenceFileDialog.cpp \
    SerializableWindow.cpp \
    Shaders.cpp \
    SpinBox.cpp \
    SpinBoxValidator.cpp \
    SplashScreen.cpp \
    Splitter.cpp \
    TabGroup.cpp \
    TabWidget.cpp \
    TableModelView.cpp \
    TextRenderer.cpp \
    TimeLineGui.cpp \
    ToolButton.cpp \
    TrackerPanel.cpp \
    VerticalColorBar.cpp \
    ViewerGL.cpp \
    ViewerGLPrivate.cpp \
    ViewerTab.cpp \
    ViewerTab10.cpp \
    ViewerTab20.cpp \
    ViewerTab30.cpp \
    ViewerTab40.cpp \
    ViewerTabPrivate.cpp \
    ViewerToolButton.cpp \
    ticks.cpp \
    $${GUI_WRAPPER_DIR}/natrongui_module_wrapper.cpp \

HEADERS += \
    AboutWindow.h \
    ActionShortcuts.h \
    AddKnobDialog.h \
    AnimatedCheckBox.h \
    AutoHideToolBar.h \
    BackdropGui.h \
    Button.h \
    ChannelsComboBox.h \
    ClickableLabel.h \
    ColoredFrame.h \
    ColorSelectorWidget.h \
    ComboBox.h \
    CurveEditor.h \
    CurveEditorUndoRedo.h \
    CurveGui.h \
    CurveSelection.h \
    CurveWidget.h \
    CurveWidgetDialogs.h \
    CurveWidgetPrivate.h \
    CustomParamInteract.h \
    DialogButtonBox.h \
    DockablePanel.h \
    DockablePanelPrivate.h \
    DockablePanelTabWidget.h \
    DocumentationManager.h \
    DopeSheet.h \
    DopeSheetEditor.h \
    DopeSheetEditorUndoRedo.h \
    DopeSheetHierarchyView.h \
    DopeSheetView.h \
    DotGui.h \
    Edge.h \
    EditExpressionDialog.h \
    EditScriptDialog.h \
    ExportGroupTemplateDialog.h \
    FileTypeMainWindow_win.h \
    FloatingWidget.h \
    GroupBoxLabel.h \
    Gui.h \
    GuiAppInstance.h \
    GuiApplicationManager.h \
    GuiApplicationManagerPrivate.h \
    GuiDefines.h \
    GuiFwd.h \
    GuiMacros.h \
    GuiPrivate.h \
    Histogram.h \
    HostOverlay.h \
    InfoViewerWidget.h \
    KnobGui.h \
    KnobGuiBool.h \
    KnobGuiButton.h \
    KnobGuiChoice.h \
    KnobGuiColor.h \
    KnobGuiContainerHelper.h \
    KnobGuiContainerI.h \
    KnobGuiFactory.h \
    KnobGuiFile.h \
    KnobGuiGroup.h \
    KnobGuiParametric.h \
    KnobGuiSeparator.h \
    KnobGuiString.h \
    KnobGuiTable.h \
    KnobGuiValue.h \
    KnobUndoCommand.h \
    KnobWidgetDnD.h \
    Label.h \
    LineEdit.h \
    LinkToKnobDialog.h \
    LogWindow.h \
    ManageUserParamsDialog.h \
    Menu.h \
    MessageBox.h \
    MultiInstancePanel.h \
    NewLayerDialog.h \
    NodeBackdropSerialization.h \
    NodeClipBoard.h \
    NodeCreationDialog.h \
    NodeGraph.h \
    NodeGraphPrivate.h \
    NodeGraphRectItem.h \
    NodeGraphTextItem.h \
    NodeGraphUndoRedo.h \
    NodeGui.h \
    NodeGuiSerialization.h \
    NodeSettingsPanel.h \
    NodeViewerContext.h \
    PanelWidget.h \
    PickKnobDialog.h \
    PreferencesPanel.h \
    PreviewThread.h \
    ProgressPanel.h \
    ProgressTaskInfo.h \
    ProjectGui.h \
    ProjectGuiSerialization.h \
    PropertiesBinWrapper.h \
    PyGlobalGui.h \
    PyGuiApp.h \
    PythonPanels.h \
    QtColorTriangle.h \
    QtEnumConvert.h \
    RegisteredTabs.h \
    RenderStatsDialog.h \
    ResizableMessageBox.h \
    RightClickableWidget.h \
    RotoPanel.h \
    ScaleSliderQWidget.h \
    ScriptEditor.h \
    ScriptTextEdit.h \
    SequenceFileDialog.h \
    SerializableWindow.h \
    Shaders.h \
    SpinBox.h \
    SpinBoxValidator.h \
    SplashScreen.h \
    Splitter.h \
    TabGroup.h \
    TabWidget.h \
    TableModelView.h \
    TextRenderer.h \
    TimeLineGui.h \
    ToolButton.h \
    TrackerPanel.h \
    VerticalColorBar.h \
    ViewerGL.h \
    ViewerGLPrivate.h \
    ViewerTab.h \
    ViewerTabPrivate.h \
    ViewerToolButton.h \
    ZoomContext.h \
    ticks.h \
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
    $${GUI_WRAPPER_DIR}/natrongui_python.h \

GUI_GENERATED_SOURCES = \
    guiapp_wrapper \
    pyguiapplication_wrapper \
    pymodaldialog_wrapper \
    pypanel_wrapper \
    pytabwidget_wrapper \
    pyviewer_wrapper

for(name, GUI_GENERATED_SOURCES) {
    SOURCES += $${GUI_WRAPPER_DIR}/$${name}.cpp
    HEADERS += $${GUI_WRAPPER_DIR}/$${name}.h
}

RESOURCES += \
    GuiResources.qrc

# for i in `find Resources -type f |sort |uniq`; do fgrep -q "$i" GuiResources.qrc || echo "$i"; done |fgrep -v .git |fgrep -v .DS_Store
OTHER_FILES += \
    Resources/Fonts/Apache_License.txt \
    Resources/Images/MotionTypeRTS.png \
    Resources/Images/Other/natron_picto_tools.svg \
    Resources/Images/Other/natron_picto_viewer.svg \
    Resources/Images/motionTypeAffine.png \
    Resources/Images/motionTypeRT.png \
    Resources/Images/motionTypeS.png \
    Resources/Images/motionTypeT.png \
    Resources/Images/natronIcon.icns \
    Resources/Images/natronIcon.svg \
    Resources/Images/natronIcon256_windows.ico \
    Resources/Images/patternSize.png \
    Resources/Images/prevUserKey.png \
    Resources/Images/searchSize.png \
    Resources/Images/splashscreen.svg

macx {
OBJECTIVE_SOURCES += \
    $$PWD/../Gui/QtMac.mm
}
