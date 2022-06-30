# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# (C) 2018-2022 The Natron developers
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

TARGET = Engine
TEMPLATE = lib
CONFIG += staticlib
CONFIG += moc
CONFIG += boost boost-serialization-lib qt cairo python shiboken pyside 
QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

ENGINE_WRAPPER_DIR = Qt$${QT_MAJOR_VERSION}/NatronEngine

!noexpat: CONFIG += expat

# Do not uncomment the following: pyside requires QtGui, because PySide/QtCore/pyside_qtcore_python.h includes qtextdocument.h
#QT -= gui

CONFIG += libmv-flags openmvg-flags glad-flags libtess-flags

include(../global.pri)

log {
    DEFINES += NATRON_LOG
}

precompile_header {
  #message("Using precompiled header")
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
INCLUDEPATH += $$PWD/../Global
INCLUDEPATH += $$PWD/../libs/SequenceParsing

# hoedown
INCLUDEPATH += $$PWD/../libs/hoedown/src

#To overcome wrongly generated #include <...> by shiboken
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/../Global

SOURCES += \
    AbortableRenderInfo.cpp \
    AppInstance.cpp \
    AppManager.cpp \
    AppManagerPrivate.cpp \
    Backdrop.cpp \
    Bezier.cpp \
    BezierCP.cpp \
    BlockingBackgroundRender.cpp \
    CLArgs.cpp \
    Cache.cpp \
    CoonsRegularization.cpp \
    CreateNodeArgs.cpp \
    Curve.cpp \
    CurveSerialization.cpp \
    DefaultShaders.cpp \
    DiskCacheNode.cpp \
    Dot.cpp \
    EffectInstance.cpp \
    EffectInstancePrivate.cpp \
    EffectInstanceRenderRoI.cpp \
    ExistenceCheckThread.cpp \
    FileDownloader.cpp \
    FileSystemModel.cpp \
    FitCurve.cpp \
    FrameEntry.cpp \
    FrameKey.cpp \
    FrameParamsSerialization.cpp \
    GLShader.cpp \
    GPUContextPool.cpp \
    GenericSchedulerThread.cpp \
    GenericSchedulerThreadWatcher.cpp \
    GroupInput.cpp \
    GroupOutput.cpp \
    Hash64.cpp \
    HistogramCPU.cpp \
    HostOverlaySupport.cpp \
    Image.cpp \
    ImageConvert.cpp \
    ImageCopyChannels.cpp \
    ImageKey.cpp \
    ImageMaskMix.cpp \
    ImageParamsSerialization.cpp \
    ImagePlaneDesc.cpp \
    Interpolation.cpp \
    JoinViewsNode.cpp \
    Knob.cpp \
    KnobFactory.cpp \
    KnobFile.cpp \
    KnobSerialization.cpp \
    KnobTypes.cpp \
    LibraryBinary.cpp \
    Log.cpp \
    Lut.cpp \
    Markdown.cpp \
    MemoryFile.cpp \
    MemoryInfo.cpp \
    NoOpBase.cpp \
    Node.cpp \
    NodeDocumentation.cpp \
    NodeGroup.cpp \
    NodeGroupSerialization.cpp \
    NodeInputs.cpp \
    NodeMain.cpp \
    NodeMetadata.cpp \
    NodeName.cpp \
    NodeOverlay.cpp \
    NodeSerialization.cpp \
    Noise.cpp \
    NonKeyParams.cpp \
    NonKeyParamsSerialization.cpp \
    OSGLContext.cpp \
    OSGLContext_mac.cpp \
    OSGLContext_win.cpp \
    OSGLContext_x11.cpp \
    OfxClipInstance.cpp \
    OfxEffectInstance.cpp \
    OfxHost.cpp \
    OfxImageEffectInstance.cpp \
    OfxMemory.cpp \
    OfxOverlayInteract.cpp \
    OfxParamInstance.cpp \
    OneViewNode.cpp \
    OutputEffectInstance.cpp \
    OutputSchedulerThread.cpp \
    ParallelRenderArgs.cpp \
    Plugin.cpp \
    PluginMemory.cpp \
    PrecompNode.cpp \
    ProcessHandler.cpp \
    Project.cpp \
    ProjectPrivate.cpp \
    ProjectSerialization.cpp \
    PyAppInstance.cpp \
    PyExprUtils.cpp \
    PyNode.cpp \
    PyNodeGroup.cpp \
    PyParameter.cpp \
    PyRoto.cpp \
    PySideCompat.cpp \
    PyTracker.cpp \
    ReadNode.cpp \
    RectD.cpp \
    RectI.cpp \
    RenderStats.cpp \
    RotoContext.cpp \
    RotoDrawableItem.cpp \
    RotoItem.cpp \
    RotoLayer.cpp \
    RotoPaint.cpp \
    RotoPaintInteract.cpp \
    RotoSmear.cpp \
    RotoStrokeItem.cpp \
    RotoUndoCommand.cpp \
    ScriptObject.cpp \
    Settings.cpp \
    Smooth1D.cpp \
    StandardPaths.cpp \
    StringAnimationManager.cpp \
    TLSHolder.cpp \
    Texture.cpp \
    TextureRect.cpp \
    ThreadPool.cpp \
    TimeLine.cpp \
    Timer.cpp \
    TrackMarker.cpp \
    TrackerContext.cpp \
    TrackerContextPrivate.cpp \
    TrackerFrameAccessor.cpp \
    TrackerNode.cpp \
    TrackerNodeInteract.cpp \
    TrackerUndoCommand.cpp \
    Transform.cpp \
    Utils.cpp \
    ViewerInstance.cpp \
    WriteNode.cpp \
    ../Global/glad_source.c \
    ../Global/FStreamsSupport.cpp \
    ../Global/ProcInfo.cpp \
    ../Global/PythonUtils.cpp \
    ../Global/StrUtils.cpp \
    ../libs/SequenceParsing/SequenceParsing.cpp \

HEADERS += \
    AbortableRenderInfo.h \
    AfterQuitProcessingI.h \
    AppInstance.h \
    AppManager.h \
    AppManagerPrivate.h \
    Backdrop.h \
    Bezier.h \
    BezierCP.h \
    BezierCPPrivate.h \
    BezierCPSerialization.h \
    BezierSerialization.h \
    BlockingBackgroundRender.h \
    BufferableObject.h \
    CLArgs.h \
    Cache.h \
    CacheEntry.h \
    CacheEntryHolder.h \
    CacheSerialization.h \
    ChoiceOption.h \
    CoonsRegularization.h \
    CreateNodeArgs.h \
    Curve.h \
    CurvePrivate.h \
    CurveSerialization.h \
    DefaultShaders.h \
    DiskCacheNode.h \
    DockablePanelI.h \
    Dot.h \
    EffectInstance.h \
    EffectInstancePrivate.h \
    EngineFwd.h \
    ExistenceCheckThread.h \
    FeatherPoint.h \
    FileDownloader.h \
    FileSystemModel.h \
    FitCurve.h \
    Format.h \
    FormatSerialization.h \
    FrameEntry.h \
    FrameEntrySerialization.h \
    FrameKey.h \
    FrameParams.h \
    FrameParamsSerialization.h \
    GLShader.h \
    GPUContextPool.h \
    GenericSchedulerThread.h \
    GenericSchedulerThreadWatcher.h \
    GroupInput.h \
    GroupOutput.h \
    Hash64.h \
    HistogramCPU.h \
    HostOverlaySupport.h \
    Image.h \
    ImageKey.h \
    ImageLocker.h \
    ImageParams.h \
    ImageParamsSerialization.h \
    ImagePlaneDesc.h \
    ImageSerialization.h \
    Interpolation.h \
    JoinViewsNode.h \
    KeyHelper.h \
    Knob.h \
    KnobFactory.h \
    KnobFile.h \
    KnobGuiI.h \
    KnobImpl.h \
    KnobSerialization.h \
    KnobTypes.h \
    LRUHashTable.h \
    LibraryBinary.h \
    Log.h \
    LogEntry.h \
    Lut.h \
    Markdown.h \
    MemoryFile.h \
    MemoryInfo.h \
    MergingEnum.h \
    NoOpBase.h \
    Node.h \
    NodeGraphI.h \
    NodeGroup.h \
    NodeGroupSerialization.h \
    NodeGuiI.h \
    NodeMetadata.h \
    NodePrivate.h \
    NodeSerialization.h \
    Noise.h \
    NoiseTables.h \
    NonKeyParams.h \
    NonKeyParamsSerialization.h \
    OSGLContext.h \
    OSGLContext_mac.h \
    OSGLContext_win.h \
    OSGLContext_x11.h \
    OfxClipInstance.h \
    OfxEffectInstance.h \
    OfxHost.h \
    OfxImageEffectInstance.h \
    OfxMemory.h \
    OfxOverlayInteract.h \
    OfxParamInstance.h \
    OneViewNode.h \
    OpenGLViewerI.h \
    OutputEffectInstance.h \
    OutputSchedulerThread.h \
    OverlaySupport.h \
    ParallelRenderArgs.h \
    Plugin.h \
    PluginActionShortcut.h \
    PluginMemory.h \
    PrecompNode.h \
    ProcessHandler.h \
    Project.h \
    ProjectPrivate.h \
    ProjectSerialization.h \
    PyAppInstance.h \
    PyExprUtils.h \
    PyGlobalFunctions.h \
    PyNode.h \
    PyNodeGroup.h \
    PyParameter.h \
    PyRoto.h \
    PyTracker.h \
    ReadNode.h \
    RectD.h \
    RectDSerialization.h \
    RectI.h \
    RectISerialization.h \
    RenderStats.h \
    RotoContext.h \
    RotoContextPrivate.h \
    RotoContextSerialization.h \
    RotoDrawableItem.h \
    RotoDrawableItemSerialization.h \
    RotoItem.h \
    RotoItemSerialization.h \
    RotoLayer.h \
    RotoLayerSerialization.h \
    RotoPaint.h \
    RotoPaintInteract.h \
    RotoPoint.h \
    RotoSmear.h \
    RotoStrokeItem.h \
    RotoStrokeItemSerialization.h \
    RotoUndoCommand.h \
    ScriptObject.h \
    Settings.h \
    Singleton.h \
    Smooth1D.h \
    StandardPaths.h \
    StringAnimationManager.h \
    TLSHolder.h \
    TLSHolderImpl.h \
    Texture.h \
    TextureRect.h \
    TextureRectSerialization.h \
    ThreadPool.h \
    ThreadStorage.h \
    TimeLine.h \
    TimeLineKeyFrames.h \
    Timer.h \
    TrackMarker.h \
    TrackerContext.h \
    TrackerContextPrivate.h \
    TrackerFrameAccessor.h \
    TrackerNode.h \
    TrackerNodeInteract.h \
    TrackerSerialization.h \
    TrackerUndoCommand.h \
    Transform.h \
    UndoCommand.h \
    UpdateViewerParams.h \
    Utils.h \
    Variant.h \
    VariantSerialization.h \
    ViewIdx.h \
    ViewerInstance.h \
    ViewerInstancePrivate.h \
    WriteNode.h \
    fstream_mingw.h \
    ../Global/Enums.h \
    ../Global/FStreamsSupport.h \
    ../Global/GitVersion.h \
    ../Global/GLIncludes.h \
    ../Global/GlobalDefines.h \
    ../Global/KeySymbols.h \
    ../Global/Macros.h \
    ../Global/ProcInfo.h \
    ../Global/QtCompat.h \
    ../Global/StrUtils.h \
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

GENERATED_SOURCES += $${ENGINE_WRAPPER_DIR}/natronengine_module_wrapper.cpp
GENERATED_HEADERS += $${ENGINE_WRAPPER_DIR}/natronengine_python.h

ENGINE_GENERATED_SOURCES = \
    animatedparam_wrapper \
    app_wrapper \
    appsettings_wrapper \
    beziercurve_wrapper \
    booleanparam_wrapper \
    boolnodecreationproperty_wrapper \
    buttonparam_wrapper \
    choiceparam_wrapper \
    colorparam_wrapper \
    colortuple_wrapper \
    double2dparam_wrapper \
    double2dtuple_wrapper \
    double3dparam_wrapper \
    double3dtuple_wrapper \
    doubleparam_wrapper \
    effect_wrapper \
    exprutils_wrapper \
    fileparam_wrapper \
    floatnodecreationproperty_wrapper \
    group_wrapper \
    groupparam_wrapper \
    imagelayer_wrapper \
    int2dparam_wrapper \
    int2dtuple_wrapper \
    int3dparam_wrapper \
    int3dtuple_wrapper \
    intnodecreationproperty_wrapper \
    intparam_wrapper \
    itembase_wrapper \
    layer_wrapper \
    natron_enum_wrapper \
    nodecreationproperty_wrapper \
    outputfileparam_wrapper \
    pageparam_wrapper \
    param_wrapper \
    parametricparam_wrapper \
    pathparam_wrapper \
    pycoreapplication_wrapper \
    rectd_wrapper \
    recti_wrapper \
    roto_wrapper \
    separatorparam_wrapper \
    stringnodecreationproperty_wrapper \
    stringparam_wrapper \
    stringparambase_wrapper \
    track_wrapper \
    tracker_wrapper \
    userparamholder_wrapper \

for(name, ENGINE_GENERATED_SOURCES) {
    GENERATED_SOURCES += $${ENGINE_WRAPPER_DIR}/$${name}.cpp
    GENERATED_HEADERS += $${ENGINE_WRAPPER_DIR}/$${name}.h
}


OTHER_FILES += \
    typesystem_engine.xml

defineReplace(shibokenEngine) {
    SOURCES += $$GENERATED_SOURCES
    HEADERS += $$GENERATED_HEADERS
    return("%_wrapper.cpp")
}

QT_INCLUDEPATH = $$PYTHON_INCLUDEPATH $$PYSIDE_INCLUDEDIR
for(dep, QT) {
    QT_INCLUDEPATH += $$eval(QT.$${dep}.includes)
    QT_INCLUDEPATH += $$absolute_path($$eval(QT.$${dep}.name), $$PYSIDE_INCLUDEDIR)
}

equals(QT_MAJOR_VERSION, 5) {
    PYENGINE_HEADER = PySide2_Engine_Python.h
    POST_SHIBOKEN = bash $$shell_path(../tools/utils/runPostShiboken2.sh)
} else:equals(QT_MAJOR_VERSION, 4) {
    PYENGINE_HEADER = Pyside_Engine_Python.h
    POST_SHIBOKEN = bash $$shell_path(../tools/utils/runPostShiboken.sh)
}

SRC_PATH = $$relative_path($$PWD, $$OUT_PWD)/
DEP_GROUP = $$PYENGINE_HEADER typesystem_engine.xml $$HEADERS
enginesbk.input = $$PYENGINE_HEADER typesystem_engine.xml
enginesbk.depends = $$eval($$list($$join(DEP_GROUP, " "$$SRC_PATH, $$SRC_PATH)))
enginesbk.target = enginesbk
enginesbk.commands = cd $$PWD && $$SHIBOKEN \
    --enable-parent-ctor-heuristic --use-isnull-as-nb_nonzero \
    --avoid-protected-hack --enable-pyside-extensions \
    -I.:..:../Global:../libs/OpenFX/include:$$PYTHON_SITE_PACKAGES/PySide2/include:$$[QT_INSTALL_PREFIX]/include \
    $$join(INCLUDEPATH, ":", "-I") \
    $$join(QT_INCLUDEPATH, ":", "-I") \
    -T$$TYPESYSTEMPATH --output-directory=$$OUT_PWD/Qt$$QT_MAJOR_VERSION \
    $$PYENGINE_HEADER typesystem_engine.xml && \
    $$POST_SHIBOKEN $$OUT_PWD/Qt$$QT_MAJOR_VERSION/NatronEngine natronengine
pyengine.depends = enginesbk
pyengine.target = $$shell_path($$ENGINE_WRAPPER_DIR/%_wrapper.cpp)
pyengine.output_function = shibokenEngine
pyengine.commands = bash -c 'true'

QMAKE_EXTRA_TARGETS += enginesbk pyengine

macx {

OBJECTIVE_SOURCES += \
    QUrlFix.mm
}
