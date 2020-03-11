# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <https://natrongithub.github.io/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
# Copyright (C) 2018-2020 The Natron developers
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
# Cairo is still the default renderer for Roto
!enable-osmesa {
   CONFIG += enable-cairo
}
CONFIG += moc

CONFIG += boost qt python shiboken pyside osmesa fontconfig
enable-cairo: CONFIG += cairo
QT += core network
greaterThan(QT_MAJOR_VERSION, 4): QT += concurrent

!noexpat: CONFIG += expat

# Do not uncomment the following: pyside requires QtGui, because PySide/QtCore/pyside_qtcore_python.h includes qtextdocument.h
#QT -= gui

CONFIG += libmv-flags openmvg-flags glad-flags libtess-flags yaml-cpp-flags

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
INCLUDEPATH += $$PWD/../Serialization
INCLUDEPATH += $$PWD/../libs
INCLUDEPATH += $$PWD/../libs/SequenceParsing
INCLUDEPATH += $$PWD/../libs/exprtk

# hoedown
INCLUDEPATH += $$PWD/../libs/hoedown/src

#To overcome wrongly generated #include <...> by shiboken
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/NatronEngine
DEPENDPATH += $$PWD/../Global

enable-planartracker {

    DEFINES += ROTOPAINT_ENABLE_PLANARTRACKER
    SOURCES += \
    ConvexHull.cpp \
    PlanarTrackerInteract.cpp \
    RotoPaintPlanarTrackerImpl.cpp

    HEADERS += \
    ConvexHull.h \
    PlanarTrackerInteract.h

}

