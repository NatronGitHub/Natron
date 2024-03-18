/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef Engine_EngineFwd_h
#define Engine_EngineFwd_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <memory>
#include <list>
#include <vector>


// boost

namespace boost {
namespace archive {
class xml_iarchive;
class xml_oarchive;
}
namespace serialization {
class access;
}
}


// Qt

class QByteArray;
class QChar;
class QDateTime;
class QFileInfo;
class QLocalServer;
class QLocalSocket;
class QMutex;
class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;
class QProcess;
class QSettings;
class QString;
class QStringList;
class QThread;
class QTimer;
class QUrl;
class QWaitCondition;
typedef std::shared_ptr<QTimer> QTimerPtr;

// cairo
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;


// OpenFX

typedef struct OfxImageMemoryStruct *OfxImageMemoryHandle;
typedef struct OfxImageEffectStruct *OfxImageEffectHandle;
typedef struct OfxImageClipStruct *OfxImageClipHandle;

namespace OFX {
namespace Host {
class Plugin;
namespace Property {
class Set;
}
namespace ImageEffect {
class ImageEffectPlugin;
class Descriptor;
class ClipDescriptor;
class OverlayInteract;
class PluginCache;
typedef std::shared_ptr<PluginCache> PluginCachePtr;
}
namespace Interact {
class Descriptor;
class Instance;
}
}
}

// SequenceParsing
namespace SequenceParsing {
class SequenceFromFiles;
typedef std::shared_ptr<SequenceFromFiles> SequenceFromFilesPtr;
}

// libmv
namespace mv {
class AutoTrack;
typedef std::shared_ptr<AutoTrack> AutoTrackPtr;
}

// Natron Engine

NATRON_NAMESPACE_ENTER
class AbortableRenderInfo;
class AbortableThread;
class AbstractOfxEffectInstance;
class ActionsCache;
class AfterQuitProcessingI;
class AppInstance;
class AppTLS;
class Bezier;
class BezierCP;
class BezierSerialization;
class BlockingBackgroundRender;
class BufferableObject;
class CLArgs;
class CacheEntryHolder;
class CacheSignalEmitter;
class ChoiceExtraData;
class CreateNodeArgs;
class Curve;
class Dimension;
class DockablePanelI;
class EffectInstance;
class ExistenceCheckerThread;
class FileSystemItem;
class FileSystemModel;
class Format;
class FrameEntry;
class FrameKey;
class FrameParams;
class FramebufferConfig;
class GLRendererID;
class GLShader;
class GPUContextPool;
class GenericAccess;
class GenericSchedulerThread;
class GenericSchedulerThreadWatcher;
class GenericThreadExecOnMainThreadArgs;
class GenericThreadStartArgs;
class GenericWatcherCallerArgs;
class GroupKnobSerialization;
class Hash64;
class HostOverlayKnobs;
class HostOverlayKnobsCornerPin;
class HostOverlayKnobsPosition;
class HostOverlayKnobsTransform;
class Image;
class ImageKey;
class ImageParams;
class ImagePlaneDesc;
class KeyFrame;
class KnobBool;
class KnobButton;
class KnobChoice;
class KnobColor;
class KnobDouble;
class KnobFactory;
class KnobFile;
class KnobGroup;
class KnobGuiI;
class KnobHelper;
class KnobHolder;
class KnobI;
class KnobInt;
class KnobLayers;
class KnobOutputFile;
class KnobPage;
class KnobParametric;
class KnobPath;
class KnobSeparator;
class KnobSerialization;
class KnobSerializationBase;
class KnobSignalSlotHandler;
class KnobString;
class KnobTLSData;
class KnobTable;
class LibraryBinary;
class LogEntry;
class MemoryFile;
class Node;
class NodeCollection;
class NodeFrameRequest;
class NodeGraphI;
class NodeGroup;
class NodeGuiI;
class NodeMetadata;
class NodeRenderWatcher;
class NodeSerialization;
class NodeSettingsPanel;
class OSGLContext;
class OSGLContextAttacher;
class OfxClipInstance;
class OfxEffectInstance;
class OfxHost;
class OfxImage;
class OfxImageEffectInstance;
class OfxOverlayInteract;
class OfxParamOverlayInteract;
class OfxParamToKnob;
class OfxStringInstance;
class OpenGLRendererInfo;
class OpenGLViewerI;
class OutputEffectInstance;
class OutputSchedulerThreadStartArgs;
class OverlaySupport;
class ParallelRenderArgs;
class ParallelRenderArgsSetter;
class Plugin;
class PluginGroupNode;
class PluginMemory;
class PrecompNode;
class ProcessHandler;
class ProcessInputChannel;
class Project;
class ProjectBeingLoadedInfo;
class ProjectSerialization;
class RectD;
class RectI;
class RenderEngine;
class RenderScale;
class RenderStats;
class RenderingFlagSetter;
class RotoContext;
class RotoDrawableItem;
class RotoItem;
class RotoItemSerialization;
class RotoLayer;
class RotoLayerSerialization;
class RotoPaint;
class RotoPaintInteract;
class RotoPoint;
class RotoStrokeItem;
class RotoStrokeItemSerialization;
class Settings;
class StringAnimationManager;
class TLSHolderBase;
class Texture;
class TextureRect;
class TileCacheFile;
class TimeLapse;
class TimeLine;
class TrackArgs;
class TrackMarker;
class TrackMarkerAndOptions;
class TrackSerialization;
class TrackerContext;
class TrackerContextSerialization;
class TrackerFrameAccessor;
class TrackerNode;
class TrackerNodeInteract;
class UndoCommand;
class UpdateViewerParams;
class ViewIdx;
class ViewerArgs;
class ViewerCurrentFrameRequestSchedulerStartArgs;
class ViewerInstance;
class ViewerParallelRenderArgsSetter;
namespace Color {
class Lut;
}
namespace Transform {
struct Matrix3x3;
typedef std::shared_ptr<Matrix3x3> Matrix3x3Ptr;
}

