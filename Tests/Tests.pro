# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2023 The Natron developers
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

TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG += moc rcc
CONFIG += boost boost-serialization-lib opengl qt cairo python shiboken pyside 
CONFIG += static-gui static-engine static-host-support static-breakpadclient static-libmv static-openmvg static-ceres static-libtess
QT += gui core opengl network widgets concurrent
equals(QT_MAJOR_VERSION, 6): QT += openglwidgets

CONFIG += openmvg-flags glad-flags

CONFIG += expat

INCLUDEPATH += google-test/include
INCLUDEPATH += google-test
INCLUDEPATH += google-mock/include
INCLUDEPATH += google-mock


QMAKE_CLEAN += ofxTestLog.txt test_dot_generator0.jpg

include(../global.pri)

# Windows: stack reserve of the main thread (see NATRON_MAIN_THREAD_STACK_SIZE in global.pri)
win32-g++: QMAKE_LFLAGS += -Wl,--stack,$$NATRON_MAIN_THREAD_STACK_SIZE
win32-msvc*: QMAKE_LFLAGS += /STACK:$$NATRON_MAIN_THREAD_STACK_SIZE
# and the test that checks the stack sizes (DeepGraphRender_Test.cpp) needs it
DEFINES += NATRON_MAIN_THREAD_STACK_SIZE=$$NATRON_MAIN_THREAD_STACK_SIZE

# bundled third-party code: see warnings.pri
NATRON_THIRD_PARTY_SOURCES += \
    google-test/src/gtest-all.cc \
    google-mock/src/gmock-all.cc

# NaNGuards_FastMath.cpp is compiled with -ffast-math by the CMake build only (qmake can't set
# per-file flags); here it runs the same tests without -ffast-math.
SOURCES += \
    BaseTest.cpp \
    CLArgs_Test.cpp \
    Curve_Test.cpp \
    DeepGraphRender_Test.cpp \
    DiskCache_Test.cpp \
    ExpressionDepsRenderArgs_Test.cpp \
    ExpressionSetError_Test.cpp \
    FileSystemModel_Test.cpp \
    Hash64_Test.cpp \
    ImageCache_Test.cpp \
    Image_Test.cpp \
    KnobDuplicate_Test.cpp \
    KnobExpressionCache_Test.cpp \
    KnobInvalidExpression_Test.cpp \
    KnobLockOrder_Test.cpp \
    KnobFile_Test.cpp \
    KnobSlave_Test.cpp \
    KnobStringAnimation_Test.cpp \
    LibTess_Test.cpp \
    LoadPythonScript_Test.cpp \
    Lut_Test.cpp \
    Markdown_Test.cpp \
    MultiInputLinks_Test.cpp \
    NaNGuards_FastMath.cpp \
    NaNGuards_Test.cpp \
    NodeGroup_Test.cpp \
    NodePresets_Test.cpp \
    NodePreview_Test.cpp \
    NoDialogs_Test.cpp \
    OfxBytesParam_Test.cpp \
    OfxContexts_Test.cpp \
    OfxCPURenderSupport_Test.cpp \
    OfxClipGetImage_Test.cpp \
    OfxCustomParamInterp_Test.cpp \
    OfxDrawSuite_Test.cpp \
    OfxHostProperties_Test.cpp \
    OfxImageMemory_Test.cpp \
    OfxInstanceChanged_Test.cpp \
    OfxInstanceEdit_Test.cpp \
    OfxMultiThreadSuite_Test.cpp \
    OfxMultiplePAR_Test.cpp \
    OfxParamKeys_Test.cpp \
    OfxParamAnimating_Test.cpp \
    OfxParamRange_Test.cpp \
    OfxParamCopy_Test.cpp \
    OfxOpenGLSupport_Test.cpp \
    OfxOverlayInput_Test.cpp \
    OfxParamInteract_Test.cpp \
    OfxParamUndo_Test.cpp \
    OfxPluginObsolete_Test.cpp \
    OfxPluginScan_Test.cpp \
    OfxProjectLoad_Test.cpp \
    OfxQuestion_Test.cpp \
    OfxSequenceRenderArgs_Test.cpp \
    OfxSingleInstance_Test.cpp \
    OfxSuiteStatus_Test.cpp \
    OfxSyncPrivateData_Test.cpp \
    OfxTestPlugin_Test.cpp \
    OpenGLRenderFailed_Test.cpp \
    OSGLContext_Test.cpp \
    ParamRandom_Test.cpp \
    PluginGrouping_Test.cpp \
    PrecompLoad_Test.cpp \
    ProjectAutoSave_Test.cpp \
    PyEffectEquality_Test.cpp \
    PythonScript_Test.cpp \
    ProjectFileFormat_Test.cpp \
    PythonExpressionScope_Test.cpp \
    ReadNodeOverlay_Test.cpp \
    RenderThreadSafety_Test.cpp \
    RenderThreads_Test.cpp \
    RotoPaint_Test.cpp \
    SequentialRender_Test.cpp \
    ShuffleAutoConnect_Test.cpp \
    StrUtilsFormat_Test.cpp \
    Tracker_Test.cpp \
    ViewerCache_Test.cpp \
    ViewerTexture_Test.cpp \
    WriteNodeViews_Test.cpp \
    wmain.cpp

HEADERS += \
    BaseTest.h \
    NaNGuards_FastMath.h \
    OfxTestPlugin.h

# the OFX test plug-in (OfxTestPlugin/OfxTestPlugin.pro), loaded through OFX_PLUGIN_PATH by wmain.cpp
include(OfxTestPlugin/OfxTestPlugin.pri)
DEFINES += NATRON_TESTS_OFX_PLUGIN_DIR=\\\"$${NATRON_TEST_OFX_PLUGIN_DIR}\\\"
DEFINES += NATRON_TESTS_OFX_PLUGIN_BINARY=\\\"$${NATRON_TEST_OFX_BINARY_DIR}/NatronTestPlugin.ofx\\\"
# the project files read by ProjectFileFormat_Test.cpp
DEFINES += NATRON_TESTS_DATA_DIR=\\\"$$PWD/data\\\"

include(../warnings.pri)
