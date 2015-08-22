#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Engine
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc
CONFIG += boost qt expat cairo python shiboken pyside
QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

# Don't uncomment the following: pyside requires QtGui, because PySide/QtCore/pyside_qtcore_python.h includes qtextdocument.h
#QT -= gui


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
INCLUDEPATH += $$PWD/../Global
INCLUDEPATH += $$PWD/../libs/SequenceParsing

#To overcome wrongly generated #include <...> by shiboken
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/../Global

win32-msvc* {
	CONFIG(64bit) {
                QMAKE_LFLAGS += /MACHINE:X64
        } else {
                QMAKE_LFLAGS += /MACHINE:X86
	}
}

# XCode clang 3.5 without optimization generates code that crashes
#(Natron on OSX, XCode 6, Spaceship_Natron.ntp)
*-xcode {
  #QMAKE_CXXFLAGS += -O1
}

SOURCES += \
    AppInstance.cpp \
    AppInstanceWrapper.cpp \
    AppManager.cpp \
    BackDrop.cpp \
    BlockingBackgroundRender.cpp \
    CoonsRegularization.cpp \
    Curve.cpp \
    CurveSerialization.cpp \
    DiskCacheNode.cpp \
    Dot.cpp \
    EffectInstance.cpp \
    FileDownloader.cpp \
    FileSystemModel.cpp \
    FitCurve.cpp \
    FrameEntry.cpp \
    FrameKey.cpp \
    FrameParamsSerialization.cpp \
    GroupInput.cpp \
    GroupOutput.cpp \
    Hash64.cpp \
    HistogramCPU.cpp \
    Image.cpp \
    ImageConvert.cpp \
    ImageCopyChannels.cpp \
    ImageComponents.cpp \
    ImageKey.cpp \
    ImageMaskMix.cpp \
    ImageParamsSerialization.cpp \
    Interpolation.cpp \
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
    NodeGroup.cpp \
    NodeGroupWrapper.cpp \
    NodeWrapper.cpp \
    NonKeyParams.cpp \
    NonKeyParamsSerialization.cpp \
    NodeSerialization.cpp \
    NodeGroupSerialization.cpp \
    NoOpBase.cpp \
    OfxClipInstance.cpp \
    OfxHost.cpp \
    OfxImageEffectInstance.cpp \
    OfxEffectInstance.cpp \
    OfxMemory.cpp \
    OfxOverlayInteract.cpp \
    OfxParamInstance.cpp \
    OutputSchedulerThread.cpp \
    ParameterWrapper.cpp \
    Plugin.cpp \
    PluginMemory.cpp \
    ProcessHandler.cpp \
    Project.cpp \
    ProjectPrivate.cpp \
    ProjectSerialization.cpp \
    PySideCompat.cpp \
    Rect.cpp \
    RotoContext.cpp \
    RotoPaint.cpp \
    RotoSerialization.cpp  \
    RotoSmear.cpp \
    RotoWrapper.cpp \
    ScriptObject.cpp \
    Settings.cpp \
    StandardPaths.cpp \
    StringAnimationManager.cpp \
    TimeLine.cpp \
    Timer.cpp \
    Transform.cpp \
    ViewerInstance.cpp \
    ../libs/SequenceParsing/SequenceParsing.cpp \
    NatronEngine/natronengine_module_wrapper.cpp \
    NatronEngine/natron_wrapper.cpp \
    NatronEngine/app_wrapper.cpp \
    NatronEngine/effect_wrapper.cpp \
    NatronEngine/intparam_wrapper.cpp \
    NatronEngine/param_wrapper.cpp \
    NatronEngine/doubleparam_wrapper.cpp \
    NatronEngine/colortuple_wrapper.cpp \
    NatronEngine/double2dparam_wrapper.cpp \
    NatronEngine/double2dtuple_wrapper.cpp \
    NatronEngine/double3dparam_wrapper.cpp \
    NatronEngine/double3dtuple_wrapper.cpp \
    NatronEngine/int2dparam_wrapper.cpp \
    NatronEngine/int2dtuple_wrapper.cpp \
    NatronEngine/int3dparam_wrapper.cpp \
    NatronEngine/int3dtuple_wrapper.cpp \
    NatronEngine/colorparam_wrapper.cpp \
    NatronEngine/booleanparam_wrapper.cpp \
    NatronEngine/buttonparam_wrapper.cpp \
    NatronEngine/choiceparam_wrapper.cpp \
    NatronEngine/fileparam_wrapper.cpp \
    NatronEngine/outputfileparam_wrapper.cpp \
    NatronEngine/stringparam_wrapper.cpp \
    NatronEngine/stringparambase_wrapper.cpp \
    NatronEngine/pathparam_wrapper.cpp \
    NatronEngine/animatedparam_wrapper.cpp \
    NatronEngine/parametricparam_wrapper.cpp \
    NatronEngine/group_wrapper.cpp \
    NatronEngine/beziercurve_wrapper.cpp \
    NatronEngine/itembase_wrapper.cpp \
    NatronEngine/layer_wrapper.cpp \
    NatronEngine/roto_wrapper.cpp \
    NatronEngine/groupparam_wrapper.cpp \
    NatronEngine/pageparam_wrapper.cpp \
    NatronEngine/appsettings_wrapper.cpp \
    NatronEngine/pycoreapplication_wrapper.cpp \
    NatronEngine/userparamholder_wrapper.cpp \
    NatronEngine/rectd_wrapper.cpp \
    NatronEngine/recti_wrapper.cpp

