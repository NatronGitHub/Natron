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

#include <list>
#include <vector>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

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

class QChar;
class QDateTime;
class QFileInfo;
class QLocalServer;
class QLocalSocket;
class QMutex;
class QProcess;
class QSettings;
class QString;
class QStringList;
class QThread;
class QTimer;
class QWaitCondition;

// cairo

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
}

// Natron Engine

NATRON_NAMESPACE_ENTER;
class AbstractOfxEffectInstance;
class AppInstance;
class AppSettings;
class AppTLS;
class Bezier;
class BezierCP;
class BlockingBackgroundRender;
class BooleanParam;
class BufferableObject;
class ButtonParam;
class CLArgs;
class CacheEntryHolder;
class CacheSignalEmitter;
struct CreateNodeArgs;
class ChoiceExtraData;
class ChoiceParam;
class ColorParam;
class Curve;
class DockablePanelI;
class Double2DParam;
class Double3DParam;
class DoubleParam;
class Effect;
class EffectInstance;
class ExistenceCheckerThread;
class FileParam;
class Format;
class FrameEntry;
class FrameKey;
class FrameParams;
class GenericAccess;
class GroupParam;
class Hash64;
class Image;
class ImageComponents;
class ImageKey;
class ImageLayer;
class ImageParams;
class Int2DParam;
class Int3DParam;
class IntParam;
class KeyFrame;
class KnobBool;
class KnobButton;
class KnobChoice;
class KnobColor;
class KnobDouble;
class KnobFactory;
class KnobFile;
class KnobGroup;
class KnobHelper;
class KnobHolder;
class KnobI;
class KnobInt;
class KnobOutputFile;
class KnobPage;
class KnobParametric;
class KnobPath;
class KnobSeparator;
class KnobSerialization;
class KnobString;
class LibraryBinary;
class Node;
class NodeCollection;
class NodeGroup;
class NodeGraphI;
class NodeGuiI;
class NodeSerialization;
class NodeSettingsPanel;
class OfxClipInstance;
class OfxEffectInstance;
class OfxHost;
class OfxImage;
class OfxImageEffectInstance;
class OfxOverlayInteract;
class OfxParamOverlayInteract;
class OfxParamToKnob;
class OfxStringInstance;
class OpenGLViewerI;
class OutputEffectInstance;
class OutputFileParam;
class OverlaySupport;
class PageParam;
class ParallelRenderArgsSetter;
class Param;
class ParametricParam;
class PathParam;
class Plugin;
class PluginGroupNode;
class PluginMemory;
class PrecompNode;
class ProcessHandler;
class ProcessInputChannel;
class Project;
class ProjectSerialization;
class RectD;
class RectI;
class RenderEngine;
class RenderStats;
class RenderingFlagSetter;
class RequestedFrame;
class RichText_Knob;
class Roto;
class RotoContext;
class RotoDrawableItem;
class RotoItem;
class RotoItemSerialization;
class RotoLayer;
class RotoPoint;
class RotoStrokeItem;
class SeparatorParam;
class Settings;
class StringAnimationManager;
class StringParam;
class TLSHolderBase;
class TextureRect;
class TimeLine;
class UserParamHolder;
class ViewerInstance;
class ViewIdx;
namespace Color {
class Lut;
}
namespace Transform {
struct Matrix3x3;
}

typedef boost::shared_ptr<Node> NodePtr;
typedef std::list<NodePtr> NodesList;

typedef boost::weak_ptr<Node> NodeWPtr;
typedef std::list<NodeWPtr> NodesWList;

typedef boost::shared_ptr<KnobI> KnobPtr;
typedef std::vector<KnobPtr> KnobsVec;


typedef boost::shared_ptr<EffectInstance> EffectInstPtr;
typedef boost::weak_ptr<EffectInstance> EffectInstWPtr;

NATRON_NAMESPACE_EXIT;

#endif // Engine_EngineFwd_h