SOURCES += \
    AddPlaneNode.cpp \
    AnimatingObjectI.cpp \
    AppInstance.cpp \
    AppManager.cpp \
    AppManagerPrivate.cpp \
    Backdrop.cpp \
    Bezier.cpp \
    BezierCP.cpp \
    CLArgs.cpp \
    Cache.cpp \
    CacheEntryBase.cpp \
    CacheEntryKeyBase.cpp \
    ColorParser.cpp \
    CoonsRegularization.cpp \
    CornerPinOverlayInteract.cpp \
    CreateNodeArgs.cpp \
    CurrentFrameRequestScheduler.cpp \
    Curve.cpp \
    DefaultRenderScheduler.cpp \
    DiskCacheNode.cpp \
    Distortion2D.cpp \
    Dot.cpp \
    EffectDescription.cpp \
    EffectInstance.cpp \
    EffectInstanceActionResults.cpp \
    EffectInstanceActions.cpp \
    EffectInstanceChannelSelectors.cpp \
    EffectInstanceDefaultKnobs.cpp \
    EffectInstancePrivate.cpp \
    EffectInstanceRenderRoI.cpp \
    EffectInstanceTLSData.cpp \
    EffectOpenGLContextData.cpp \
    ExistenceCheckThread.cpp \
    ExprTk.cpp \
    FileDownloader.cpp \
    FileSystemModel.cpp \
    FitCurve.cpp \
    Format.cpp \
    FrameViewRequest.cpp \
    GPUContextPool.cpp \
    GenericSchedulerThread.cpp \
    GenericSchedulerThreadWatcher.cpp \
    GroupInput.cpp \
    GroupOutput.cpp \
    Hash64.cpp \
    HashableObject.cpp \
    HistogramCPU.cpp \
    IPCCommon.cpp \
    Image.cpp \
    ImageApplyShader.cpp \
    ImageCacheEntry.cpp \
    ImageCacheKey.cpp \
    ImageConvert.cpp \
    ImageCopyChannels.cpp \
    ImageFill.cpp \
    ImageMaskMix.cpp \
    ImagePlaneDesc.cpp  \
    ImagePrivate.cpp \
    ImageStorage.cpp \
    ImageTilesState.cpp \
    InputDescription.cpp \
    Interpolation.cpp \
    JoinViewsNode.cpp \
    KeyFrameInterpolator.cpp \
    KeybindShortcut.cpp \
    Knob.cpp \
    KnobAnimation.cpp \
    KnobExpression.cpp \
    KnobFactory.cpp \
    KnobFile.cpp \
    KnobItemsTable.cpp \
    KnobItemsTableUndoCommand.cpp \
    KnobTypes.cpp \
    KnobUndoCommand.cpp \
    LibraryBinary.cpp \
    LoadKnobsCompat.cpp \
    Log.cpp \
    Lut.cpp \
    Markdown.cpp \
    MemoryFile.cpp \
    MemoryInfo.cpp \
    MultiThread.cpp \
    NoOpBase.cpp \
    Node.cpp \
    NodeDocumentation.cpp \
    NodeGroup.cpp \
    NodeInputs.cpp \
    NodeMain.cpp \
    NodeMetadata.cpp \
    NodeName.cpp \
    NodeOverlay.cpp \
    NodePrivate.cpp \
    NodePythonInteraction.cpp \
    NodeRenderPreview.cpp \
    Noise.cpp \
    OSGLContext.cpp \
    OSGLContext_mac.cpp \
    OSGLContext_osmesa.cpp \
    OSGLContext_win.cpp \
    OSGLContext_x11.cpp \
    OSGLFunctions_gl.cpp \
    OSGLFunctions_mesa.cpp \
    OfxClipInstance.cpp \
    OfxEffectInstance.cpp \
    OfxHost.cpp \
    OfxImageEffectInstance.cpp \
    OfxMemory.cpp \
    OfxOverlayInteract.cpp \
    OfxParamInstance.cpp \
    OneViewNode.cpp \
    OutputSchedulerThread.cpp \
    OverlayInteractBase.cpp \
    Plugin.cpp \
    PluginMemory.cpp \
    PointOverlayInteract.cpp \
    PrecompNode.cpp \
    ProcessFrameThread.cpp \
    ProcessHandler.cpp \
    Project.cpp \
    ProjectPrivate.cpp \
    PropertiesHolder.cpp \
    PyAppInstance.cpp \
    PyExprUtils.cpp \
    PyItemsTable.cpp \
    PyNode.cpp \
    PyNodeGroup.cpp \
    PyOverlayInteract.cpp \
    PyPanelI.cpp \
    PyParameter.cpp \
    PyRoto.cpp \
    PySideCompat.cpp \
    PyTracker.cpp \
    QtEnumConvert.cpp \
    ReadNode.cpp \
    RectD.cpp \
    RectI.cpp \
    RemovePlaneNode.cpp \
    RenderEngine.cpp \
    RenderQueue.cpp \
    RenderStats.cpp \
    RotoBezierTriangulation.cpp \
    RotoDrawableItem.cpp \
    RotoItem.cpp \
    RotoLayer.cpp \
    RotoPaint.cpp \
    RotoPaintInteract.cpp \
    RotoPaintPrivate.cpp \
    RotoShapeRenderCairo.cpp \
    RotoShapeRenderGL.cpp \
    RotoShapeRenderNode.cpp \
    RotoShapeRenderNodePrivate.cpp \
    RotoStrokeItem.cpp \
    RotoUndoCommand.cpp \
    ScriptObject.cpp \
    SerializableWindow.cpp \
    Settings.cpp \
    Smooth1D.cpp \
    SplitterI.cpp \
    StandardPaths.cpp \
    StorageDeleterThread.cpp \
    StubNode.cpp \
    TLSHolder.cpp \
    TabWidgetI.cpp \
    Texture.cpp \
    ThreadPool.cpp \
    TimeLine.cpp \
    Timer.cpp \
    TrackArgs.cpp \
    TrackMarker.cpp \
    TrackScheduler.cpp \
    TrackerDetect.cpp \
    TrackerFrameAccessor.cpp \
    TrackerHelper.cpp \
    TrackerHelperPrivate.cpp \
    TrackerNode.cpp \
    TrackerNodePrivate.cpp \
    TrackerNodeTransformExport.cpp \
    Transform.cpp \
    TransformOverlayInteract.cpp \
    TreeRender.cpp \
    TreeRenderQueueManager.cpp \
    TreeRenderQueueProvider.cpp \
    Utils.cpp \
    ViewIdx.cpp \
    ViewerDisplayScheduler.cpp \
    ViewerInstance.cpp \
    ViewerNode.cpp \
    ViewerNodeKnobs.cpp \
    ViewerNodeOverlays.cpp \
    ViewerNodePrivate.cpp \
    WriteNode.cpp \
    ../Global/glad_source.c \
    ../Global/FStreamsSupport.cpp \
    ../Global/ProcInfo.cpp \
    ../Global/PythonUtils.cpp \
    ../Global/StrUtils.cpp \
    ../libs/SequenceParsing/SequenceParsing.cpp \
    NatronEngine/animatedparam_wrapper.cpp \
    NatronEngine/app_wrapper.cpp \
    NatronEngine/appsettings_wrapper.cpp \
    NatronEngine/beziercurve_wrapper.cpp \
    NatronEngine/booleanparam_wrapper.cpp \
    NatronEngine/boolnodecreationproperty_wrapper.cpp \
    NatronEngine/buttonparam_wrapper.cpp \
    NatronEngine/choiceparam_wrapper.cpp \
    NatronEngine/colorparam_wrapper.cpp \
    NatronEngine/colortuple_wrapper.cpp \
    NatronEngine/double2dparam_wrapper.cpp \
    NatronEngine/double2dtuple_wrapper.cpp \
    NatronEngine/double3dparam_wrapper.cpp \
    NatronEngine/double3dtuple_wrapper.cpp \
    NatronEngine/doubleparam_wrapper.cpp \
    NatronEngine/effect_wrapper.cpp \
    NatronEngine/exprutils_wrapper.cpp \
    NatronEngine/fileparam_wrapper.cpp \
    NatronEngine/floatnodecreationproperty_wrapper.cpp \
    NatronEngine/group_wrapper.cpp \
    NatronEngine/groupparam_wrapper.cpp \
    NatronEngine/imagelayer_wrapper.cpp \
    NatronEngine/int2dparam_wrapper.cpp \
    NatronEngine/int2dtuple_wrapper.cpp \
    NatronEngine/int3dparam_wrapper.cpp \
    NatronEngine/int3dtuple_wrapper.cpp \
    NatronEngine/intnodecreationproperty_wrapper.cpp \
    NatronEngine/intparam_wrapper.cpp \
    NatronEngine/itembase_wrapper.cpp \
    NatronEngine/itemstable_wrapper.cpp \
    NatronEngine/natron_namespace_wrapper.cpp \
    NatronEngine/natronengine_module_wrapper.cpp \
    NatronEngine/nodecreationproperty_wrapper.cpp \
    NatronEngine/pageparam_wrapper.cpp \
    NatronEngine/param_wrapper.cpp \
    NatronEngine/parametricparam_wrapper.cpp \
    NatronEngine/pathparam_wrapper.cpp \
    NatronEngine/pycoreapplication_wrapper.cpp \
    NatronEngine/pycornerpinoverlayinteract_wrapper.cpp \
    NatronEngine/pyoverlayinteract_wrapper.cpp \
    NatronEngine/pyoverlayparamdesc_wrapper.cpp \
    NatronEngine/pypointoverlayinteract_wrapper.cpp \
    NatronEngine/pytransformoverlayinteract_wrapper.cpp \
    NatronEngine/rectd_wrapper.cpp \
    NatronEngine/recti_wrapper.cpp \
    NatronEngine/roto_wrapper.cpp \
    NatronEngine/separatorparam_wrapper.cpp \
    NatronEngine/stringnodecreationproperty_wrapper.cpp \
    NatronEngine/stringparam_wrapper.cpp \
    NatronEngine/stringparambase_wrapper.cpp \
    NatronEngine/strokeitem_wrapper.cpp \
    NatronEngine/strokepoint_wrapper.cpp \
    NatronEngine/track_wrapper.cpp \
    NatronEngine/tracker_wrapper.cpp \
    NatronEngine/userparamholder_wrapper.cpp \

