/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Serialization/SerializationFwd.h"


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
class QTextStream;
class QThread;
class QTimer;
class QUrl;
class QWaitCondition;

// cairo
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;
typedef struct _cairo_pattern cairo_pattern_t;


// OpenFX

namespace OFX {
namespace Host {
class Plugin;
namespace Property {
class Set;
}
namespace ImageEffect {
class ImageEffectPlugin;
class Descriptor;
}
}
}

// SequenceParsing
namespace SequenceParsing {
class SequenceFromFiles;
typedef boost::shared_ptr<SequenceFromFiles> SequenceFromFilesPtr;
}

// Natron Engine

NATRON_NAMESPACE_ENTER;
class AbortableRenderInfo;
class AbortableThread;
class AbstractOfxEffectInstance;
class AfterQuitProcessingI;
class AnimatingObjectI;
class AnimatingKnobStringHelper;
class AppInstance;
class AppTLS;
class Backdrop;
class Bezier;
class BezierCP;
class BlockingBackgroundRender;
class BufferableObject;
class CLArgs;
class CacheSignalEmitter;
class CompNodeItem;
class CreateNodeArgs;
class Curve;
class DimIdx;
class DiskCacheNode;
class DockablePanelI;
class Dot;
class EffectInstance;
class EffectOpenGLContextData;
class ExistenceCheckerThread;
class FileSystemItem;
class FileSystemModel;
class Format;
class FrameEntry;
class FrameKey;
class FrameParams;
class FramebufferConfig;
class GLRendererID;
class GLShaderBase;
class GPUContextPool;
class GenericAccess;
class GenericSchedulerThread;
class GenericSchedulerThreadWatcher;
class GenericWatcherCallerArgs;
class GroupInput;
class GroupOutput;
class Hash64;
class HashableObject;
class HostOverlayKnobs;
class HostOverlayKnobsCornerPin;
class HostOverlayKnobsPosition;
class HostOverlayKnobsTransform;
class Image;
class ImageComponents;
class ImageKey;
class ImageParams;
class JoinViewsNode;
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
class KnobPage;
class KnobParametric;
class KnobPath;
class KnobSeparator;
class KnobString;
class KnobTable;
class KnobTableItem;
class KnobItemsTable;
class LayeredCompNode;
class LibraryBinary;
class LogEntry;
class NamedKnobHolder;
class Node;
class NodeCollection;
class NodeFrameRequest;
class NodeGraphI;
class NodeGroup;
class NodeGuiI;
class NodeMetadata;
class NodeSettingsPanel;
class NoOpBase;
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
class OneViewNode;
class OpenGLRendererInfo;
class OpenGLViewerI;
class OutputEffectInstance;
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
class PyPanelI;
class ReadNode;
class RectD;
class RectI;
class RenderEngine;
class RenderStats;
class RenderValuesCache;
class RotoDrawableItem;
class RotoItem;
class RotoLayer;
class RotoNode;
class RotoPaint;
class RotoPaintInteract;
class RotoPoint;
class RotoShapeRenderNodeOpenGLData;
class RotoStrokeItem;
class SerializableWindow;
class Settings;
class SplitterI;
class StringAnimationManager;
class StubNode;
class TLSHolderBase;
class TabWidgetI;
class Texture;
class TextureRect;
class TimeLine;
class TimeLapse;
class TrackArgs;
class TrackMarker;
class TrackMarkerAndOptions;
class TrackMarkerPM;
class TrackerHelper;
class TrackerFrameAccessor;
class TrackerNode;
class TrackerParamsProvider;
class UndoCommand;
class UpdateViewerParams;
class ViewIdx;
class ViewerCurrentFrameRequestSchedulerStartArgs;
class ViewerInstance;
class ViewerNode;
class WriteNode;

namespace Color {
class Lut;
}
namespace Transform {
struct Matrix3x3;
}

#ifdef __NATRON_WIN32__
struct OSGLContext_wgl_data;
#endif
#ifdef __NATRON_LINUX__
class OSGLContext_glx_data;
#endif