HEADERS += \
    AppInstance.h \
    AppInstanceWrapper.h \
    AppManager.h \
    BackDrop.h \
    BlockingBackgroundRender.h \
    Cache.h \
    CacheEntry.h \
    CoonsRegularization.h \
    Curve.h \
    CurveSerialization.h \
    CurvePrivate.h \
    DockablePanelI.h \
    Dot.h \
    DiskCacheNode.h \
    EffectInstance.h \
    FileDownloader.h \
    FileSystemModel.h \
    FitCurve.h \
    Format.h \
    FrameEntry.h \
    FrameKey.h \
    FrameEntrySerialization.h \
    FrameParams.h \
    FrameParamsSerialization.h \
    GlobalFunctionsWrapper.h \
    GroupInput.h \
    GroupOutput.h \
    Hash64.h \
    HistogramCPU.h \
    ImageInfo.h \
    Image.h \
    ImageComponents.h \
    ImageKey.h \
    ImageLocker.h \
    ImageSerialization.h \
    ImageParams.h \
    ImageParamsSerialization.h \
    Interpolation.h \
    KeyHelper.h \
    Knob.h \
    KnobGuiI.h \
    KnobImpl.h \
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
    NodeGroup.h \
    NodeGroupSerialization.h \
    NodeGroupWrapper.h \
    NodeGraphI.h \
    NodeWrapper.h \
    NodeGuiI.h \
    NonKeyParams.h \
    NonKeyParamsSerialization.h \
    NodeSerialization.h \
    NoOpBase.h \
    OfxClipInstance.h \
    OfxHost.h \
    OfxImageEffectInstance.h \
    OfxEffectInstance.h \
    OfxOverlayInteract.h \
    OfxMemory.h \
    OfxParamInstance.h \
    OpenGLViewerI.h \
    OutputSchedulerThread.h \
    OverlaySupport.h \
    ParameterWrapper.h \
    Plugin.h \
    PluginMemory.h \
    ProcessHandler.h \
    Project.h \
    ProjectPrivate.h \
    ProjectSerialization.h \
    Pyside_Engine_Python.h \
    Rect.h \
    RotoContext.h \
    RotoContextPrivate.h \
    RotoPaint.h \
    RotoSerialization.h \
    RotoSmear.h \
    RotoWrapper.h \
    ScriptObject.h \
    Settings.h \
    Singleton.h \
    StandardPaths.h \
    StringAnimationManager.h \
    TextureRect.h \
    TextureRectSerialization.h \
    ThreadStorage.h \
    TimeLine.h \
    Timer.h \
    Transform.h \
    Variant.h \
    ViewerInstance.h \
    ViewerInstancePrivate.h \
    ../Global/Enums.h \
    ../Global/GitVersion.h \
    ../Global/GLIncludes.h \
    ../Global/GlobalDefines.h \
    ../Global/KeySymbols.h \
    ../Global/Macros.h \
    ../Global/MemoryInfo.h \
    ../Global/QtCompat.h \
    ../libs/SequenceParsing/SequenceParsing.h \
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
    NatronEngine/natronengine_python.h \
    NatronEngine/natron_wrapper.h \
    NatronEngine/app_wrapper.h \
    NatronEngine/effect_wrapper.h \
    NatronEngine/intparam_wrapper.h \
    NatronEngine/param_wrapper.h \
    NatronEngine/doubleparam_wrapper.h \
    NatronEngine/colortuple_wrapper.h \
    NatronEngine/double2dparam_wrapper.h \
    NatronEngine/double2dtuple_wrapper.h \
    NatronEngine/double3dparam_wrapper.h \
    NatronEngine/double3dtuple_wrapper.h \
    NatronEngine/int2dparam_wrapper.h \
    NatronEngine/int2dtuple_wrapper.h \
    NatronEngine/int3dparam_wrapper.h \
    NatronEngine/int3dtuple_wrapper.h \
    NatronEngine/colorparam_wrapper.h \
    NatronEngine/booleanparam_wrapper.h \
    NatronEngine/buttonparam_wrapper.h \
    NatronEngine/choiceparam_wrapper.h \
    NatronEngine/fileparam_wrapper.h \
    NatronEngine/outputfileparam_wrapper.h \
    NatronEngine/stringparam_wrapper.h \
    NatronEngine/stringparambase_wrapper.h \
    NatronEngine/pathparam_wrapper.h \
    NatronEngine/animatedparam_wrapper.h \
    NatronEngine/parametricparam_wrapper.h \
    NatronEngine/group_wrapper.h \
    NatronEngine/beziercurve_wrapper.h \
    NatronEngine/itembase_wrapper.h \
    NatronEngine/layer_wrapper.h \
    NatronEngine/roto_wrapper.h \
    NatronEngine/groupparam_wrapper.h \
    NatronEngine/pageparam_wrapper.h \
    NatronEngine/appsettings_wrapper.h \
    NatronEngine/pycoreapplication_wrapper.h \
    NatronEngine/userparamholder_wrapper.h \
    NatronEngine/rectd_wrapper.h \
    NatronEngine/recti_wrapper.h


