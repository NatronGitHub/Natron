# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
# Cairo is still the default renderer for Roto
!enable-osmesa {
   CONFIG += enable-cairo
}
CONFIG += moc rcc
CONFIG += boost opengl qt python shiboken pyside fontconfig
enable-cairo: CONFIG += cairo
QT += gui core opengl network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

CONFIG += glad-flags yaml-cpp-flags

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
INCLUDEPATH += $$PWD/../Serialization
INCLUDEPATH += $$PWD/..
INCLUDEPATH += $$PWD/../libs/SequenceParsing

DEPENDPATH += $$PWD/../Engine
DEPENDPATH += $$PWD/../Global

INCLUDEPATH += $$PWD/../Engine
INCLUDEPATH += $$PWD/../Engine/NatronEngine
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
    AnimItemBase.cpp \
    AnimatedCheckBox.cpp \
    AnimationModule.cpp \
    AnimationModuleBase.cpp \
    AnimationModuleEditor.cpp \
    AnimationModuleSelectionModel.cpp \
    AnimationModuleTreeView.cpp \
    AnimationModuleUndoRedo.cpp \
    AnimationModuleView.cpp \
    AnimationModuleViewCurveWidget.cpp \
    AnimationModuleViewCurveWidgetPrivate.cpp \
    AnimationModuleViewDopeSheet.cpp \
    AnimationModuleViewPrivate.cpp \
    AutoHideToolBar.cpp \
    BackdropGui.cpp \
    Button.cpp \
    CachedFramesThread.cpp \
    ClickableLabel.cpp \
    ColoredFrame.cpp \
    ComboBox.cpp \
    CurveGui.cpp \
    CurveWidgetDialogs.cpp \
    CustomParamInteract.cpp \
    DialogButtonBox.cpp \
    DockablePanel.cpp \
    DockablePanelPrivate.cpp \
    DockablePanelTabWidget.cpp \
    DocumentationManager.cpp \
    DotGui.cpp \
    Edge.cpp \
    EditExpressionDialog.cpp \
    EditNodeViewerContextDialog.cpp \
    EditScriptDialog.cpp \
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
    GuiGLContext.cpp \
    GuiPrivate.cpp \
    Histogram.cpp \
    InfoViewerWidget.cpp \
    KnobAnim.cpp \
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
    KnobGuiKeyFrameMarkers.cpp \
    KnobGuiParametric.cpp \
    KnobGuiPrivate.cpp \
    KnobGuiSeparator.cpp \
    KnobGuiString.cpp \
    KnobGuiTable.cpp \
    KnobGuiValue.cpp \
    KnobGuiWidgets.cpp \
    KnobItemsTableGui.cpp \
    KnobWidgetDnD.cpp \
    Label.cpp \
    LineEdit.cpp \
    LinkToKnobDialog.cpp \
    LogWindow.cpp \
    ManageUserParamsDialog.cpp \
    Menu.cpp \
    MessageBox.cpp \
    NewLayerDialog.cpp \
    NodeAnim.cpp \
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
    NodeSettingsPanel.cpp \
    NodeViewerContext.cpp \
    PanelWidget.cpp \
    PickKnobDialog.cpp \
    PreferencesPanel.cpp \
    PreviewThread.cpp \
    ProgressPanel.cpp \
    ProgressTaskInfo.cpp \
    ProjectGui.cpp \
    PropertiesBinWrapper.cpp \
    PyGlobalGui.cpp \
    PyGuiApp.cpp \
    PythonPanels.cpp \
    QtEnumConvert.cpp \
    RenderStatsDialog.cpp \
    ResizableMessageBox.cpp \
    RightClickableWidget.cpp \
    ScaleSliderQWidget.cpp \
    ScriptEditor.cpp \
    ScriptTextEdit.cpp \
    SequenceFileDialog.cpp \
    Shaders.cpp \
    SpinBox.cpp \
    SpinBoxValidator.cpp \
    SplashScreen.cpp \
    Splitter.cpp \
    StyledKnobWidgetBase.cpp \
    TabGroup.cpp \
    TabWidget.cpp \
    TableItemAnim.cpp \
    TableModelView.cpp \
    TextRenderer.cpp \
    TimeLineGui.cpp \
    ToolButton.cpp \
    UndoCommand_qt.cpp \
    VerticalColorBar.cpp \
    ViewerGL.cpp \
    ViewerGLPrivate.cpp \
    ViewerTab.cpp \
    ViewerTab10.cpp \
    ViewerTab40.cpp \
    ViewerTabNodesUI.cpp \
    ViewerTabOverlays.cpp \
    ViewerTabPrivate.cpp \
    ViewerToolButton.cpp \
    ticks.cpp \
    NatronGui/guiapp_wrapper.cpp \
    NatronGui/pyguiapplication_wrapper.cpp \
    NatronGui/pymodaldialog_wrapper.cpp \
    NatronGui/pypanel_wrapper.cpp \
    NatronGui/pytabwidget_wrapper.cpp \