HEADERS += \
    AddPlaneNode.h \
    AfterQuitProcessingI.h \
    AnimatingObjectI.h \
    AppInstance.h \
    AppManager.h \
    AppManagerPrivate.h \
    Backdrop.h \
    Bezier.h \
    BezierCP.h \
    BezierCPPrivate.h \
    CLArgs.h \
    Cache.h \
    CacheEntryBase.h \
    CacheEntryKeyBase.h \
    ChoiceOption.h \
    Color.h \
    ColorParser.h \
    CoonsRegularization.h \
    CornerPinOverlayInteract.h \
    CreateNodeArgs.h \
    CurrentFrameRequestScheduler.h \
    Curve.h \
    CurvePrivate.h \
    DefaultRenderScheduler.h \
    DimensionIdx.h \
    DiskCacheNode.h \
    Distortion2D.h \
    DockablePanelI.h \
    Dot.h \
    EffectDescription.h \
    EffectInstance.h \
    EffectInstanceActionResults.h \
    EffectInstancePrivate.h \
    EffectInstanceTLSData.h \
    EffectOpenGLContextData.h \
    EngineFwd.h \
    ExistenceCheckThread.h \
    FeatherPoint.h \
    FileDownloader.h \
    FileSystemModel.h \
    FitCurve.h \
    Format.h \
    FrameViewRequest.h \
    GLShader.h \
    GPUContextPool.h \
    GenericSchedulerThread.h \
    GenericSchedulerThreadWatcher.h \
    GroupInput.h \
    GroupOutput.h \
    Hash64.h \
    HashableObject.h \
    HistogramCPU.h \
    IPCCommon.h \
    Image.h \
    ImageCacheEntry.h \
    ImageCacheEntryProcessing.h \
    ImageCacheKey.h \
    ImagePlaneDesc.h \
    ImagePrivate.h \
    ImageStorage.h \
    ImageTilesState.h \
    InputDescription.h \
    Interpolation.h \
    JoinViewsNode.h \
    KeyFrameInterpolator.h \
    KeybindShortcut.h \
    Knob.h \
    KnobFactory.h \
    KnobFile.h \
    KnobGetValueImpl.h \
    KnobGuiI.h \
    KnobImpl.h \
    KnobItemsTable.h \
    KnobItemsTableUndoCommand.h \
    KnobPrivate.h \
    KnobSetValueImpl.h \
    KnobTypes.h \
    KnobUndoCommand.h \
    LibraryBinary.h \
    LoadKnobsCompat.h \
    Log.h \
    LogEntry.h \
    Lut.h \
    Markdown.h \
    MemoryFile.h \
    MemoryInfo.h \
    MergingEnum.h \
    MultiThread.h \
    NoOpBase.h \
    Node.h \
    NodeGraphI.h \
    NodeGroup.h \
    NodeGuiI.h \
    NodeMetadata.h \
    OSGLContext.h \
    OSGLContext_mac.h \
    OSGLContext_osmesa.h \
    OSGLContext_win.h \
    OSGLContext_x11.h \
    OSGLFramebufferConfig.h \
    OSGLFunctions.h \
    OSGLFunctions_mesa.h \
    OfxClipInstance.h \
    OfxEffectInstance.h \
    OfxHost.h \
    OfxImageEffectInstance.h \
    OfxMemory.h \
    OfxOverlayInteract.h \
    OfxParamInstance.h \
    OneViewNode.h \
    OpenGLViewerI.h \
    OutputSchedulerThread.h \
    OverlayInteractBase.h \
    OverlaySupport.h \
    Plugin.h \
    PluginActionShortcut.h \
    PluginMemory.h \
    PointOverlayInteract.h \
    PrecompNode.h \
    ProcessFrameThread.h \
    ProcessHandler.h \
    Project.h \
    ProjectPrivate.h \
    PropertiesHolder.h \
    PyAppInstance.h \
    PyExprUtils.h \
    PyGlobalFunctions.h \
    PyItemsTable.h \
    PyNode.h \
    PyNodeGroup.h \
    PyOverlayInteract.h \
    PyPanelI.h \
    PyParameter.h \
    PyRoto.h \
    PyTracker.h \
    Pyside_Engine_Python.h \
    QtEnumConvert.h \
    RamBuffer.h \
    ReadNode.h \
    RectD.h \
    RectI.h \
    RemovePlaneNode.h \
    RenderEngine.h \
    RenderQueue.h \
    RenderStats.h \
    RotoBezierTriangulation.h \
    RotoDrawableItem.h \
    RotoItem.h \
    RotoLayer.h \
    RotoPaint.h \
    RotoPaintPrivate.h \
    RotoPoint.h \
    RotoShapeRenderCairo.h \
    RotoShapeRenderGL.h \
    RotoShapeRenderNode.h \
    RotoShapeRenderNodePrivate.h \
    RotoStrokeItem.h \
    RotoUndoCommand.h \
    ScriptObject.h \
    SerializableWindow.h \
    Settings.h \
    Singleton.h \
    SplitterI.h \
    StandardPaths.h \
    StorageDeleterThread.h \
    StubNode.h \
    TLSHolder.h \
    TLSHolderImpl.h \
    TabWidgetI.h \
    Texture.h \
    ThreadPool.h \
    ThreadStorage.h \
    TimeLine.h \
    TimeLineKeys.h \
    TimeValue.h \
    Timer.h \
    TrackArgs.h \
    TrackMarker.h \
    TrackScheduler.h \
    TrackerDetect.h \
    TrackerFrameAccessor.h \
    TrackerHelper.h \
    TrackerHelperPrivate.h \
    TrackerNode.h \
    TrackerNodePrivate.h \
    TrackerParamsProvider.h \
    Transform.h \
    TransformOverlayInteract.h \
    TreeRender.h \
    TreeRenderQueueManager.h \
    TreeRenderQueueProvider.h \
    UndoCommand.h \
    Utils.h \
    Variant.h \
    ViewIdx.h \
    ViewerDisplayScheduler.h \
    ViewerInstance.h \
    ViewerNode.h \
    ViewerNodePrivate.h \
    WriteNode.h \
    fstream_mingw.h \
    ../Global/Enums.h \
    ../Global/FStreamsSupport.h \
    ../Global/fstream_mingw.h \
    ../Global/GitVersion.h \
    ../Global/GLIncludes.h \
    ../Global/GLObfuscate.h \
    ../Global/GlobalDefines.h \
    ../Global/KeySymbols.h \
    ../Global/Macros.h \
    ../Global/ProcInfo.h \
    ../Global/PythonUtils.h \
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
    ../libs/exprtk/exprtk.hpp \
    NatronEngine/animatedparam_wrapper.h \
    NatronEngine/app_wrapper.h \
    NatronEngine/appsettings_wrapper.h \
    NatronEngine/beziercurve_wrapper.h \
    NatronEngine/booleanparam_wrapper.h \
    NatronEngine/boolnodecreationproperty_wrapper.h \
    NatronEngine/buttonparam_wrapper.h \
    NatronEngine/choiceparam_wrapper.h \
    NatronEngine/colorparam_wrapper.h \
    NatronEngine/colortuple_wrapper.h \
    NatronEngine/double2dparam_wrapper.h \
    NatronEngine/double2dtuple_wrapper.h \
    NatronEngine/double3dparam_wrapper.h \
    NatronEngine/double3dtuple_wrapper.h \
    NatronEngine/doubleparam_wrapper.h \
    NatronEngine/effect_wrapper.h \
    NatronEngine/exprutils_wrapper.h \
    NatronEngine/fileparam_wrapper.h \
    NatronEngine/floatnodecreationproperty_wrapper.h \
    NatronEngine/group_wrapper.h \
    NatronEngine/groupparam_wrapper.h \
    NatronEngine/imagelayer_wrapper.h \
    NatronEngine/int2dparam_wrapper.h \
    NatronEngine/int2dtuple_wrapper.h \
    NatronEngine/int3dparam_wrapper.h \
    NatronEngine/int3dtuple_wrapper.h \
    NatronEngine/intnodecreationproperty_wrapper.h \
    NatronEngine/intparam_wrapper.h \
    NatronEngine/itembase_wrapper.h \
    NatronEngine/itemstable_wrapper.h \
    NatronEngine/natron_namespace_wrapper.h \
    NatronEngine/natronengine_python.h \
    NatronEngine/nodecreationproperty_wrapper.h \
    NatronEngine/pageparam_wrapper.h \
    NatronEngine/param_wrapper.h \
    NatronEngine/parametricparam_wrapper.h \
    NatronEngine/pathparam_wrapper.h \
    NatronEngine/pycoreapplication_wrapper.h \
    NatronEngine/pycornerpinoverlayinteract_wrapper.h \
    NatronEngine/pyoverlayinteract_wrapper.h \
    NatronEngine/pyoverlayparamdesc_wrapper.h \
    NatronEngine/pypointoverlayinteract_wrapper.h \
    NatronEngine/pytransformoverlayinteract_wrapper.h \
    NatronEngine/rectd_wrapper.h \
    NatronEngine/recti_wrapper.h \
    NatronEngine/roto_wrapper.h \
    NatronEngine/separatorparam_wrapper.h \
    NatronEngine/stringnodecreationproperty_wrapper.h \
    NatronEngine/stringparam_wrapper.h \
    NatronEngine/stringparambase_wrapper.h \
    NatronEngine/strokeitem_wrapper.h \
    NatronEngine/strokepoint_wrapper.h \
    NatronEngine/track_wrapper.h \
    NatronEngine/tracker_wrapper.h \
    NatronEngine/userparamholder_wrapper.h \

OTHER_FILES += \
    typesystem_engine.xml

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