OTHER_FILES += \
    typesystem_engine.xml

BREAKPAD_PATH = ../google-breakpad/src
INCLUDEPATH += $$BREAKPAD_PATH

# every *nix
unix {
        SOURCES += $$BREAKPAD_PATH/client/minidump_file_writer.cc \
                $$BREAKPAD_PATH/common/string_conversion.cc \
                $$BREAKPAD_PATH/common/convert_UTF.c \
                $$BREAKPAD_PATH/common/md5.cc
}

# mac os x
mac {
        # hack to make minidump_generator.cc compile as it uses
        # esp instead of __esp
        # DEFINES += __DARWIN_UNIX03=0 -- looks like we do not need it anymore

        SOURCES += $$BREAKPAD_PATH/client/mac/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/mac/handler/minidump_generator.cc \
                $$BREAKPAD_PATH/client/mac/handler/dynamic_images.cc \
                $$BREAKPAD_PATH/client/mac/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/common/mac/string_utilities.cc \
                $$BREAKPAD_PATH/common/mac/file_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_id.cc \
                $$BREAKPAD_PATH/common/mac/macho_utilities.cc \
                $$BREAKPAD_PATH/common/mac/macho_walker.cc
        OBJECTIVE_SOURCES += \
                $$BREAKPAD_PATH/common/mac/MachIPC.mm
}

