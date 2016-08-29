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

#ifndef SERIALIZATIONFWD_H
#define SERIALIZATIONFWD_H

#include <sstream>
#include <list>
#include <vector>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#endif // NATRON_BOOST_SERIALIZATION_COMPAT

#endif // #if !defined(Q_MOC_RUN) && !defined(SBK_RUN)

#include "Global/Macros.h"


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include "yaml.h"
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

SERIALIZATION_NAMESPACE_ENTER;
class BezierSerialization;
class CurveSerialization;
class ImageComponentsSerialization;
class ImageKeySerialization;
class ImageParamsSerialization;
class FrameKeySerialization;
class FrameParamsSerialization;
class KnobSerializationBase;
class KnobSerialization;
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
class RotoItemSerialization;
class RotoDrawableItemSerialization;
class RotoLayerSerialization;
class RotoStrokeItemSerialization;
class TabWidgetSerialization;
class TrackerContextSerialization;
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
typedef boost::shared_ptr<NodeSerialization> NodeSerializationPtr;
typedef boost::shared_ptr<ProjectSerialization> ProjectSerializationPtr;
typedef boost::shared_ptr<RotoDrawableItemSerialization> RotoDrawableItemSerializationPtr;
typedef boost::shared_ptr<RotoItemSerialization> RotoItemSerializationPtr;
typedef boost::shared_ptr<RotoLayerSerialization> RotoLayerSerializationPtr;
typedef boost::shared_ptr<RotoStrokeItemSerialization> RotoStrokeItemSerializationPtr;

typedef std::list<NodeSerializationPtr> NodeSerializationList;
typedef std::list<KnobSerializationPtr> KnobSerializationList;
SERIALIZATION_NAMESPACE_EXIT;

#endif // SERIALIZATIONFWD_H
