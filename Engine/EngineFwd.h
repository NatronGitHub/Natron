/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <list>
#include <vector>


// boost

namespace boost {
template<class T> class weak_ptr;
template<class T> class shared_ptr;
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
typedef boost::shared_ptr<QTimer> QTimerPtr;

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
typedef boost::shared_ptr<PluginCache> PluginCachePtr;
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
typedef boost::shared_ptr<SequenceFromFiles> SequenceFromFilesPtr;
}

// libmv
namespace mv {
class AutoTrack;
typedef boost::shared_ptr<AutoTrack> AutoTrackPtr;
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
class QuitInstanceArgs;
class RectD;
class RectI;
class RenderEngine;
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
typedef boost::shared_ptr<Matrix3x3> Matrix3x3Ptr;
}

#ifdef __NATRON_WIN32__
struct OSGLContext_wgl_data;
#endif
#ifdef __NATRON_LINUX__
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


typedef boost::shared_ptr<AbortableRenderInfo> AbortableRenderInfoPtr;
typedef boost::shared_ptr<AbstractOfxEffectInstance> AbstractOfxEffectInstancePtr;
typedef boost::shared_ptr<ActionsCache> ActionsCachePtr;
typedef boost::shared_ptr<AppInstance> AppInstancePtr;
typedef boost::shared_ptr<Bezier> BezierPtr;
typedef boost::shared_ptr<BezierCP> BezierCPPtr;
typedef boost::shared_ptr<BezierSerialization> BezierSerializationPtr;
typedef boost::shared_ptr<BufferableObject> BufferableObjectPtr;
typedef boost::shared_ptr<CacheSignalEmitter> CacheSignalEmitterPtr;
typedef boost::shared_ptr<Curve> CurvePtr;
typedef boost::shared_ptr<EffectInstance> EffectInstancePtr;
typedef boost::shared_ptr<FileSystemItem> FileSystemItemPtr;
typedef boost::shared_ptr<FileSystemModel> FileSystemModelPtr;
typedef boost::shared_ptr<FrameEntry> FrameEntryPtr;
typedef boost::shared_ptr<FrameParams> FrameParamsPtr;
typedef boost::shared_ptr<GLShader> GLShaderPtr;
typedef boost::shared_ptr<GenericAccess> GenericAccessPtr;
typedef boost::shared_ptr<GenericThreadExecOnMainThreadArgs> GenericThreadExecOnMainThreadArgsPtr;
typedef boost::shared_ptr<GenericThreadStartArgs> GenericThreadStartArgsPtr;
typedef boost::shared_ptr<GenericWatcherCallerArgs> GenericWatcherCallerArgsPtr;
typedef boost::shared_ptr<GenericWatcherCallerArgs> WatcherCallerArgsPtr;
typedef boost::shared_ptr<GroupKnobSerialization> GroupKnobSerializationPtr;
typedef boost::shared_ptr<HostOverlayKnobs> HostOverlayKnobsPtr;
typedef boost::shared_ptr<HostOverlayKnobsCornerPin> HostOverlayKnobsCornerPinPtr;
typedef boost::shared_ptr<HostOverlayKnobsPosition> HostOverlayKnobsPositionPtr;
typedef boost::shared_ptr<HostOverlayKnobsTransform> HostOverlayKnobsTransformPtr;
typedef boost::shared_ptr<Image> ImagePtr;
typedef boost::shared_ptr<ImageParams> ImageParamsPtr;
typedef boost::shared_ptr<ImagePlaneDesc> ImagePlaneDescPtr;
typedef boost::shared_ptr<KnobBool> KnobBoolPtr;
typedef boost::shared_ptr<KnobButton> KnobButtonPtr;
typedef boost::shared_ptr<KnobChoice> KnobChoicePtr;
typedef boost::shared_ptr<KnobColor> KnobColorPtr;
typedef boost::shared_ptr<KnobDouble> KnobDoublePtr;
typedef boost::shared_ptr<KnobFactory> KnobFactoryPtr;
typedef boost::shared_ptr<KnobFile> KnobFilePtr;
typedef boost::shared_ptr<KnobGroup> KnobGroupPtr;
typedef boost::shared_ptr<KnobGuiI> KnobGuiIPtr;
typedef boost::shared_ptr<KnobHelper> KnobHelperPtr;
typedef boost::shared_ptr<KnobHolder> KnobHolderPtr;
typedef boost::shared_ptr<KnobI> KnobIPtr;
typedef boost::shared_ptr<KnobInt> KnobIntPtr;
typedef boost::shared_ptr<KnobLayers> KnobLayersPtr;
typedef boost::shared_ptr<KnobOutputFile> KnobOutputFilePtr;
typedef boost::shared_ptr<KnobPage> KnobPagePtr;
typedef boost::shared_ptr<KnobParametric> KnobParametricPtr;
typedef boost::shared_ptr<KnobPath> KnobPathPtr;
typedef boost::shared_ptr<KnobSeparator> KnobSeparatorPtr;
typedef boost::shared_ptr<KnobSerialization> KnobSerializationPtr;
typedef boost::shared_ptr<KnobSerializationBase> KnobSerializationBasePtr;
typedef boost::shared_ptr<KnobSerializationBase> KnobsSerializationBasePtr;
typedef boost::shared_ptr<KnobSignalSlotHandler> KnobSignalSlotHandlerPtr;
typedef boost::shared_ptr<KnobString> KnobStringPtr;
typedef boost::shared_ptr<KnobTLSData> KnobTLSDataPtr;
typedef boost::shared_ptr<KnobTable> KnobTablePtr;
typedef boost::shared_ptr<MemoryFile> MemoryFilePtr;
typedef boost::shared_ptr<Node> NodePtr;
typedef boost::shared_ptr<NodeCollection> NodeCollectionPtr;
typedef boost::shared_ptr<NodeFrameRequest> NodeFrameRequestPtr;
typedef boost::shared_ptr<NodeGroup> NodeGroupPtr;
typedef boost::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef boost::shared_ptr<NodeRenderWatcher> NodeRenderWatcherPtr;
typedef boost::shared_ptr<NodeSerialization> NodeSerializationPtr;
typedef boost::shared_ptr<OSGLContext> OSGLContextPtr;
typedef boost::shared_ptr<OSGLContextAttacher> OSGLContextAttacherPtr;
typedef boost::shared_ptr<OfxEffectInstance> OfxEffectInstancePtr;
typedef boost::shared_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractPtr;
typedef boost::shared_ptr<OutputEffectInstance> OutputEffectInstancePtr;
typedef boost::shared_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsPtr;
typedef boost::shared_ptr<ParallelRenderArgs> ParallelRenderArgsPtr;
typedef boost::shared_ptr<ParallelRenderArgsSetter> ParallelRenderArgsSetterPtr;
typedef boost::shared_ptr<PluginGroupNode> PluginGroupNodePtr;
typedef boost::shared_ptr<PluginMemory> PluginMemoryPtr;
typedef boost::shared_ptr<PrecompNode> PrecompNodePtr;
typedef boost::shared_ptr<ProcessHandler> ProcessHandlerPtr;
typedef boost::shared_ptr<Project> ProjectPtr;
typedef boost::shared_ptr<QuitInstanceArgs> QuitInstanceArgsPtr;
typedef boost::shared_ptr<RenderEngine> RenderEnginePtr;
typedef boost::shared_ptr<RenderStats> RenderStatsPtr;
typedef boost::shared_ptr<RenderingFlagSetter> RenderingFlagSetterPtr;
typedef boost::shared_ptr<RotoContext> RotoContextPtr;
typedef boost::shared_ptr<RotoDrawableItem> RotoDrawableItemPtr;
typedef boost::shared_ptr<RotoItem const> RotoItemConstPtr;
typedef boost::shared_ptr<RotoItem> RotoItemPtr;
typedef boost::shared_ptr<RotoItemSerialization> RotoItemSerializationPtr;
typedef boost::shared_ptr<RotoLayer> RotoLayerPtr;
typedef boost::shared_ptr<RotoLayerSerialization> RotoLayerSerializationPtr;
typedef boost::shared_ptr<RotoPaintInteract> RotoPaintInteractPtr;
typedef boost::shared_ptr<RotoStrokeItem> RotoStrokeItemPtr;
typedef boost::shared_ptr<RotoStrokeItemSerialization> RotoStrokeItemSerializationPtr;
typedef boost::shared_ptr<Settings> SettingsPtr;
typedef boost::shared_ptr<TLSHolderBase const> TLSHolderBaseConstPtr;
typedef boost::shared_ptr<Texture> GLTexturePtr;
typedef boost::shared_ptr<Texture> TexturePtr;
typedef boost::shared_ptr<TileCacheFile> TileCacheFilePtr;
typedef boost::shared_ptr<TimeLapse> TimeLapsePtr;
typedef boost::shared_ptr<TimeLine> TimeLinePtr;
typedef boost::shared_ptr<TrackArgs> TrackArgsPtr;
typedef boost::shared_ptr<TrackMarker> TrackMarkerPtr;
typedef boost::shared_ptr<TrackMarkerAndOptions> TrackMarkerAndOptionsPtr;
typedef boost::shared_ptr<TrackerContext> TrackerContextPtr;
typedef boost::shared_ptr<TrackerFrameAccessor> TrackerFrameAccessorPtr;
typedef boost::shared_ptr<TrackerNode> TrackerNodePtr;
typedef boost::shared_ptr<TrackerNodeInteract> TrackerNodeInteractPtr;
typedef boost::shared_ptr<UndoCommand> UndoCommandPtr;
typedef boost::shared_ptr<UpdateViewerParams> UpdateViewerParamsPtr;
typedef boost::shared_ptr<ViewerArgs> ViewerArgsPtr;
typedef boost::shared_ptr<ViewerCurrentFrameRequestSchedulerStartArgs> ViewerCurrentFrameRequestSchedulerStartArgsPtr;
typedef boost::shared_ptr<ViewerInstance> ViewerInstancePtr;
typedef boost::shared_ptr<ViewerParallelRenderArgsSetter> ViewerParallelRenderArgsSetterPtr;
typedef boost::weak_ptr<AbortableRenderInfo> AbortableRenderInfoWPtr;
typedef boost::weak_ptr<AppInstance> AppInstanceWPtr;
typedef boost::weak_ptr<Bezier> BezierWPtr;
typedef boost::weak_ptr<Curve> CurveWPtr;
typedef boost::weak_ptr<EffectInstance> EffectInstanceWPtr;
typedef boost::weak_ptr<FileSystemItem> FileSystemItemWPtr;
typedef boost::weak_ptr<FileSystemModel> FileSystemModelWPtr;
typedef boost::weak_ptr<Image> ImageWPtr;
typedef boost::weak_ptr<KnobBool> KnobBoolWPtr;
typedef boost::weak_ptr<KnobButton> KnobButtonWPtr;
typedef boost::weak_ptr<KnobChoice> KnobChoiceWPtr;
typedef boost::weak_ptr<KnobColor> KnobColorWPtr;
typedef boost::weak_ptr<KnobDouble> KnobDoubleWPtr;
typedef boost::weak_ptr<KnobFile> KnobFileWPtr;
typedef boost::weak_ptr<KnobGroup> KnobGroupWPtr;
typedef boost::weak_ptr<KnobGuiI> KnobGuiIWPtr;
typedef boost::weak_ptr<KnobHolder> KnobHolderWPtr;
typedef boost::weak_ptr<KnobI const> KnobIConstWPtr;
typedef boost::weak_ptr<KnobI> KnobIWPtr;
typedef boost::weak_ptr<KnobInt> KnobIntWPtr;
typedef boost::weak_ptr<KnobLayers> KnobLayersWPtr;
typedef boost::weak_ptr<KnobOutputFile> KnobOutputFileWPtr;
typedef boost::weak_ptr<KnobPage> KnobPageWPtr;
typedef boost::weak_ptr<KnobParametric> KnobParametricWPtr;
typedef boost::weak_ptr<KnobPath> KnobPathWPtr;
typedef boost::weak_ptr<KnobSeparator> KnobSeparatorWPtr;
typedef boost::weak_ptr<KnobString> KnobStringWPtr;
typedef boost::weak_ptr<KnobTable> KnobTableWPtr;
typedef boost::weak_ptr<Node> NodeWPtr;
typedef boost::weak_ptr<NodeCollection> NodeCollectionWPtr;
typedef boost::weak_ptr<NodeGuiI> NodeGuiIWPtr;
typedef boost::weak_ptr<OSGLContext> OSGLContextWPtr;
typedef boost::weak_ptr<OSGLContextAttacher> OSGLContextAttacherWPtr;
typedef boost::weak_ptr<OfxEffectInstance> OfxEffectInstanceWPtr;
typedef boost::weak_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractWPtr;
typedef boost::weak_ptr<OutputEffectInstance> OutputEffectInstanceWPtr;
typedef boost::weak_ptr<OutputSchedulerThreadStartArgs> OutputSchedulerThreadStartArgsWPtr;
typedef boost::weak_ptr<Plugin> PluginWPtr;
typedef boost::weak_ptr<PluginGroupNode> PluginGroupNodeWPtr;
typedef boost::weak_ptr<PluginMemory> PluginMemoryWPtr;
typedef boost::weak_ptr<PrecompNode> PrecompNodeWPtr;
typedef boost::weak_ptr<Project> ProjectWPtr;
typedef boost::weak_ptr<RenderEngine> RenderEngineWPtr;
typedef boost::weak_ptr<RotoContext> RotoContextWPtr;
typedef boost::weak_ptr<RotoDrawableItem> RotoDrawableItemWPtr;
typedef boost::weak_ptr<RotoLayer> RotoLayerWPtr;
typedef boost::weak_ptr<RotoPaint> RotoPaintWPtr;
typedef boost::weak_ptr<RotoPaintInteract> RotoPaintInteractWPtr;
typedef boost::weak_ptr<RotoStrokeItem> RotoStrokeItemWPtr;
typedef boost::weak_ptr<TLSHolderBase const> TLSHolderBaseConstWPtr;
typedef boost::weak_ptr<TileCacheFile> TileCacheFileWPtr;
typedef boost::weak_ptr<TimeLine> TimeLineWPtr;
typedef boost::weak_ptr<TrackMarker> TrackMarkerWPtr;
typedef boost::weak_ptr<TrackerContext> TrackerContextWPtr;
typedef boost::weak_ptr<ViewerInstance> ViewerInstanceWPtr;
typedef std::list<ImagePtr> ImageList;
typedef std::list<NodePtr> NodesList;
typedef std::list<NodeWPtr> NodesWList;
typedef std::vector<KnobIPtr> KnobsVec;

NATRON_NAMESPACE_EXIT

#endif // Engine_EngineFwd_h