# linux 
linux {
        SOURCES += $$BREAKPAD_PATH/client/linux/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/linux/handler/minidump_descriptor.cc \
                $$BREAKPAD_PATH/client/linux/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/common/linux/guid_creator.cc \
                $$BREAKPAD_PATH/common/linux/file_id.cc
}

win32 {
        SOURCES += $$BREAKPAD_PATH/client/windows/handler/exception_handler.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/crash_generation_client.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/client_info.cc \
                $$BREAKPAD_PATH/client/windows/crash_generation/minidump_generator.cc \
                $$BREAKPAD_PATH/common/windows/guid_string.cc
}

# GENERATED_SOURCES =				\
# NatronEngine/animatedparam_wrapper.cpp		\
# NatronEngine/app_wrapper.cpp			\
# NatronEngine/beziercurve_wrapper.cpp		\
# NatronEngine/booleanparam_wrapper.cpp		\
# NatronEngine/buttonparam_wrapper.cpp		\
# NatronEngine/choiceparam_wrapper.cpp		\
# NatronEngine/colorparam_wrapper.cpp		\
# NatronEngine/colortuple_wrapper.cpp		\
# NatronEngine/double2dparam_wrapper.cpp		\
# NatronEngine/double2dtuple_wrapper.cpp		\
# NatronEngine/double3dparam_wrapper.cpp		\
# NatronEngine/double3dtuple_wrapper.cpp		\
# NatronEngine/doubleparam_wrapper.cpp		\
# NatronEngine/effect_wrapper.cpp			\
# NatronEngine/fileparam_wrapper.cpp		\
# NatronEngine/group_wrapper.cpp			\
# NatronEngine/groupparam_wrapper.cpp		\
# NatronEngine/int2dparam_wrapper.cpp		\
# NatronEngine/int2dtuple_wrapper.cpp		\
# NatronEngine/int3dparam_wrapper.cpp		\
# NatronEngine/int3dtuple_wrapper.cpp		\
# NatronEngine/intparam_wrapper.cpp		\
# NatronEngine/itembase_wrapper.cpp		\
# NatronEngine/layer_wrapper.cpp			\
# NatronEngine/natron_wrapper.cpp			\
# NatronEngine/natronengine_module_wrapper.cpp	\
# NatronEngine/outputfileparam_wrapper.cpp	\
# NatronEngine/pageparam_wrapper.cpp		\
# NatronEngine/param_wrapper.cpp			\
# NatronEngine/parametricparam_wrapper.cpp	\
# NatronEngine/pathparam_wrapper.cpp		\
# NatronEngine/roto_wrapper.cpp			\
# NatronEngine/stringparam_wrapper.cpp		\
# NatronEngine/stringparambase_wrapper.cpp    

# defineReplace(shibokenWorkaround) {
#     SOURCES += $$GENERATED_SOURCES
#     return("%_wrapper.cpp")
# }

# isEmpty(SHIBOKEN) {
#    SHIBOKEN = shiboken
# }

# SHIBOKEN_FILE  = . # Need to give some bogus input
# SHIBOKEN.input = SHIBOKEN_FILE
# SHIBOKEN.output_function = shibokenWorkaround
# SHIBOKEN.commands = $$SHIBOKEN --include-paths=..:$$system(pkg-config --variable=includedir pyside)  --typesystem-paths=$$system(pkg-config --variable=typesystemdir pyside) Pyside_Engine_Python.h typesystem_engine.xml
# SHIBOKEN.CONFIG = no_link # don't add the .cpp target file to OBJECTS
# SHIBOKEN.clean = dummy # don't remove the %_wrapper.cpp file by "make clean"

# QMAKE_EXTRA_COMPILERS += SHIBOKEN

macx {
OBJECTIVE_SOURCES += \
    QUrlFix.mm
}
