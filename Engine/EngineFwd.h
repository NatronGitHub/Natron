/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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
class AbstractOfxEffectInstance;


// Qt

class QChar;
class QDateTime;
class QFileInfo;
class QLocalServer;
class QLocalSocket;
class QMutex;
class QSettings;
class QString;
class QStringList;
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

// Natron Engine

class AppInstance;
class AppSettings;
class Bezier;
class BezierCP;
class BlockingBackgroundRender;
class BooleanParam;
class BufferableObject;
class ButtonParam;
class CLArgs;
class CacheEntryHolder;
class ChoiceExtraData;
class ChoiceParam;
class ColorParam;
class Curve;
class DockablePanelI;
class Double2DParam;
class Double3DParam;
class DoubleParam;
class Effect;
class FileParam;
class Format;
class GlobalOFXTLS;
class GroupParam;
class Hash64;
class Int2DParam;
class Int3DParam;
class IntParam;
class KeyFrame;
class KnobBool;
class KnobButton;
class KnobChoice;
class KnobColor;
class KnobDouble;
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
class NodeCollection;
class NodeGraphI;
class NodeGuiI;
class NodeSerialization;
class NodeSettingsPanel;
class OfxClipInstance;
class OfxEffectInstance;
class OfxImage;
class OpenGLViewerI;
class OutputFileParam;
class OverlaySupport;
class PageParam;
class ParallelRenderArgsSetter;
class Param;
class ParametricParam;
class PathParam;
class PluginGroupNode;
class PluginMemory;
class ProcessHandler;
class ProcessInputChannel;
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
class Settings;
class StringAnimationManager;
class StringParam;
class TextureRect;
class TimeLine;
class ViewerInstance;

namespace Natron {
class CacheSignalEmitter;
class EffectInstance;
class FrameEntry;
class FrameKey;
class FrameParams;
class GenericAccess;
class Image;
class ImageComponents;
class ImageKey;
class ImageParams;
class LibraryBinary;
class Node;
class OfxHost;
class OfxImageEffectInstance;
class OfxOverlayInteract;
class OfxParamOverlayInteract;
class OutputEffectInstance;
class Plugin;
class Project;
namespace Color {
class Lut;
}
}

namespace Transform {
struct Matrix3x3;
}




#endif // Engine_EngineFwd_h
