/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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

#ifndef SERIALIZATIONFWD_H
#define SERIALIZATIONFWD_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <string>

#include "Global/Macros.h"

namespace boost {
template<class T> class weak_ptr;
template<class T> class shared_ptr;
}

SERIALIZATION_NAMESPACE_ENTER

class BezierSerialization;
class CurveSerialization;
struct DefaultValueSerialization;
class ImagePlaneDescSerialization;
class KnobSerializationBase;
class KnobSerialization;
class KnobTableItemSerialization;
class KnobItemsTableSerialization;
class GroupKnobSerialization;
class NodeClipBoard;
class NodeSerialization;
class NodePresetSerialization;
class ProjectBeingLoadedInfo;
class ProjectSerialization;
class PythonPanelSerialization;
class RectDSerialization;
class RectISerialization;
class RotoContextSerialization;
class RotoStrokeItemSerialization;
class TabWidgetSerialization;
struct ValueSerialization;
class ViewportData;
class WidgetSplitterSerialization;
class WindowSerialization;
class WorkspaceSerialization;

typedef boost::shared_ptr<BezierSerialization> BezierSerializationPtr;
typedef boost::shared_ptr<CurveSerialization> CurveSerializationPtr;
typedef boost::shared_ptr<GroupKnobSerialization> GroupKnobSerializationPtr;
typedef boost::shared_ptr<KnobSerialization> KnobSerializationPtr;
typedef boost::shared_ptr<KnobSerializationBase> KnobSerializationBasePtr;
typedef boost::shared_ptr<KnobTableItemSerialization> KnobTableItemSerializationPtr;
typedef boost::shared_ptr<KnobItemsTableSerialization> KnobItemsTableSerializationPtr;
typedef boost::shared_ptr<NodeSerialization> NodeSerializationPtr;
typedef boost::shared_ptr<ProjectSerialization> ProjectSerializationPtr;
typedef boost::shared_ptr<RotoStrokeItemSerialization> RotoStrokeItemSerializationPtr;

typedef std::list<NodeSerializationPtr> NodeSerializationList;
typedef std::list<KnobSerializationPtr> KnobSerializationList;

SERIALIZATION_NAMESPACE_EXIT

#ifndef YAML
#error "YAML should be defined to YAML_NATRON"
#endif

namespace YAML {
class Emitter;
class Node;
}

#endif // SERIALIZATIONFWD_H
