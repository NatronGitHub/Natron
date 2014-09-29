#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Gui
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc rcc
CONFIG += boost glew opengl qt cairo sigar
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
    NodeCreationDialog.cpp \
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
    SerializableWindow.cpp \
    ShortCutEditor.cpp \
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
    ActionShortcuts.h \
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
    GuiMacros.h \
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
    NodeCreationDialog.h \
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
    SerializableWindow.h \
    ShortCutEditor.h \
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

# for i in `find Resources -type f |sort |uniq`; do fgrep -q "$i" GuiResources.qrc || echo "$i"; done |fgrep -v .git |fgrep -v .DS_Store
OTHER_FILES += \
Resources/Fonts/Apache_License.txt \
Resources/Images/Other/natron_picto_tools.svg \
Resources/Images/Other/natron_picto_viewer.svg \
Resources/Images/natronIcon.svg \
Resources/Images/natronIcon256_osx.icns \
Resources/Images/natronIcon256_windows.ico \
Resources/Images/splashscreen.svg \
Resources/Images/treeview_end.png \
Resources/Images/treeview_more.png \
Resources/Images/treeview_vline.png \
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
Resources/OpenColorIO-Configs/spi-vfx/make_vfx_ocio.py \
