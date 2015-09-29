# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
    BackDropGui.cpp \
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
    DefaultOverlays.cpp \
    DockablePanel.cpp \
    DockablePanelPrivate.cpp \
    DockablePanelTabWidget.cpp \
    DopeSheet.cpp \
    DopeSheetView.cpp \
    DopeSheetEditorUndoRedo.cpp \
    DopeSheetHierarchyView.cpp \
    DopeSheetEditor.cpp \
    Edge.cpp \
    EditExpressionDialog.cpp \
    EditScriptDialog.cpp \
    FloatingWidget.cpp \
    QtEnumConvert.cpp \
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
    NodeBackDropSerialization.cpp \
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
    NodeGraphUndoRedo.cpp \
    NodeGui.cpp \
    NodeGuiSerialization.cpp \
    NodeSettingsPanel.cpp \
    PanelWidget.cpp \
    PickKnobDialog.cpp \
    PreferencesPanel.cpp \
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
    BackDropGui.h \
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
    DefaultOverlays.h \
    DockablePanel.h \
    DockablePanelPrivate.h \
    DockablePanelTabWidget.h \
    DopeSheet.h \
    DopeSheetView.h \
    DopeSheetEditorUndoRedo.h \
    DopeSheetHierarchyView.h \
    DopeSheetEditor.h \
    Edge.h \
    EditExpressionDialog.h \
    EditScriptDialog.h \
    FileTypeMainWindow_win.h \
    FloatingWidget.h \
    QtEnumConvert.h \
    GroupBoxLabel.h \
    Gui.h \
    GuiDefines.h \
    GuiApplicationManager.h \
    GuiApplicationManagerPrivate.h \
    GuiAppInstance.h \
    GuiAppWrapper.h \
    GuiMacros.h \
    GuiPrivate.h \
    GlobalGuiWrapper.h \
    Histogram.h \
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
    NodeBackDropSerialization.h \
    NodeClipBoard.h \
    NodeCreationDialog.h \
    NodeGraph.h \
    NodeGraphPrivate.h \
    NodeGraphUndoRedo.h \
    NodeGui.h \
    NodeGuiSerialization.h \
    NodeSettingsPanel.h \
    PanelWidget.h \
    PickKnobDialog.h \
    PreferencesPanel.h \
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
Resources/OpenColorIO-Configs/ChangeLog \
Resources/OpenColorIO-Configs/README \
Resources/OpenColorIO-Configs/aces_0.1.1/README \
Resources/OpenColorIO-Configs/aces_0.1.1/config.ocio \
Resources/OpenColorIO-Configs/aces_0.1.1/config_1_0_3.ocio \
Resources/OpenColorIO-Configs/aces_0.1.1/lutimages.zip \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/arri/logc800.py \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/arri/logc800.spi1d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/arri/logc_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/rrt/rrt_v0_1_1_dcdm.spi3d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/rrt/rrt_v0_1_1_p3d60.spi3d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/rrt/rrt_v0_1_1_p3dci.spi3d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/rrt/rrt_v0_1_1_rec709.spi3d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/rrt/rrt_v0_1_1_sRGB.spi3d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slog1.py \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slog1_10.spi1d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slog2.py \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slog2_10.spi1d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf35_to_aces.ctl \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf35_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf65_to_aces_3200.ctl \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf65_to_aces_3200.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf65_to_aces_5500.ctl \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/sony/slogf65_to_aces_5500.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_adx10_to_aces.ctl \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_adx10_to_cdd.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_adx16_to_aces.ctl \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_adx16_to_cdd.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_cdd_to_cid.spimtx \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_cid_to_rle.py \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_cid_to_rle.spi1d \
Resources/OpenColorIO-Configs/aces_0.1.1/luts/unbuild/adx_exp_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/README \
Resources/OpenColorIO-Configs/aces_0.7.1/config.ocio \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/AlexV3LogC_to_Rec709Prim.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/AlexV3LogC_to_p3dciPrim.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/AlexaV3_K1S1_LogC2Video_EE_nuke1d.cube \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/aceslog.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/acesproxy.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_adx10_to_cdd.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_adx16_to_cdd.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_cdd_to_cid.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_cid_to_rle.py \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_cid_to_rle.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/adx_exp_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/cineon.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/logc800.py \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/logc800.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/logc_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_dcdm_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_p3d60_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_p3dci_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec2020_d60sim_full_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec2020_d60sim_smpte_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec2020_full_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec2020_smpte_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec709_d60sim_full_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec709_d60sim_smpte_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec709_full_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rec709_smpte_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/odt_rgbMonitor_d60sim_100nits_inv_rrt_inv.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_dcdm.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_dcdm_p3d60.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_p3d60.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_p3dci.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec2020_d60sim_full_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec2020_d60sim_smpte_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec2020_full_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec2020_smpte_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec709_d60sim_full_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec709_d60sim_smpte_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec709_full_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rec709_smpte_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/rrt_odt_rgbMonitor_d60sim_100nits.spi3d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slog.py \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slog.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slog10.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slog2.py \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slogf35_to_aces.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slogf65_to_aces_3200.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/slogf65_to_aces_5500.spimtx \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/srgb.spi1d \
Resources/OpenColorIO-Configs/aces_0.7.1/luts/ten_bit_scale.spimtx \
Resources/OpenColorIO-Configs/nuke-default/config.ocio \
Resources/OpenColorIO-Configs/nuke-default/luts/alexalogc.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/cineon.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/panalog.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/ploglin.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/rec709.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/redlog.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/slog.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/srgb.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/srgbf.spi1d \
Resources/OpenColorIO-Configs/nuke-default/luts/viperlog.spi1d \
Resources/OpenColorIO-Configs/nuke-default/make.py \
Resources/OpenColorIO-Configs/spi-anim/config.ocio \
Resources/OpenColorIO-Configs/spi-anim/luts/correction.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/cpf.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/dt.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/hd10_to_xyz16.spi3d \
Resources/OpenColorIO-Configs/spi-anim/luts/hdOffset.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/htr_dlp_tweak.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/lm10.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/lm16.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/lm8.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/lmf.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/lnf_to_aces.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/mp.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/p3_to_xyz16.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/p3_to_xyz16_corrected_wp.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/p3d65_to_pdci.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/srgb_to_p3d65.spimtx \
Resources/OpenColorIO-Configs/spi-anim/luts/vd10.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/vd16.spi1d \
Resources/OpenColorIO-Configs/spi-anim/luts/vd8.spi1d \
Resources/OpenColorIO-Configs/spi-anim/makeconfig_anim.py \
Resources/OpenColorIO-Configs/spi-vfx/config.ocio \
Resources/OpenColorIO-Configs/spi-vfx/luts/colorworks_filmlg_to_p3.3dl \
Resources/OpenColorIO-Configs/spi-vfx/luts/cpf.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/diffuseTextureMultiplier.spimtx \
Resources/OpenColorIO-Configs/spi-vfx/luts/gn10.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/gn16.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/gn8.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/gnf.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/hdOffset.spimtx \
Resources/OpenColorIO-Configs/spi-vfx/luts/lg10.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/lg16.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/lgf.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/p3_to_xyz16.spimtx \
Resources/OpenColorIO-Configs/spi-vfx/luts/p3_to_xyz16_corrected_wp.spimtx \
Resources/OpenColorIO-Configs/spi-vfx/luts/spi_ocio_srgb_test.spi3d \
Resources/OpenColorIO-Configs/spi-vfx/luts/vd10.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/vd16.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/vd8.spi1d \
Resources/OpenColorIO-Configs/spi-vfx/luts/version_8_whitebalanced.spimtx \
Resources/OpenColorIO-Configs/spi-vfx/make_vfx_ocio.py

macx {
OBJECTIVE_SOURCES += \
    QtMac.mm
}