#ifdef __NATRON_WIN32__
struct OSGLContext_wgl_data;
#endif
#ifdef __NATRON_LINUX__
class OSGLContext_egl_data;
class OSGLContext_glx_data;
#endif

NATRON_PYTHON_NAMESPACE_ENTER
class App;
class AppSettings;
class BezierCurve;
class BooleanParam;
class ButtonParam;
class ChoiceParam;
class ColorParam;
class Double2DParam;
class Double3DParam;
class DoubleParam;
class Effect;
class FileParam;
class Group;
class GroupParam;
class ItemBase;
class ImageLayer;
class Int2DParam;
class Int3DParam;
class IntParam;
class Layer;
class OutputFileParam;
class PathParam;
class Param;
class PageParam;
class ParametricParam;
class Roto;
class SeparatorParam;
class StringParam;
class Track;
class Tracker;
class UserParamHolder;

NATRON_PYTHON_NAMESPACE_EXIT


typedef std::shared_ptr<AbortableRenderInfo> AbortableRenderInfoPtr;
typedef std::shared_ptr<AbstractOfxEffectInstance> AbstractOfxEffectInstancePtr;
typedef std::shared_ptr<ActionsCache> ActionsCachePtr;
typedef std::shared_ptr<AppInstance> AppInstancePtr;
typedef std::shared_ptr<Bezier> BezierPtr;
typedef std::shared_ptr<BezierCP> BezierCPPtr;
typedef std::shared_ptr<BezierSerialization> BezierSerializationPtr;
typedef std::shared_ptr<BufferableObject> BufferableObjectPtr;
typedef std::shared_ptr<CacheSignalEmitter> CacheSignalEmitterPtr;
typedef std::shared_ptr<Curve> CurvePtr;
typedef std::shared_ptr<EffectInstance> EffectInstancePtr;
typedef std::shared_ptr<ExistenceCheckerThread> ExistenceCheckerThreadPtr;
typedef std::shared_ptr<FileSystemItem> FileSystemItemPtr;
typedef std::shared_ptr<FileSystemModel> FileSystemModelPtr;
typedef std::shared_ptr<FrameEntry> FrameEntryPtr;
typedef std::shared_ptr<FrameParams> FrameParamsPtr;
typedef std::shared_ptr<GLShader> GLShaderPtr;
typedef std::shared_ptr<GenericAccess> GenericAccessPtr;
typedef std::shared_ptr<GenericThreadExecOnMainThreadArgs> GenericThreadExecOnMainThreadArgsPtr;
typedef std::shared_ptr<GenericThreadStartArgs> GenericThreadStartArgsPtr;
typedef std::shared_ptr<GenericWatcherCallerArgs> GenericWatcherCallerArgsPtr;
typedef std::shared_ptr<GroupKnobSerialization> GroupKnobSerializationPtr;
typedef std::shared_ptr<HostOverlayKnobs> HostOverlayKnobsPtr;
typedef std::shared_ptr<HostOverlayKnobsCornerPin> HostOverlayKnobsCornerPinPtr;
typedef std::shared_ptr<HostOverlayKnobsPosition> HostOverlayKnobsPositionPtr;
typedef std::shared_ptr<HostOverlayKnobsTransform> HostOverlayKnobsTransformPtr;
typedef std::shared_ptr<Image> ImagePtr;
typedef std::shared_ptr<Image const> ImageConstPtr;
typedef std::shared_ptr<ImageParams> ImageParamsPtr;
typedef std::shared_ptr<ImagePlaneDesc> ImagePlaneDescPtr;
typedef std::shared_ptr<KnobBool> KnobBoolPtr;
typedef std::shared_ptr<KnobButton> KnobButtonPtr;
typedef std::shared_ptr<KnobChoice> KnobChoicePtr;
typedef std::shared_ptr<KnobColor> KnobColorPtr;
typedef std::shared_ptr<KnobDouble> KnobDoublePtr;
typedef std::shared_ptr<KnobFactory> KnobFactoryPtr;
typedef std::shared_ptr<KnobFile> KnobFilePtr;
typedef std::shared_ptr<KnobGroup> KnobGroupPtr;
typedef std::shared_ptr<KnobGuiI> KnobGuiIPtr;
typedef std::shared_ptr<KnobHelper> KnobHelperPtr;
typedef std::shared_ptr<KnobHolder> KnobHolderPtr;
typedef std::shared_ptr<KnobI> KnobIPtr;
typedef std::shared_ptr<KnobInt> KnobIntPtr;
typedef std::shared_ptr<KnobLayers> KnobLayersPtr;
typedef std::shared_ptr<KnobOutputFile> KnobOutputFilePtr;
typedef std::shared_ptr<KnobPage> KnobPagePtr;
typedef std::shared_ptr<KnobParametric> KnobParametricPtr;
typedef std::shared_ptr<KnobPath> KnobPathPtr;
typedef std::shared_ptr<KnobSeparator> KnobSeparatorPtr;
typedef std::shared_ptr<KnobSerialization> KnobSerializationPtr;
typedef std::shared_ptr<KnobSerializationBase> KnobSerializationBasePtr;
typedef std::shared_ptr<KnobSerializationBase> KnobsSerializationBasePtr;
typedef std::shared_ptr<KnobSignalSlotHandler> KnobSignalSlotHandlerPtr;
typedef std::shared_ptr<KnobString> KnobStringPtr;
typedef std::shared_ptr<KnobTLSData> KnobTLSDataPtr;
typedef std::shared_ptr<KnobTable> KnobTablePtr;
typedef std::shared_ptr<MemoryFile> MemoryFilePtr;
typedef std::shared_ptr<Node> NodePtr;
typedef std::shared_ptr<NodeCollection> NodeCollectionPtr;
typedef std::shared_ptr<NodeFrameRequest> NodeFrameRequestPtr;
typedef std::shared_ptr<NodeGroup> NodeGroupPtr;
typedef std::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef std::shared_ptr<NodeRenderWatcher> NodeRenderWatcherPtr;
typedef std::shared_ptr<NodeSerialization> NodeSerializationPtr;
typedef std::shared_ptr<OSGLContext> OSGLContextPtr;
typedef std::shared_ptr<OSGLContextAttacher> OSGLContextAttacherPtr;
typedef std::shared_ptr<OfxEffectInstance> OfxEffectInstancePtr;
typedef std::shared_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractPtr;
typedef std::shared_ptr<OutputEffectInstance> OutputEffectInstancePtr;
typedef std::shared_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsPtr;
typedef std::shared_ptr<ParallelRenderArgs> ParallelRenderArgsPtr;
typedef std::shared_ptr<ParallelRenderArgsSetter> ParallelRenderArgsSetterPtr;
typedef std::shared_ptr<PluginGroupNode> PluginGroupNodePtr;
typedef std::shared_ptr<PluginMemory> PluginMemoryPtr;
typedef std::shared_ptr<PrecompNode> PrecompNodePtr;
typedef std::shared_ptr<ProcessHandler> ProcessHandlerPtr;
typedef std::shared_ptr<Project> ProjectPtr;
typedef std::shared_ptr<RenderEngine> RenderEnginePtr;
typedef std::shared_ptr<RenderStats> RenderStatsPtr;
typedef std::shared_ptr<RenderingFlagSetter> RenderingFlagSetterPtr;
typedef std::shared_ptr<RotoContext> RotoContextPtr;
typedef std::shared_ptr<RotoDrawableItem> RotoDrawableItemPtr;
typedef std::shared_ptr<RotoItem const> RotoItemConstPtr;
typedef std::shared_ptr<RotoItem> RotoItemPtr;
typedef std::shared_ptr<RotoItemSerialization> RotoItemSerializationPtr;
typedef std::shared_ptr<RotoLayer> RotoLayerPtr;
typedef std::shared_ptr<RotoLayerSerialization> RotoLayerSerializationPtr;
typedef std::shared_ptr<RotoPaintInteract> RotoPaintInteractPtr;
typedef std::shared_ptr<RotoStrokeItem> RotoStrokeItemPtr;
typedef std::shared_ptr<RotoStrokeItemSerialization> RotoStrokeItemSerializationPtr;
typedef std::shared_ptr<Settings> SettingsPtr;
typedef std::shared_ptr<TLSHolderBase const> TLSHolderBaseConstPtr;
typedef std::shared_ptr<Texture> GLTexturePtr;
typedef std::shared_ptr<Texture> TexturePtr;
typedef std::shared_ptr<TileCacheFile> TileCacheFilePtr;
typedef std::shared_ptr<TimeLapse> TimeLapsePtr;
typedef std::shared_ptr<TimeLine> TimeLinePtr;
typedef std::shared_ptr<TrackArgs> TrackArgsPtr;
typedef std::shared_ptr<TrackMarker> TrackMarkerPtr;
typedef std::shared_ptr<TrackMarkerAndOptions> TrackMarkerAndOptionsPtr;
typedef std::shared_ptr<TrackerContext> TrackerContextPtr;
typedef std::shared_ptr<TrackerFrameAccessor> TrackerFrameAccessorPtr;
typedef std::shared_ptr<TrackerNode> TrackerNodePtr;
typedef std::shared_ptr<TrackerNodeInteract> TrackerNodeInteractPtr;
typedef std::shared_ptr<UndoCommand> UndoCommandPtr;
typedef std::shared_ptr<UpdateViewerParams> UpdateViewerParamsPtr;
typedef std::shared_ptr<ViewerArgs> ViewerArgsPtr;
typedef std::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs> ViewerCurrentFrameRequestSchedulerStartArgsPtr;
typedef std::shared_ptr<ViewerInstance> ViewerInstancePtr;
typedef std::shared_ptr<ViewerParallelRenderArgsSetter> ViewerParallelRenderArgsSetterPtr;
typedef std::weak_ptr<AbortableRenderInfo> AbortableRenderInfoWPtr;
typedef std::weak_ptr<AppInstance> AppInstanceWPtr;
typedef std::weak_ptr<Bezier> BezierWPtr;
typedef std::weak_ptr<Curve> CurveWPtr;
typedef std::weak_ptr<EffectInstance> EffectInstanceWPtr;
typedef std::weak_ptr<FileSystemItem> FileSystemItemWPtr;
typedef std::weak_ptr<FileSystemModel> FileSystemModelWPtr;
typedef std::weak_ptr<Image> ImageWPtr;
typedef std::weak_ptr<KnobBool> KnobBoolWPtr;
typedef std::weak_ptr<KnobButton> KnobButtonWPtr;
typedef std::weak_ptr<KnobChoice> KnobChoiceWPtr;
typedef std::weak_ptr<KnobColor> KnobColorWPtr;
typedef std::weak_ptr<KnobDouble> KnobDoubleWPtr;
typedef std::weak_ptr<KnobFile> KnobFileWPtr;
typedef std::weak_ptr<KnobGroup> KnobGroupWPtr;
typedef std::weak_ptr<KnobGuiI> KnobGuiIWPtr;
typedef std::weak_ptr<KnobHolder> KnobHolderWPtr;
typedef std::weak_ptr<KnobI const> KnobIConstWPtr;
typedef std::weak_ptr<KnobI> KnobIWPtr;
typedef std::weak_ptr<KnobInt> KnobIntWPtr;
typedef std::weak_ptr<KnobLayers> KnobLayersWPtr;
typedef std::weak_ptr<KnobOutputFile> KnobOutputFileWPtr;
typedef std::weak_ptr<KnobPage> KnobPageWPtr;
typedef std::weak_ptr<KnobParametric> KnobParametricWPtr;
typedef std::weak_ptr<KnobPath> KnobPathWPtr;
typedef std::weak_ptr<KnobSeparator> KnobSeparatorWPtr;
typedef std::weak_ptr<KnobString> KnobStringWPtr;
typedef std::weak_ptr<KnobTable> KnobTableWPtr;
typedef std::weak_ptr<Node> NodeWPtr;
typedef std::weak_ptr<NodeCollection> NodeCollectionWPtr;
typedef std::weak_ptr<NodeGuiI> NodeGuiIWPtr;
typedef std::weak_ptr<OSGLContext> OSGLContextWPtr;
typedef std::weak_ptr<OSGLContextAttacher> OSGLContextAttacherWPtr;
typedef std::weak_ptr<OfxEffectInstance> OfxEffectInstanceWPtr;
typedef std::weak_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractWPtr;
typedef std::weak_ptr<OutputEffectInstance> OutputEffectInstanceWPtr;
typedef std::weak_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsWPtr;
typedef std::weak_ptr<Plugin> PluginWPtr;
typedef std::weak_ptr<PluginGroupNode> PluginGroupNodeWPtr;
typedef std::weak_ptr<PluginMemory> PluginMemoryWPtr;
typedef std::weak_ptr<PrecompNode> PrecompNodeWPtr;
typedef std::weak_ptr<Project> ProjectWPtr;
typedef std::weak_ptr<RenderEngine> RenderEngineWPtr;
typedef std::weak_ptr<RotoContext> RotoContextWPtr;
typedef std::weak_ptr<RotoDrawableItem> RotoDrawableItemWPtr;
typedef std::weak_ptr<RotoLayer> RotoLayerWPtr;
typedef std::weak_ptr<RotoPaint> RotoPaintWPtr;
typedef std::weak_ptr<RotoPaintInteract> RotoPaintInteractWPtr;
typedef std::weak_ptr<RotoStrokeItem> RotoStrokeItemWPtr;
typedef std::weak_ptr<TLSHolderBase const> TLSHolderBaseConstWPtr;
typedef std::weak_ptr<TileCacheFile> TileCacheFileWPtr;
typedef std::weak_ptr<TimeLine> TimeLineWPtr;
typedef std::weak_ptr<TrackMarker> TrackMarkerWPtr;
typedef std::weak_ptr<TrackerContext> TrackerContextWPtr;
typedef std::weak_ptr<ViewerInstance> ViewerInstanceWPtr;
typedef std::list<ImagePtr> ImageList;
typedef std::list<NodePtr> NodesList;
typedef std::list<NodeWPtr> NodesWList;
typedef std::vector<KnobIPtr> KnobsVec;

NATRON_NAMESPACE_EXIT

#endif // Engine_EngineFwd_h