HEADERS += \
    AboutWindow.h \
    ActionShortcuts.h \
    AddKnobDialog.h \
    AnimItemBase.h \
    AnimatedCheckBox.h \
    AnimationModule.h \
    AnimationModuleBase.h \
    AnimationModuleEditor.h \
    AnimationModuleSelectionModel.h \
    AnimationModuleTreeView.h \
    AnimationModuleUndoRedo.h \
    AnimationModuleView.h \
    AnimationModuleViewPrivate.h \
    AutoHideToolBar.h \
    BackdropGui.h \
    Button.h \
    CachedFramesThread.h \
    ClickableLabel.h \
    ColoredFrame.h \
    ComboBox.h \
    CurveGui.h \
    CurveWidgetDialogs.h \
    CurveWidgetPrivate.h \
    CustomParamInteract.h \
    DialogButtonBox.h \
    DockablePanel.h \
    DockablePanelPrivate.h \
    DockablePanelTabWidget.h \
    DocumentationManager.h \
    DotGui.h \
    Edge.h \
    EditExpressionDialog.h \
    EditNodeViewerContextDialog.h \
    EditScriptDialog.h \
    FileTypeMainWindow_win.h \
    FloatingWidget.h \
    GroupBoxLabel.h \
    Gui.h \
    GuiAppInstance.h \
    GuiApplicationManager.h \
    GuiApplicationManagerPrivate.h \
    GuiDefines.h \
    GuiFwd.h \
    GuiGLContext.h \
    GuiMacros.h \
    GuiPrivate.h \
    Histogram.h \
    InfoViewerWidget.h \
    KnobAnim.h \
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
    KnobGuiKeyFrameMarkers.h \
    KnobGuiParametric.h \
    KnobGuiPrivate.h \
    KnobGuiSeparator.h \
    KnobGuiString.h \
    KnobGuiTable.h \
    KnobGuiValue.h \
    KnobGuiWidgets.h \
    KnobItemsTableGui.h \
    KnobWidgetDnD.h \
    Label.h \
    LineEdit.h \
    LinkToKnobDialog.h \
    LogWindow.h \
    ManageUserParamsDialog.h \
    Menu.h \
    MessageBox.h \
    NewLayerDialog.h \
    NodeAnim.h \
    NodeCreationDialog.h \
    NodeGraph.h \
    NodeGraphPrivate.h \
    NodeGraphRectItem.h \
    NodeGraphTextItem.h \
    NodeGraphUndoRedo.h \
    NodeGui.h \
    NodeSettingsPanel.h \
    NodeViewerContext.h \
    PanelWidget.h \
    PickKnobDialog.h \
    PreferencesPanel.h \
    PreviewThread.h \
    ProgressPanel.h \
    ProgressTaskInfo.h \
    ProjectGui.h \
    PropertiesBinWrapper.h \
    PyGlobalGui.h \
    PyGuiApp.h \
    Pyside_Gui_Python.h \
    PythonPanels.h \
    QtEnumConvert.h \
    RegisteredTabs.h \
    RenderStatsDialog.h \
    ResizableMessageBox.h \
    RightClickableWidget.h \
    ScaleSliderQWidget.h \
    ScriptEditor.h \
    ScriptTextEdit.h \
    SequenceFileDialog.h \
    Shaders.h \
    SpinBox.h \
    SpinBoxValidator.h \
    SplashScreen.h \
    Splitter.h \
    StyledKnobWidgetBase.h \
    TabGroup.h \
    TabWidget.h \
    TableItemAnim.h \
    TableModelView.h \
    TextRenderer.h \
    TimeLineGui.h \
    ToolButton.h \
    UndoCommand_qt.h \
    VerticalColorBar.h \
    ViewerGL.h \
    ViewerGLPrivate.h \
    ViewerTab.h \
    ViewerTabPrivate.h \
    ViewerToolButton.h \
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
    NatronGui/guiapp_wrapper.h \
    NatronGui/natrongui_python.h \
    NatronGui/pyguiapplication_wrapper.h \
    NatronGui/pymodaldialog_wrapper.h \
    NatronGui/pypanel_wrapper.h \
    NatronGui/pytabwidget_wrapper.h \

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
    Resources/Images/splashscreen.svg \

macx {
OBJECTIVE_SOURCES += \
    $$PWD/../Gui/QtMac.mm
}
