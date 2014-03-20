#This Source Code Form is subject to the terms of the Mozilla Public
#License, v. 2.0. If a copy of the MPL was not distributed with this
#file, You can obtain one at http://mozilla.org/MPL/2.0/.


TARGET = Engine
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc
CONFIG += boost qt expat
QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent
QT -= gui


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
    AppInstance.cpp \
    AppManager.cpp \
    BlockingBackgroundRender.cpp \
    ChannelSet.cpp \
    Curve.cpp \
    CurveSerialization.cpp \
    EffectInstance.cpp \
    FrameEntry.cpp \
    FrameParamsSerialization.cpp \
    Hash64.cpp \
    HistogramCPU.cpp \
    Image.cpp \
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
    NonKeyParams.cpp \
    NonKeyParamsSerialization.cpp \
    NodeSerialization.cpp \
    OfxClipInstance.cpp \
    OfxHost.cpp \
    OfxImageEffectInstance.cpp \
    OfxEffectInstance.cpp \
    OfxMemory.cpp \
    OfxOverlayInteract.cpp \
    OfxParamInstance.cpp \
    Plugin.cpp \
    PluginMemory.cpp \
    ProcessHandler.cpp \
    Project.cpp \
    ProjectPrivate.cpp \
    ProjectSerialization.cpp \
    SequenceParsing.cpp \
    Settings.cpp \
    StandardPaths.cpp \
    StringAnimationManager.cpp \
    TimeLine.cpp \
    Timer.cpp \
    VideoEngine.cpp \
    ViewerInstance.cpp

HEADERS += \
    AppInstance.h \
    AppManager.h \
    BlockingBackgroundRender.h \
    Cache.h \
    CacheEntry.h \
    Curve.h \
    CurveSerialization.h \
    CurvePrivate.h \
    ChannelSet.h \
    EffectInstance.h \
    Format.h \
    FrameEntry.h \
    FrameEntrySerialization.h \
    FrameParams.h \
    FrameParamsSerialization.h \
    Hash64.h \
    HistogramCPU.h \
    ImageInfo.h \
    Image.h \
    ImageSerialization.h \
    ImageParams.h \
    ImageParamsSerialization.h \
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
    NonKeyParams.h \
    NonKeyParamsSerialization.h \
    NodeSerialization.h \
    OfxClipInstance.h \
    OfxHost.h \
    OfxImageEffectInstance.h \
    OfxEffectInstance.h \
    OfxOverlayInteract.h \
    OfxMemory.h \
    OfxParamInstance.h \
    OpenGLViewerI.h \
    OverlaySupport.h \
    Plugin.h \
    PluginMemory.h \
    ProcessHandler.h \
    Project.h \
    ProjectPrivate.h \
    ProjectSerialization.h \
    Rect.h \
    SequenceParsing.h \
    Settings.h \
    Singleton.h \
    StandardPaths.h \
    StringAnimationManager.h \
    TextureRect.h \
    TextureRectSerialization.h \
    ThreadStorage.h \
    TimeLine.h \
    Timer.h \
    Variant.h \
    VideoEngine.h \
    ViewerInstance.h \
    ViewerInstancePrivate.h \
    ../Global/Enums.h \
    ../Global/GitVersion.h \
    ../Global/GLIncludes.h \
    ../Global/GlobalDefines.h \
    ../Global/KeySymbols.h \
    ../Global/Macros.h \
    ../Global/MemoryInfo.h \
    ../Global/QtCompat.h