typedef boost::shared_ptr<AbortableRenderInfo> AbortableRenderInfoPtr;
typedef boost::shared_ptr<AbstractOfxEffectInstance> AbstractOfxEffectInstancePtr;
typedef boost::shared_ptr<AnimatingObjectI> AnimatingObjectIPtr;
typedef boost::shared_ptr<AnimatingKnobStringHelper> AnimatingKnobStringHelperPtr;
typedef boost::shared_ptr<AppInstance> AppInstancePtr;
typedef boost::shared_ptr<Backdrop> BackdropPtr;
typedef boost::shared_ptr<Bezier> BezierPtr;
typedef boost::shared_ptr<BezierCP> BezierCPPtr;
typedef boost::shared_ptr<BufferableObject> BufferableObjectPtr;
typedef boost::shared_ptr<CompNodeItem> CompNodeItemPtr;
typedef boost::shared_ptr<Curve> CurvePtr;
typedef boost::shared_ptr<CreateNodeArgs> CreateNodeArgsPtr;
typedef boost::shared_ptr<DiskCacheNode> DiskCacheNodePtr;
typedef boost::shared_ptr<Dot> DotPtr;
typedef boost::shared_ptr<EffectInstance> EffectInstancePtr;
typedef boost::shared_ptr<EffectInstance const> EffectInstanceConstPtr;
typedef boost::shared_ptr<EffectOpenGLContextData> EffectOpenGLContextDataPtr;
typedef boost::shared_ptr<FileSystemItem> FileSystemItemPtr;
typedef boost::shared_ptr<FileSystemModel> FileSystemModelPtr;
typedef boost::shared_ptr<FrameEntry> FrameEntryPtr;
typedef boost::shared_ptr<GenericWatcherCallerArgs> WatcherCallerArgsPtr;
typedef boost::shared_ptr<GLShaderBase> GLShaderBasePtr;
typedef boost::shared_ptr<GroupInput> GroupInputPtr;
typedef boost::shared_ptr<GroupOutput> GroupOutputPtr;
typedef boost::shared_ptr<HashableObject> HashableObjectPtr;
typedef boost::shared_ptr<HostOverlayKnobs> HostOverlayKnobsPtr;
typedef boost::shared_ptr<Image> ImagePtr;
typedef boost::shared_ptr<ImageParams> ImageParamsPtr;
typedef boost::shared_ptr<JoinViewsNode> JoinViewsNodePtr;
typedef boost::shared_ptr<KnobBool> KnobBoolPtr;
typedef boost::shared_ptr<KnobButton> KnobButtonPtr;
typedef boost::shared_ptr<KnobChoice> KnobChoicePtr;
typedef boost::shared_ptr<KnobColor> KnobColorPtr;
typedef boost::shared_ptr<KnobDouble> KnobDoublePtr;
typedef boost::shared_ptr<KnobFile> KnobFilePtr;
typedef boost::shared_ptr<KnobGroup> KnobGroupPtr;
typedef boost::shared_ptr<KnobGroup const> KnobGroupConstPtr;
typedef boost::shared_ptr<KnobGuiI> KnobGuiIPtr;
typedef boost::shared_ptr<KnobHelper> KnobHelperPtr;
typedef boost::shared_ptr<KnobInt> KnobIntPtr;
typedef boost::shared_ptr<KnobHolder> KnobHolderPtr;
typedef boost::shared_ptr<const KnobHolder> KnobHolderConstPtr;
typedef boost::shared_ptr<KnobI> KnobIPtr;
typedef boost::shared_ptr<KnobLayers> KnobLayersPtr;
typedef boost::shared_ptr<KnobI const> KnobIConstPtr;
typedef boost::shared_ptr<KnobPath> KnobPathPtr;
typedef boost::shared_ptr<KnobPage> KnobPagePtr;
typedef boost::shared_ptr<KnobParametric> KnobParametricPtr;
typedef boost::shared_ptr<KnobSeparator> KnobSeparatorPtr;
typedef boost::shared_ptr<KnobString> KnobStringPtr;
typedef boost::shared_ptr<KnobTable> KnobTablePtr;
typedef boost::shared_ptr<KnobItemsTable> KnobItemsTablePtr;
typedef boost::shared_ptr<KnobTableItem> KnobTableItemPtr;
typedef boost::shared_ptr<const KnobTableItem> KnobTableItemConstPtr;
typedef boost::shared_ptr<LayeredCompNode> LayeredCompNodePtr;
typedef boost::shared_ptr<LibraryBinary> LibraryBinaryPtr;
typedef boost::shared_ptr<NamedKnobHolder> NamedKnobHolderPtr;
typedef boost::shared_ptr<NoOpBase> NoOpBasePtr;
typedef boost::shared_ptr<Node> NodePtr;
typedef boost::shared_ptr<Node const> NodeConstPtr;
typedef boost::shared_ptr<NodeCollection> NodeCollectionPtr;
typedef boost::shared_ptr<NodeFrameRequest> NodeFrameRequestPtr;
typedef boost::shared_ptr<NodeGroup> NodeGroupPtr;
typedef boost::shared_ptr<NodeGuiI> NodeGuiIPtr;
typedef boost::shared_ptr<OfxEffectInstance> OfxEffectInstancePtr;
typedef boost::shared_ptr<OfxParamOverlayInteract> OfxParamOverlayInteractPtr;
typedef boost::shared_ptr<OneViewNode> OneViewNodePtr;
typedef boost::shared_ptr<OpenGLViewerI> OpenGLViewerIPtr;
typedef boost::shared_ptr<OSGLContext> OSGLContextPtr;
typedef boost::shared_ptr<OSGLContextAttacher> OSGLContextAttacherPtr;
typedef boost::shared_ptr<OutputEffectInstance> OutputEffectInstancePtr;
typedef boost::shared_ptr<ParallelRenderArgs> ParallelRenderArgsPtr;
typedef boost::shared_ptr<ParallelRenderArgsSetter> ParallelRenderArgsSetterPtr;
typedef boost::shared_ptr<PrecompNode> PrecompNodePtr;
typedef boost::shared_ptr<ProcessHandler> ProcessHandlerPtr;
typedef boost::shared_ptr<Project> ProjectPtr;
typedef boost::shared_ptr<Plugin> PluginPtr;
typedef boost::shared_ptr<PluginGroupNode> PluginGroupNodePtr;
typedef boost::shared_ptr<PluginMemory> PluginMemoryPtr;
typedef boost::shared_ptr<ReadNode> ReadNodePtr;
typedef boost::shared_ptr<RenderEngine> RenderEnginePtr;
typedef boost::shared_ptr<RenderValuesCache> RenderValuesCachePtr;
typedef boost::shared_ptr<RenderStats> RenderStatsPtr;
typedef boost::shared_ptr<RotoDrawableItem> RotoDrawableItemPtr;
typedef boost::shared_ptr<const RotoDrawableItem> RotoDrawableItemConstPtr;
typedef boost::shared_ptr<RotoItem> RotoItemPtr;
typedef boost::shared_ptr<const RotoItem> RotoItemConstPtr;
typedef boost::shared_ptr<RotoLayer> RotoLayerPtr;
typedef boost::shared_ptr<RotoNode> RotoNodePtr;
typedef boost::shared_ptr<RotoPaint> RotoPaintPtr;
typedef boost::shared_ptr<RotoPaintInteract> RotoPaintInteractPtr;
typedef boost::shared_ptr<RotoShapeRenderNodeOpenGLData> RotoShapeRenderNodeOpenGLDataPtr;
typedef boost::shared_ptr<RotoStrokeItem> RotoStrokeItemPtr;
typedef boost::shared_ptr<Settings> SettingsPtr;
typedef boost::shared_ptr<StringAnimationManager> StringAnimationManagerPtr;
typedef boost::shared_ptr<StubNode> StubNodePtr;
typedef boost::shared_ptr<Texture> GLTexturePtr;
typedef boost::shared_ptr<TimeLapse> TimeLapsePtr;
typedef boost::shared_ptr<TimeLine> TimeLinePtr;
typedef boost::shared_ptr<TrackerHelper> TrackerHelperPtr;
typedef boost::shared_ptr<TrackerNode> TrackerNodePtr;
typedef boost::shared_ptr<TrackMarker> TrackMarkerPtr;
typedef boost::shared_ptr<TrackerParamsProvider> TrackerParamsProviderPtr;
typedef boost::shared_ptr<TrackMarkerAndOptions> TrackMarkerAndOptionsPtr;
typedef boost::shared_ptr<TrackMarkerPM> TrackMarkerPMPtr;
typedef boost::shared_ptr<UndoCommand> UndoCommandPtr;
typedef boost::shared_ptr<UpdateViewerParams> UpdateViewerParamsPtr;
typedef boost::shared_ptr<ViewerInstance> ViewerInstancePtr;
typedef boost::shared_ptr<ViewerNode> ViewerNodePtr;
typedef boost::shared_ptr<WriteNode> WriteNodePtr;
typedef boost::weak_ptr<AbortableRenderInfo> AbortableRenderInfoWPtr;
typedef boost::weak_ptr<AnimatingObjectI> AnimatingObjectIWPtr;
typedef boost::weak_ptr<AppInstance> AppInstanceWPtr;
typedef boost::weak_ptr<Bezier> BezierWPtr;
typedef boost::weak_ptr<CompNodeItem> CompNodeItemWPtr;
typedef boost::weak_ptr<Curve> CurveWPtr;
typedef boost::weak_ptr<EffectInstance> EffectInstanceWPtr;
typedef boost::weak_ptr<HashableObject> HashableObjectWPtr;
typedef boost::weak_ptr<OSGLContext> OSGLContextWPtr;
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
typedef boost::weak_ptr<KnobI> KnobIWPtr;
typedef boost::weak_ptr<KnobI const> KnobIConstWPtr;
typedef boost::weak_ptr<KnobInt> KnobIntWPtr;
typedef boost::weak_ptr<KnobPage> KnobPageWPtr;
typedef boost::weak_ptr<KnobPath> KnobPathWPtr;
typedef boost::weak_ptr<KnobParametric> KnobParametricWPtr;
typedef boost::weak_ptr<KnobSeparator> KnobSeparatorWPtr;
typedef boost::weak_ptr<KnobString> KnobStringWPtr;
typedef boost::weak_ptr<KnobTable> KnobTableWPtr;
typedef boost::weak_ptr<KnobTableItem> KnobTableItemWPtr;
typedef boost::weak_ptr<KnobItemsTable> KnobItemsTableWPtr;
typedef boost::weak_ptr<Node> NodeWPtr;
typedef boost::weak_ptr<ParallelRenderArgsSetter> ParallelRenderArgsSetterWPtr;
typedef boost::weak_ptr<Plugin> PluginWPtr;
typedef boost::weak_ptr<PluginGroupNode> PluginGroupNodeWPtr;
typedef boost::weak_ptr<NodeCollection> NodeCollectionWPtr;
typedef boost::weak_ptr<PluginMemory> PluginMemoryWPtr;
typedef boost::weak_ptr<RotoPaintInteract> RotoPaintInteractWPtr;
typedef boost::weak_ptr<RotoPaint> RotoPaintWPtr;
typedef boost::weak_ptr<RotoDrawableItem> RotoDrawableItemWPtr;
typedef boost::weak_ptr<RotoStrokeItem> RotoStrokeItemWPtr;
typedef boost::weak_ptr<TimeLine> TimeLineWPtr;
typedef boost::weak_ptr<TrackerParamsProvider> TrackerParamsProviderWPtr;
typedef boost::weak_ptr<TrackerHelper> TrackerHelperWPtr;
typedef boost::weak_ptr<TrackMarker> TrackMarkerWPtr;
typedef boost::weak_ptr<ViewerInstance> ViewerInstanceWPtr;
typedef boost::weak_ptr<ViewerNode> ViewerNodeWPtr;
typedef std::list<ImagePtr> ImageList;
typedef std::list<NodePtr> NodesList;
typedef std::list<NodeWPtr> NodesWList;
typedef std::vector<KnobIPtr> KnobsVec;
typedef std::list< BezierCPPtr > BezierCPs;


NATRON_PYTHON_NAMESPACE_ENTER;
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
class ItemsTable;
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

NATRON_PYTHON_NAMESPACE_EXIT;

NATRON_NAMESPACE_EXIT;

#endif // Engine_EngineFwd_h
