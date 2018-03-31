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

#ifndef SERIALIZATIONCOMPAT_H
#define SERIALIZATIONCOMPAT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

// This file contains all implementation of old boost serialization involving Engine and Gui classes

#include <sstream> // stringstream

#include "Global/Macros.h"


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/algorithm/string/predicate.hpp> // iequals
#include <boost/utility.hpp> // next
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)

#include "Engine/ImagePlaneDesc.h"

SERIALIZATION_NAMESPACE_ENTER


SERIALIZATION_NAMESPACE_EXIT

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/algorithm/string/predicate.hpp> // iequals
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/scoped_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "Serialization/RotoStrokeItemSerialization.h"
#include "Serialization/BezierSerialization.h"
#include "Serialization/BezierCPSerialization.h"
#include "Serialization/SerializationBase.h"
#include "Serialization/CurveSerialization.h"
#include "Serialization/KnobSerialization.h"
#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/ProjectSerialization.h"
#include "Serialization/SerializationFwd.h"

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/BezierCPPrivate.h"
#include "Engine/ColorParser.h"
#include "Engine/CurvePrivate.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/Curve.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EffectInstance.h"
#include "Engine/Format.h"
#include "Engine/RectI.h"
#include "Engine/TrackerNode.h"
#include "Engine/ViewIdx.h"
#include <SequenceParsing.h>

#define kKnobOutputFileTypeName "OutputFile"


#define BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE 2
#define BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE 3
#define BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER 4
#define BEZIER_SERIALIZATION_VERSION BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER

#define BEZIER_CP_INTRODUCES_OFFSET 2
#define BEZIER_CP_FIX_BUG_CURVE_POINTER 3
#define BEZIER_CP_REMOVE_OFFSET 4
#define BEZIER_CP_VERSION BEZIER_CP_REMOVE_OFFSET

#define FORMAT_SERIALIZATION_CHANGES_TO_RECTD 2
#define FORMAT_SERIALIZATION_CHANGES_TO_RECTI 3
#define FORMAT_SERIALIZATION_VERSION FORMAT_SERIALIZATION_CHANGES_TO_RECTI

#define ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING 2
#define ROTO_DRAWABLE_ITEM_REMOVES_INVERTED 3
#define ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST 4
#define ROTO_DRAWABLE_ITEM_VERSION ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST

#define ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER 2
#define ROTO_LAYER_SERIALIZATION_VERSION ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER

#define ROTO_ITEM_INTRODUCES_LABEL 2
#define ROTO_ITEM_VERSION ROTO_ITEM_INTRODUCES_LABEL

#define ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES 2
#define ROTO_STROKE_SERIALIZATION_VERSION ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES

#define ROTO_CTX_REMOVE_COUNTERS 2
#define ROTO_CTX_VERSION ROTO_CTX_REMOVE_COUNTERS

#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS 2
#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET 3
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_LABEL 4
#define KNOB_SERIALIZATION_INTRODUCES_USER_KNOB 5
#define KNOB_SERIALIZATION_NODE_SCRIPT_NAME 6
#define KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS 7
#define KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS 8
#define KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 9
#define KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX 10
#define KNOB_SERIALIZATION_INTRODUCES_ALIAS 11
#define KNOB_SERIALIZATION_REMOVE_SLAVED_TRACKS 12
#define KNOB_SERIALIZATION_REMOVE_DEFAULT_VALUES 13
#define KNOB_SERIALIZATION_INTRODUCE_USER_KNOB_ICON_FILE_PATH 15
#define KNOB_SERIALIZATION_CHANGE_CURVE_SERIALIZATION 16
#define KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION 17
#define KNOB_SERIALIZATION_VERSION KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION

#define VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL 2
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS 3
#define VALUE_SERIALIZATION_REMOVES_EXTRA_DATA 4
#define VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS 5
#define VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS 6
#define VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES 7
#define VALUE_SERIALIZATION_CHANGES_CURVE_SERIALIZATION 8
#define VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE 9
#define VALUE_SERIALIZATION_VERSION VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE

#define MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME 2
#define MASTER_SERIALIZATION_VERSION MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME

#define GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME 2
#define GROUP_KNOB_SERIALIZATION_VERSION GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME

#define IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID 2
#define IMAGEPLANEDESC_SERIALIZATION_VERSION IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID

#define NODE_SERIALIZATION_V_INTRODUCES_ROTO 2
#define NODE_SERIALIZATION_INTRODUCES_MULTI_INSTANCE 3
#define NODE_SERIALIZATION_INTRODUCES_USER_KNOBS 4
#define NODE_SERIALIZATION_INTRODUCES_GROUPS 5
#define NODE_SERIALIZATION_EMBEDS_MULTI_INSTANCE_CHILDREN 6
#define NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME 7
#define NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE 8
#define NODE_SERIALIZATION_CHANGE_INPUTS_SERIALIZATION 9
#define NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS 10
#define NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION 11
#define NODE_SERIALIZATION_INTRODUCES_CACHE_ID 12
#define NODE_SERIALIZATION_SERIALIZE_PYTHON_MODULE_ALWAYS 13
#define NODE_SERIALIZATION_SERIALIZE_PAGE_INDEX 14
#define NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT 15
#define NODE_SERIALIZATION_CHANGE_PYTHON_MODULE_TO_ONLY_NAME 17
#define NODE_SERIALIZATION_CURRENT_VERSION NODE_SERIALIZATION_CHANGE_PYTHON_MODULE_TO_ONLY_NAME

#define TRACK_SERIALIZATION_ADD_TRACKER_PM 2
#define TRACK_SERIALIZATION_VERSION TRACK_SERIALIZATION_ADD_TRACKER_PM
#define TRACKER_CONTEXT_SERIALIZATION_VERSION 1

#define PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION 2
#define PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS 3
#define PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS 4
#define PROJECT_SERIALIZATION_INTRODUCES_GROUPS 5
#define PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION 6
#define PROJECT_SERIALIZATION_VERSION PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION

SERIALIZATION_NAMESPACE_ENTER
namespace Compat {

class RotoItemSerialization
{

public:

    std::string name, label;
    std::string parentLayerName;
    bool locked;

    RotoItemSerialization()
    : name()
    , parentLayerName()
    , locked(false)
    {
    }

    virtual ~RotoItemSerialization()
    {
    }

    void convertRotoItemSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization);


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
};

class RotoLayerSerialization
: public RotoItemSerialization
{

public:

    RotoLayerSerialization()
    : RotoItemSerialization()
    {
    }

    virtual ~RotoLayerSerialization()
    {
    }



    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    void convertRotoLayerSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization);


    std::list <boost::shared_ptr<Compat::RotoItemSerialization> > children;

    KnobSerializationList knobs;
};

class RotoDrawableItemSerialization
: public RotoItemSerialization
{

public:


    KnobSerializationList _knobs;

    RotoDrawableItemSerialization()
    : RotoItemSerialization()
    , _knobs()
    {
    }

    virtual ~RotoDrawableItemSerialization()
    {
    }

    void convertRotoDrawableItemSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization);
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
};


struct StrokePoint
{
    double x, y, pressure;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
    
};



class BezierSerialization
: public RotoDrawableItemSerialization
{

public:

    BezierSerialization()
    : RotoDrawableItemSerialization()
    , _isOpenBezier(false)
    {
    }

    virtual ~BezierSerialization()
    {
    }


    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int version);


    void convertBezierSerialization(SERIALIZATION_NAMESPACE::BezierSerialization* outSerialization);

    struct ControlPoint
    {
        BezierCPSerialization innerPoint;

        // Serialize feather only if different
        boost::shared_ptr<BezierCPSerialization> featherPoint;
    };

    std::list<ControlPoint> _controlPoints;

    bool _closed;
    bool _isOpenBezier;
};


class RotoStrokeItemSerialization
: public RotoDrawableItemSerialization
{

public:


    std::string _brushType;
    struct PointCurves
    {
        CurveSerializationPtr x,y,pressure;
    };
    std::list<PointCurves> _subStrokes;


    RotoStrokeItemSerialization()
    : RotoDrawableItemSerialization()
    , _brushType()
    , _subStrokes()
    {
    }

    virtual ~RotoStrokeItemSerialization()
    {
    }


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    void convertStrokeSerialization(SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization* outSerialization);

    
};


class TrackSerialization
{

public:


    bool _enabled;
    bool _isPM;
    std::string _label, _scriptName;
    std::list<KnobSerializationPtr> _knobs;
    std::list<int> _userKeys;

    TrackSerialization()
    : _enabled(true)
    , _isPM(false)
    , _label()
    , _scriptName()
    , _knobs()
    , _userKeys()
    {
    }


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    void convertTrackSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization);

};

class RotoContextSerialization
{

public:

    RotoContextSerialization()
    : _baseLayer()
    {
    }

    virtual ~RotoContextSerialization()
    {
    }


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);

    void convertRotoContext(SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* outSerialization);

    RotoLayerSerialization _baseLayer;
};

class TrackerContextSerialization
{
public:

    TrackerContextSerialization()
    : _tracks()
    {
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


    void convertTrackerContext(SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* outSerialization);

    std::list<TrackSerialization> _tracks;
};

class ImagePlaneDescSerialization
{
public:

    ImagePlaneDescSerialization()
    {

    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);


    std::string _planeID, _planeLabel, _channelsLabel;
    std::vector<std::string> _channels;

};

class NodeCollectionSerialization
{

public:

    // The list of all nodes in the collection
    std::list<NodeSerializationPtr> _serializedNodes;


    NodeCollectionSerialization()
    {
    }

    virtual ~NodeCollectionSerialization()
    {
        _serializedNodes.clear();
    }

    const std::list<NodeSerializationPtr> & getNodesSerialization() const
    {
        return _serializedNodes;
    }

    void addNodeSerialization(const NodeSerializationPtr& s)
    {
        _serializedNodes.push_back(s);
    }


    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        int nodesCount;
        ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);

        for (int i = 0; i < nodesCount; ++i) {
            NodeSerializationPtr s = boost::make_shared<NodeSerialization>();
            ar & ::boost::serialization::make_nvp("item", *s);
            _serializedNodes.push_back(s);
        }
    }
};




} // namespace Compat





SERIALIZATION_NAMESPACE_EXIT

// Everything below is deprecated and maintained for projects prior to Natron 2.2
template<class Archive>
void
NATRON_NAMESPACE::BezierCP::serialize(Archive &ar, const unsigned int version)
{

    if ( (version >= BEZIER_CP_FIX_BUG_CURVE_POINTER) ) {
        ar & ::boost::serialization::make_nvp("X", _imp->x);
        NATRON_NAMESPACE::Curve xCurve;
        ar & ::boost::serialization::make_nvp("X_animation", xCurve);
        _imp->curveX->clone(xCurve);

        if (version < BEZIER_CP_FIX_BUG_CURVE_POINTER) {
            NATRON_NAMESPACE::Curve curveBug;
            ar & ::boost::serialization::make_nvp("Y", curveBug);
        } else {
            ar & ::boost::serialization::make_nvp("Y", _imp->y);
        }

        NATRON_NAMESPACE::Curve yCurve;
        ar & ::boost::serialization::make_nvp("Y_animation", yCurve);
        _imp->curveY->clone(yCurve);

        ar & ::boost::serialization::make_nvp("Left_X", _imp->leftX);
        NATRON_NAMESPACE::Curve leftCurveX, leftCurveY, rightCurveX, rightCurveY;
        ar & ::boost::serialization::make_nvp("Left_X_animation", leftCurveX);
        ar & ::boost::serialization::make_nvp("Left_Y", _imp->leftY);
        ar & ::boost::serialization::make_nvp("Left_Y_animation", leftCurveY);
        ar & ::boost::serialization::make_nvp("Right_X", _imp->rightX);
        ar & ::boost::serialization::make_nvp("Right_X_animation", rightCurveX);
        ar & ::boost::serialization::make_nvp("Right_Y", _imp->rightY);
        ar & ::boost::serialization::make_nvp("Right_Y_animation", rightCurveY);

        _imp->curveLeftBezierX->clone(leftCurveX);
        _imp->curveLeftBezierY->clone(leftCurveY);
        _imp->curveRightBezierX->clone(rightCurveX);
        _imp->curveRightBezierY->clone(rightCurveY);
    }
    if ( (version >= BEZIER_CP_INTRODUCES_OFFSET) && (version < BEZIER_CP_REMOVE_OFFSET) ) {
        int offsetTime;
        ar & ::boost::serialization::make_nvp("OffsetTime", offsetTime);
    }
}


template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::BezierSerialization::serialize(Archive & ar, const unsigned int version)
{
    boost::serialization::void_cast_register<Compat::BezierSerialization, Compat::RotoDrawableItemSerialization>(
                                                                                                 static_cast<Compat::BezierSerialization *>(NULL),
                                                                                                 static_cast<Compat::RotoDrawableItemSerialization *>(NULL)
                                                                                                 );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
    int numPoints;
    ar & ::boost::serialization::make_nvp("NumPoints", numPoints);
    if ( (version >= BEZIER_SERIALIZATION_INTRODUCES_ROTO_STROKE) && (version < BEZIER_SERIALIZATION_REMOVES_IS_ROTO_STROKE) ) {
        bool isStroke;
        ar & ::boost::serialization::make_nvp("IsStroke", isStroke);
    }
    for (int i = 0; i < numPoints; ++i) {
        NATRON_NAMESPACE::BezierCP cp;
        ar & ::boost::serialization::make_nvp("CP", cp);
        NATRON_NAMESPACE::BezierCP fp;
        ar & ::boost::serialization::make_nvp("FP", fp);

        ControlPoint controlPoint;
        cp.toSerialization(&controlPoint.innerPoint);

        if (fp != cp) {
            controlPoint.featherPoint = boost::make_shared<BezierCPSerialization>();
            fp.toSerialization(controlPoint.featherPoint.get());
        }

        _controlPoints.push_back(controlPoint);


    }
    ar & ::boost::serialization::make_nvp("Closed", _closed);
    if (version >= BEZIER_SERIALIZATION_INTRODUCES_OPEN_BEZIER) {
        ar & ::boost::serialization::make_nvp("OpenBezier", _isOpenBezier);
    } else {
        _isOpenBezier = false;
    }
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::StrokePoint::serialize(Archive & ar,
               const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("X", x);
    ar & ::boost::serialization::make_nvp("Y", y);
    ar & ::boost::serialization::make_nvp("Press", pressure);
}


template<class Archive>
void SERIALIZATION_NAMESPACE::Compat::RotoItemSerialization::serialize(Archive & ar,
                                                                       const unsigned int version)
{

    ar & ::boost::serialization::make_nvp("Name", name);
    if (version >= ROTO_ITEM_INTRODUCES_LABEL) {
        ar & ::boost::serialization::make_nvp("Label", label);
    }
    bool act;
    ar & ::boost::serialization::make_nvp("Activated", act);
    ar & ::boost::serialization::make_nvp("ParentLayer", parentLayerName);
    ar & ::boost::serialization::make_nvp("Locked", locked);
}


template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::RotoDrawableItemSerialization::serialize(Archive & ar,
          const unsigned int version)
{
    boost::serialization::void_cast_register<Compat::RotoDrawableItemSerialization, Compat::RotoItemSerialization>(
                                                                                                   static_cast<Compat::RotoDrawableItemSerialization *>(NULL),
                                                                                                   static_cast<Compat::RotoItemSerialization *>(NULL)
                                                                                                   );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
    if (version < ROTO_DRAWABLE_ITEM_CHANGES_TO_LIST) {
        KnobSerializationPtr activated = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("Activated", *activated);
        _knobs.push_back(activated);
        KnobSerializationPtr opacity = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("Opacity", *opacity);
        _knobs.push_back(opacity);
        KnobSerializationPtr feather = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("Feather", *feather);
        _knobs.push_back(feather);
        KnobSerializationPtr falloff = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("FallOff", *falloff);
        _knobs.push_back(falloff);
        if (version < ROTO_DRAWABLE_ITEM_REMOVES_INVERTED) {
            KnobSerializationPtr inverted = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Inverted", *inverted);
            _knobs.push_back(inverted);
        }
        if (version >= ROTO_DRAWABLE_ITEM_INTRODUCES_COMPOSITING) {
            KnobSerializationPtr color = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Color", *color);
            _knobs.push_back(color);
            KnobSerializationPtr comp = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("CompOP", *comp);
            _knobs.push_back(comp);
        }
    } else {
        int nKnobs;
        ar & ::boost::serialization::make_nvp("NbItems", nKnobs);
        for (int i = 0; i < nKnobs; ++i) {
            KnobSerializationPtr k = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("Item", *k);
            _knobs.push_back(k);
        }
    }
    double overlayColor[4];
    ar & ::boost::serialization::make_nvp("OC.r", overlayColor[0]);
    ar & ::boost::serialization::make_nvp("OC.g", overlayColor[1]);
    ar & ::boost::serialization::make_nvp("OC.b", overlayColor[2]);
    ar & ::boost::serialization::make_nvp("OC.a", overlayColor[3]);
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::RotoLayerSerialization::serialize(Archive & ar,
          const unsigned int version)
{

    boost::serialization::void_cast_register<Compat::RotoLayerSerialization, Compat::RotoItemSerialization>(
                                                                                            static_cast<Compat::RotoLayerSerialization *>(NULL),
                                                                                            static_cast<Compat::RotoItemSerialization *>(NULL)
                                                                                            );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
    int numChildren;
    ar & ::boost::serialization::make_nvp("NumChildren", numChildren);
    for (int i = 0; i < numChildren; ++i) {
        int type;
        if (version < ROTO_LAYER_SERIALIZATION_REMOVES_IS_BEZIER) {
            bool bezier;
            ar & ::boost::serialization::make_nvp("IsBezier", bezier);
            type = bezier ? 0 : 2;
        } else {
            ar & ::boost::serialization::make_nvp("Type", type);
        }
        if (type == 0) {
            boost::shared_ptr<Compat::BezierSerialization> b = boost::make_shared<Compat::BezierSerialization>();
            ar & ::boost::serialization::make_nvp("Item", *b);
            children.push_back(b);
        } else if (type == 1) {
            boost::shared_ptr<Compat::RotoStrokeItemSerialization> b = boost::make_shared<Compat::RotoStrokeItemSerialization>();
            ar & ::boost::serialization::make_nvp("Item", *b);
            children.push_back(b);
        } else {
            boost::shared_ptr<Compat::RotoLayerSerialization> l = boost::make_shared<Compat::RotoLayerSerialization>();
            ar & ::boost::serialization::make_nvp("Item", *l);
            children.push_back(l);
        }
    }
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::RotoStrokeItemSerialization::serialize(Archive & ar,
          const unsigned int version)
{
    boost::serialization::void_cast_register<Compat::RotoStrokeItemSerialization, Compat::RotoDrawableItemSerialization>(
                                                                                                         static_cast<Compat::RotoStrokeItemSerialization *>(NULL),
                                                                                                         static_cast<Compat::RotoDrawableItemSerialization *>(NULL)
                                                                                                         );
    ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Compat::RotoDrawableItemSerialization);
    if (version < ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES) {
        ar & ::boost::serialization::make_nvp("BrushType", _brushType);
        NATRON_NAMESPACE::Curve x,y,p;
        ar & ::boost::serialization::make_nvp("CurveX", x);
        ar & ::boost::serialization::make_nvp("CurveY", y);
        ar & ::boost::serialization::make_nvp("CurveP", p);
        PointCurves subStroke;
        subStroke.x = boost::make_shared<CurveSerialization>();
        subStroke.y = boost::make_shared<CurveSerialization>();
        subStroke.pressure = boost::make_shared<CurveSerialization>();
        x.toSerialization(subStroke.x.get());
        y.toSerialization(subStroke.y.get());
        p.toSerialization(subStroke.pressure.get());
        _subStrokes.push_back(subStroke);

    } else {
        int nb;
        ar & ::boost::serialization::make_nvp("BrushType", _brushType);
        ar & ::boost::serialization::make_nvp("NbItems", nb);
        for (int i = 0; i < nb; ++i) {
            NATRON_NAMESPACE::Curve x,y,p;
            ar & ::boost::serialization::make_nvp("CurveX", x);
            ar & ::boost::serialization::make_nvp("CurveY", y);
            ar & ::boost::serialization::make_nvp("CurveP", p);
            PointCurves subStroke;
            subStroke.x = boost::make_shared<CurveSerialization>();
            subStroke.y = boost::make_shared<CurveSerialization>();
            subStroke.pressure = boost::make_shared<CurveSerialization>();
            x.toSerialization(subStroke.x.get());
            y.toSerialization(subStroke.y.get());
            p.toSerialization(subStroke.pressure.get());
            _subStrokes.push_back(subStroke);
        }
    }
}


template<class Archive>
void NATRON_NAMESPACE::KeyFrame::serialize(Archive & ar,
               const unsigned int /*version*/)
{
    double time = _time;
    ar & ::boost::serialization::make_nvp("Time", time);
    _time = TimeValue(time);
    ar & ::boost::serialization::make_nvp("Value", _value);
    ar & ::boost::serialization::make_nvp("InterpolationMethod", _interpolation);
    ar & ::boost::serialization::make_nvp("LeftDerivative", _leftDerivative);
    ar & ::boost::serialization::make_nvp("RightDerivative", _rightDerivative);
}

template<class Archive>
void NATRON_NAMESPACE::Curve::serialize(Archive & ar, const unsigned int /*version*/)
{
    QMutexLocker k(&_imp->_lock);
    ar & ::boost::serialization::make_nvp("KeyFrameSet", _imp->keyFrames);
    KeyFrameSet newKeys;
    // We are deleting elements while iterating, be careful!
    // See https://stackoverflow.com/questions/2874441/deleting-elements-from-stl-set-while-iterating
    for (KeyFrameSet::iterator it = _imp->keyFrames.begin(); it != _imp->keyFrames.end();) {
        // Natron 2 wrongly sets keyframe tangents when refreshing them over multiple keyframes at once.
        // As a result of this, the tangents saved with the interpolation type may not be correct.
        // Since Natron 3 only saves interpolation type if this is a known interpolation mode, the project could
        // not be loaded correctly.
        KeyframeTypeEnum interpolation = it->getInterpolation();
        if ( interpolation == eKeyframeTypeSmooth ) {
            if ( it == _imp->keyFrames.begin() ||
                (it != _imp->keyFrames.end() && boost::next(it) == _imp->keyFrames.end() ) ) {
                // KeyFrameSet sorts by time.
                // If it is the first or last frame and interpolation is smooth, convert to broken
                interpolation = eKeyframeTypeBroken;
            } else {
                interpolation = eKeyframeTypeFree;
            }
            KeyFrame tmp = *it;
            tmp.setInterpolation(interpolation);
            newKeys.insert(tmp);
            _imp->keyFrames.erase(it++);
        } else if (interpolation == eKeyframeTypeCatmullRom ||
                   interpolation == eKeyframeTypeCubic) {
            KeyFrame tmp = *it;
            interpolation = eKeyframeTypeFree;
            tmp.setInterpolation(interpolation);
            newKeys.insert(tmp);
            _imp->keyFrames.erase(it++);
        } else {
            ++it;
        }
    }
    _imp->keyFrames.insert( newKeys.begin(), newKeys.end() );
}


template<class Archive>
void SERIALIZATION_NAMESPACE::Compat::ImagePlaneDescSerialization::serialize(Archive & ar, const unsigned int version)
{
    if (version < IMAGEPLANEDESC_SERIALIZATION_INTRODUCES_ID) {
        ar &  boost::serialization::make_nvp("Layer", _planeID);
        _planeLabel = _planeID;
        ar &  boost::serialization::make_nvp("Components", _channels);
        ar &  boost::serialization::make_nvp("CompName", _channelsLabel);
    } else {
        ar &  boost::serialization::make_nvp("PlaneID", _planeID);
        ar &  boost::serialization::make_nvp("PlaneLabel", _planeLabel);
        ar &  boost::serialization::make_nvp("ChannelsLabel", _channelsLabel);
        ar &  boost::serialization::make_nvp("Channels", _channels);

    }
}


template<class Archive>
void SERIALIZATION_NAMESPACE::Compat::RotoContextSerialization::serialize(Archive & ar,
          const unsigned int version)
{
    ar & ::boost::serialization::make_nvp("BaseLayer", _baseLayer);

    bool autoKeying, rippleEdit, featherLink;
    ar & ::boost::serialization::make_nvp("AutoKeying", autoKeying);
    ar & ::boost::serialization::make_nvp("RippleEdit", rippleEdit);
    ar & ::boost::serialization::make_nvp("FeatherLink", featherLink);

    std::list<std::string> selectedItems;
    ar & ::boost::serialization::make_nvp("Selection", selectedItems);

    if (version < ROTO_CTX_REMOVE_COUNTERS) {
        std::map<std::string, int> _itemCounters;
        ar & ::boost::serialization::make_nvp("Counters", _itemCounters);
    }
}

template<class Archive>
void NATRON_NAMESPACE::Format::serialize(Archive & ar,
               const unsigned int version)
{
    if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTD) {
        RectI r;
        ar & ::boost::serialization::make_nvp("RectI", r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;
    } else if (version < FORMAT_SERIALIZATION_CHANGES_TO_RECTI) {
        RectD r;
        ar & ::boost::serialization::make_nvp("RectD", r);
        x1 = r.x1;
        x2 = r.x2;
        y1 = r.y1;
        y2 = r.y2;
    } else {
        boost::serialization::void_cast_register<Format, RectI>( static_cast<Format *>(NULL),
                                                                static_cast<RectI *>(NULL) );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RectI);
    }
    ar & ::boost::serialization::make_nvp("Pixel_aspect_ratio", _par);
    ar & ::boost::serialization::make_nvp("Name", _name);
}

template<class Archive>
void
NATRON_NAMESPACE::RectI::serialize(Archive & ar,
                                   const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Left", x1);
    ar & ::boost::serialization::make_nvp("Bottom", y1);
    ar & ::boost::serialization::make_nvp("Right", x2);
    ar & ::boost::serialization::make_nvp("Top", y2);
}


template<class Archive>
void
NATRON_NAMESPACE::RectD::serialize(Archive & ar,
                                   const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Left", x1);
    ar & ::boost::serialization::make_nvp("Bottom", y1);
    ar & ::boost::serialization::make_nvp("Right", x2);
    ar & ::boost::serialization::make_nvp("Top", y2);
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::MasterSerialization::serialize(Archive & ar,
          const unsigned int version)
{
    int mDim;
    ar & ::boost::serialization::make_nvp("MasterDimension", mDim);
    std::stringstream ss;
    ss << mDim;
    masterDimensionName = ss.str();
    ar & ::boost::serialization::make_nvp("MasterNodeName", masterNodeName);
    ar & ::boost::serialization::make_nvp("MasterKnobName", masterKnobName);

    if (version >= MASTER_SERIALIZATION_INTRODUCE_MASTER_TRACK_NAME) {
        masterTableName = kTrackerParamTracksTableName;
        ar & ::boost::serialization::make_nvp("MasterTrackName", masterTableItemName);
    }
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::ValueSerialization::serialize(Archive & ar, const unsigned int version)
{
    // With boost, value was always serialized
    _serializeValue = true;

    _mustSerialize = true;

    bool isFile = _typeName == NATRON_NAMESPACE::KnobFile::typeNameStatic();
    bool isChoice = _typeName == NATRON_NAMESPACE::KnobChoice::typeNameStatic();

    bool enabled;
    ar & ::boost::serialization::make_nvp("Enabled", enabled);



    bool hasAnimation;
    ar & ::boost::serialization::make_nvp("HasAnimation", hasAnimation);
    bool convertOldFileKeyframesToPattern = false;
    if (hasAnimation) {
        NATRON_NAMESPACE::Curve c;
        ar & ::boost::serialization::make_nvp("Curve", c);
        c.toSerialization(&_animationCurve);

        convertOldFileKeyframesToPattern = isFile && getKnobName() == kOfxImageEffectFileParamName;
    }

    _serialization->_dataType = eSerializationValueVariantTypeNone;

    bool loadValue = true;
    if (version >= VALUE_SERIALIZATION_INTRODUCES_DATA_TYPE) {
        ar & ::boost::serialization::make_nvp("DataType", _serialization->_dataType);
    } else {
        bool isString = _typeName == NATRON_NAMESPACE::KnobString::typeNameStatic();
        bool isColor = _typeName == NATRON_NAMESPACE::KnobColor::typeNameStatic();
        bool isDouble = _typeName == NATRON_NAMESPACE::KnobDouble::typeNameStatic();
        bool isInt = _typeName == NATRON_NAMESPACE::KnobInt::typeNameStatic();
        bool isBool = _typeName == NATRON_NAMESPACE::KnobBool::typeNameStatic();
        bool isOutputFile = _typeName == kKnobOutputFileTypeName;
        bool isPath = _typeName == NATRON_NAMESPACE::KnobPath::typeNameStatic();
        bool isLayers = _typeName == NATRON_NAMESPACE::KnobLayers::typeNameStatic();


        if (isInt) {
            _serialization->_dataType = eSerializationValueVariantTypeInteger;
        } else if (isDouble || isColor) {
            _serialization->_dataType = eSerializationValueVariantTypeDouble;
        } else if (isBool) {
            _serialization->_dataType = eSerializationValueVariantTypeBoolean;
        } else if (isString || isOutputFile || isPath || isLayers || isFile || isChoice) {
            _serialization->_dataType = eSerializationValueVariantTypeString;
        }

        if (isChoice) {
            loadValue = false;
            ar & ::boost::serialization::make_nvp("Value", _value.isInt);
            assert(_value.isInt >= 0);
            if (version >= VALUE_SERIALIZATION_INTRODUCES_CHOICE_LABEL) {
                if (version < VALUE_SERIALIZATION_REMOVES_EXTRA_DATA) {
                    std::string label;
                    ar & ::boost::serialization::make_nvp("Label", label);
                    _value.isString  = label;
                }
            }
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isInt);
            }
        } else if (isFile) {
            loadValue = false;
            ar & ::boost::serialization::make_nvp("Value", _value.isString);

            ///Convert the old keyframes stored in the file parameter by analysing one keyframe
            ///and deducing the pattern from it and setting it as a value instead
            if (convertOldFileKeyframesToPattern) {
                SequenceParsing::FileNameContent content(_value.isString);
                content.generatePatternWithFrameNumberAtIndex(content.getPotentialFrameNumbersCount() - 1,
                                                              content.getLeadingZeroes() + 1,
                                                              &_value.isString);
            }
            if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isString);
            }
        }

    }

    if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
        _serialization->_defaultValues[_dimension].serializeDefaultValue = false;
    }

    if (loadValue) {
        switch (_serialization->_dataType) {
            case eSerializationValueVariantTypeNone:
            case eSerializationValueVariantTypeTable:
                break;

            case eSerializationValueVariantTypeInteger:
                ar & ::boost::serialization::make_nvp("Value", _value.isInt);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isInt);
                }
                break;

            case eSerializationValueVariantTypeDouble:
                ar & ::boost::serialization::make_nvp("Value", _value.isDouble);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isDouble);
                }
                break;

            case eSerializationValueVariantTypeString:
                ar & ::boost::serialization::make_nvp("Value", _value.isString);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isString);
                }
                break;

            case eSerializationValueVariantTypeBoolean:
                ar & ::boost::serialization::make_nvp("Value", _value.isBool);
                if (version >= VALUE_SERIALIZATION_INTRODUCES_DEFAULT_VALUES) {
                    ar & ::boost::serialization::make_nvp("Default", _serialization->_defaultValues[_dimension].value.isBool);
                }

                break;

        }
    }


    ///We cannot restore the master yet. It has to be done in another pass.
    bool hasMaster;
    ar & ::boost::serialization::make_nvp("HasMaster", hasMaster);
    if (hasMaster) {
        ar & ::boost::serialization::make_nvp("Master", _slaveMasterLink);
    }

    if (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS) {
        ar & ::boost::serialization::make_nvp("Expression", _expression);
        ar & ::boost::serialization::make_nvp("ExprHasRet", _expresionHasReturnVariable);
        _expressionLanguage = kKnobSerializationExpressionLanguagePython;
    }

    if ( (version >= VALUE_SERIALIZATION_INTRODUCES_EXPRESSIONS_RESULTS) && (version < VALUE_SERIALIZATION_REMOVES_EXPRESSIONS_RESULTS) ) {

        bool isString = _typeName == NATRON_NAMESPACE::KnobString::typeNameStatic();
        bool isColor = _typeName == NATRON_NAMESPACE::KnobColor::typeNameStatic();
        bool isDouble = _typeName == NATRON_NAMESPACE::KnobDouble::typeNameStatic();
        bool isInt = _typeName == NATRON_NAMESPACE::KnobInt::typeNameStatic();
        bool isBool = _typeName == NATRON_NAMESPACE::KnobBool::typeNameStatic();
        bool isFile = _typeName == NATRON_NAMESPACE::KnobFile::typeNameStatic();
        bool isOutFile = _typeName == kKnobOutputFileTypeName;
        bool isPath = _typeName == NATRON_NAMESPACE::KnobPath::typeNameStatic();

        bool isDoubleVal = isDouble || isColor;
        bool isIntVal = isChoice || isInt;
        bool isStrVal = isFile || isOutFile || isPath || isString;

        if (isIntVal) {
            std::map<int, int> exprValues;
            ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
        } else if (isBool) {
            std::map<int, bool> exprValues;
            ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
        } else if (isDoubleVal) {
            std::map<int, double> exprValues;
            ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
        } else if (isStrVal) {
            std::map<int, std::string> exprValues;
            ar & ::boost::serialization::make_nvp("ExprResults", exprValues);
        }
    }

    // Assume that by default the knob is enabled
    if (!hasMaster && !enabled) {
        setEnabledChanged(false);
    }
}

template<class Archive>
void
SERIALIZATION_NAMESPACE::KnobSerialization::serialize(Archive & ar,
          const unsigned int version)
{

    _boostSerializationVersion = version;
    _mustSerialize = true;

    ar & ::boost::serialization::make_nvp("Name", _scriptName);
    ar & ::boost::serialization::make_nvp("Type", _typeName);
    ar & ::boost::serialization::make_nvp("Dimension", _dimension);

    bool secret;
    ar & ::boost::serialization::make_nvp("Secret", secret);
    if (secret) {
        // Assume by default that knobs are visible
        _isSecret = true;
    }

    if (version >= KNOB_SERIALIZATION_INTRODUCES_ALIAS) {
        bool masterIsAlias;
        ar & ::boost::serialization::make_nvp("MasterIsAlias", masterIsAlias);
    }

    bool isFile = _typeName == NATRON_NAMESPACE::KnobFile::typeNameStatic();
    bool isOutFile = _typeName == kKnobOutputFileTypeName;
    bool isPath = _typeName == NATRON_NAMESPACE::KnobPath::typeNameStatic();
    bool isString = _typeName == NATRON_NAMESPACE::KnobString::typeNameStatic();
    bool isParametric = _typeName == NATRON_NAMESPACE::KnobParametric::typeNameStatic();
    bool isChoice = _typeName == NATRON_NAMESPACE::KnobChoice::typeNameStatic();
    bool isDouble = _typeName == NATRON_NAMESPACE::KnobDouble::typeNameStatic();
    bool isColor = _typeName == NATRON_NAMESPACE::KnobColor::typeNameStatic();
    bool isInt = _typeName == NATRON_NAMESPACE::KnobInt::typeNameStatic();
    bool isBool = _typeName == NATRON_NAMESPACE::KnobBool::typeNameStatic();

    if (isChoice && !_extraData) {
        _extraData.reset(new ChoiceExtraData);
    }
    if (isParametric && !_extraData) {
        _extraData.reset(new ParametricExtraData);
    }
    if ((isString || isFile) && !_extraData) {
        _extraData.reset(new TextExtraData);
    }

    _defaultValues.resize(_dimension);
    KnobSerialization::PerDimensionValueSerializationVec& values = _values["Main"];

    values.resize(_dimension);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i]._typeName = _typeName;
        values[i]._dimension = i;
        values[i]._serialization = this;
        ar & ::boost::serialization::make_nvp("item", values[i]);
    }

    // If the knob is a multi-line rich-text knob, parse the font since now font properties are not encoded in the text directly
    if (isString && values.size() > 0) {
        QString str = QString::fromUtf8(values[0]._value.isString.c_str());

        bool italicActivated = false;
        bool boldActivated = false;
        QString fontSizeString;
        QString fontColorString;
        QString fontFamily;

        // Find italic
        QString toFind = QString::fromUtf8(kItalicStartTag);
        int foundItalic = str.indexOf(toFind);
        int i = foundItalic;
        if (i != -1) {
            italicActivated = true;
        } else {
            italicActivated = false;
        }

        // Find bold
        toFind = QString::fromUtf8(kBoldStartTag);
        int foundBold = str.indexOf(toFind);
        i = foundBold;
        if (i != -1) {
            boldActivated = true;
        } else {
            boldActivated = false;
        }

        // Find size
        toFind = QString::fromUtf8(kFontSizeTag);
        int foundSize = str.indexOf(toFind);
        i = foundSize;
        if (i != -1) {
            i += toFind.size();
            while ( i < str.size() && str.at(i).isDigit() ) {
                fontSizeString.append( str.at(i) );
                ++i;
            }
        }

        // Find color
        toFind = QString::fromUtf8(kFontColorTag);
        int foundColor = str.indexOf(toFind, i);
        i = foundColor;
        if (i != -1) {
            i += toFind.size();
            while ( i < str.size() && str.at(i) != QLatin1Char('"') ) {
                fontColorString.append( str.at(i) );
                ++i;
            }
        }

        // Find family
        toFind = QString::fromUtf8(kFontFaceTag);
        int foundFamily = str.indexOf(toFind, i);
        i = foundFamily;
        if (i != -1) {
            i += toFind.size();
            while ( i < str.size() && str.at(i) != QLatin1Char('"') ) {
                fontFamily.append( str.at(i) );
                ++i;
            }
        }

        TextExtraData* data = dynamic_cast<TextExtraData*>(_extraData.get());
        if (!fontFamily.isEmpty()) {
            data->fontFamily = fontFamily.toStdString();
        }
        if (!fontSizeString.isEmpty()) {
            data->fontSize = fontSizeString.toInt();
        }
        if (!fontColorString.isEmpty()) {
            int r,g,b;
            NATRON_NAMESPACE::ColorParser::parseColor(fontColorString, &r, &g, &b);
            data->fontColor[0] = r * (1. / 255);
            data->fontColor[1] = g * (1. / 255);
            data->fontColor[2] = b * (1. / 255);
        }
        data->boldActivated = boldActivated;
        data->italicActivated = italicActivated;

        str = NATRON_NAMESPACE::KnobString::removeAutoAddedHtmlTags(str);
        values[0]._value.isString = str.toStdString();
    }

    ////restore extra datas
    if (isParametric) {
        ParametricExtraData* extraData = dynamic_cast<ParametricExtraData*>(_extraData.get());
        std::list<NATRON_NAMESPACE::Curve> curves;
        ar & ::boost::serialization::make_nvp("ParametricCurves", curves);

        std::list<SERIALIZATION_NAMESPACE::CurveSerialization>& mainViewCurves = extraData->parametricCurves["Main"];
        for (std::list<NATRON_NAMESPACE::Curve>::iterator it = curves.begin(); it!=curves.end(); ++it) {
            CurveSerialization c;
            it->toSerialization(&c);
            mainViewCurves.push_back(c);
        }

        //isParametric->loadParametricCurves(extraData->parametricCurves);
    } else if (isString || isFile) {

        std::map<double, std::string> keys;
        ar & ::boost::serialization::make_nvp("StringsAnimation", keys);

        values[0]._animationCurve.curveType = eCurveSerializationTypeString;
        for (std::map<double, std::string>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
            KeyFrameSerialization k;
            k.time = it->first;

            KeyFrameProperty property;
            KeyFramePropertyVariant v;
            v.stringValue = it->second;
            property.values.push_back(v);
            property.name = kKeyFramePropString;
            property.type = kKeyFramePropertyVariantTypeString;
            k.properties.push_back(property);
            values[0]._animationCurve.keys.push_back(k);
        }
        ///Don't load animation for input image files: they no longer hold keyframes
        // in the Reader context, the script name must be kOfxImageEffectFileParamName, @see kOfxImageEffectContextReader
        /*if ( !isFile || ( isFile && (isFile->getName() != kOfxImageEffectFileParamName) ) ) {
         isStringAnimated->loadAnimation(extraDatas);
         }*/
    }

    // Dead serialization code, just for backward compatibility.
    if ( ( (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS) && (version < KNOB_SERIALIZATION_REMOVE_SLAVED_TRACKS) ) &&
        isDouble && ( _scriptName == "center") && ( _dimension == 2) ) {
        int count;
        ar & ::boost::serialization::make_nvp("SlavePtsNo", count);
        for (int i = 0; i < count; ++i) {
            std::string rotoNodeName, bezierName;
            int cpIndex;
            bool isFeather;
            int offsetTime;
            ar & ::boost::serialization::make_nvp("SlavePtNodeName", rotoNodeName);
            ar & ::boost::serialization::make_nvp("SlavePtBezier", bezierName);
            ar & ::boost::serialization::make_nvp("SlavePtIndex", cpIndex);
            ar & ::boost::serialization::make_nvp("SlavePtIsFeather", isFeather);
            if (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS_OFFSET) {
                ar & ::boost::serialization::make_nvp("OffsetTime", offsetTime);
            }
        }
    }

    if (version >= KNOB_SERIALIZATION_INTRODUCES_USER_KNOB) {
        if (isChoice) {
            std::string stringChoice;
            ar & ::boost::serialization::make_nvp("ChoiceLabel", stringChoice);

            /*if (version < KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION) {
                // In Natron 2.2.3 we changed the encoding of planes: they no longer are planeLabel + "." + channels
                // but planeID + "." + channels
                // Hard-code the mapping
                Compat::checkForPreNatron226String(&stringChoice);
            }*/
            //_extraData = cData;

            values[0]._value.isString = stringChoice;
        }


        ar & ::boost::serialization::make_nvp("UserKnob", _isUserKnob);
        if (_isUserKnob) {
            ar & ::boost::serialization::make_nvp("Label", _label);
            ar & ::boost::serialization::make_nvp("Help", _tooltip);
            ar & ::boost::serialization::make_nvp("NewLine", _triggerNewLine);
            ar & ::boost::serialization::make_nvp("Evaluate", _evaluatesOnChange);

            {
                bool animationEnabled;
                ar & ::boost::serialization::make_nvp("Animates", animationEnabled);

                bool animationEnabledByDefault = isInt || isDouble || isColor;
                if ((animationEnabledByDefault && !animationEnabled) ||
                    (!animationEnabledByDefault && animationEnabled)) {
                    _animatesChanged = true;
                }
            }
            ar & ::boost::serialization::make_nvp("Persistent", _isPersistent);
            if (version >= KNOB_SERIALIZATION_INTRODUCE_USER_KNOB_ICON_FILE_PATH) {
                ar & ::boost::serialization::make_nvp("UncheckedIcon", _iconFilePath[0]);
                ar & ::boost::serialization::make_nvp("CheckedIcon", _iconFilePath[1]);
            }
            if (isChoice) {
                assert(_extraData);
                ChoiceExtraData* data = dynamic_cast<ChoiceExtraData*>(_extraData.get());
                assert(data);
                ar & ::boost::serialization::make_nvp("Entries", data->_entries);
                if (version >= KNOB_SERIALIZATION_INTRODUCES_CHOICE_HELP_STRINGS) {
                    ar & ::boost::serialization::make_nvp("Helps", data->_helpStrings);
                }
            }

            if (isString) {
                TextExtraData* tdata = dynamic_cast<TextExtraData*>(_extraData.get());
                assert(tdata);
                ar & ::boost::serialization::make_nvp("IsLabel", tdata->label);
                ar & ::boost::serialization::make_nvp("IsMultiLine", tdata->multiLine);
                ar & ::boost::serialization::make_nvp("UseRichText", tdata->richText);
            }
            if (isDouble || isInt || isColor) {
                ValueExtraData* extraData = new ValueExtraData;
                ar & ::boost::serialization::make_nvp("Min", extraData->min);
                ar & ::boost::serialization::make_nvp("Max", extraData->max);
                if (version >= KNOB_SERIALIZATION_INTRODUCES_DISPLAY_MIN_MAX) {
                    ar & ::boost::serialization::make_nvp("DMin", extraData->dmin);
                    ar & ::boost::serialization::make_nvp("DMax", extraData->dmax);
                }
                _extraData.reset(extraData);
            }

            if (isFile || isOutFile) {
                FileExtraData* extraData = new FileExtraData;
                ar & ::boost::serialization::make_nvp("Sequences", extraData->useSequences);
                _extraData.reset(extraData);
            }

            if (isPath) {
                PathExtraData* extraData = new PathExtraData;
                ar & ::boost::serialization::make_nvp("MultiPath", extraData->multiPath);
                _extraData.reset(extraData);
            }

            if ( isDouble &&
                ((version >= KNOB_SERIALIZATION_INTRODUCES_NATIVE_OVERLAYS) && (_dimension == 2)) ) {
                    ValueExtraData* extraData = dynamic_cast<ValueExtraData*>(_extraData.get());
                    assert(extraData);
                    bool useHostOverlayHandle;
                    ar & ::boost::serialization::make_nvp("HasOverlayHandle", useHostOverlayHandle);
                }

            // Dead serialization code, just for backward compat.
            if (version >= KNOB_SERIALIZATION_INTRODUCES_DEFAULT_VALUES && version < KNOB_SERIALIZATION_REMOVE_DEFAULT_VALUES) {

                bool isDoubleVal = isDouble || isColor;
                bool isIntVal = isChoice || isInt;
                bool isStrVal = isFile || isOutFile || isPath || isString;

                for (int i = 0; i < _dimension; ++i) {
                    if (isDoubleVal) {
                        double def;
                        ar & ::boost::serialization::make_nvp("DefaultValue", def);
                    } else if (isIntVal) {
                        int def;
                        ar & ::boost::serialization::make_nvp("DefaultValue", def);
                    } else if (isBool) {
                        bool def;
                        ar & ::boost::serialization::make_nvp("DefaultValue", def);
                    } else if (isStrVal) {
                        std::string def;
                        ar & ::boost::serialization::make_nvp("DefaultValue", def);
                    }
                }
            }
        }
    }

}


template<class Archive>
void
SERIALIZATION_NAMESPACE::GroupKnobSerialization::serialize(Archive & ar, const unsigned int version)
{
    if (version >= GROUP_KNOB_SERIALIZATION_INTRODUCES_TYPENAME) {
        ar & ::boost::serialization::make_nvp("TypeName", _typeName);
    }
    ar & ::boost::serialization::make_nvp("Name", _name);
    ar & ::boost::serialization::make_nvp("Label", _label);
    ar & ::boost::serialization::make_nvp("Secret", _secret);
    ar & ::boost::serialization::make_nvp("IsTab", _isSetAsTab);
    ar & ::boost::serialization::make_nvp("IsOpened", _isOpened);
    int nbChildren;
    ar & ::boost::serialization::make_nvp("NbChildren", nbChildren);
    for (int i = 0; i < nbChildren; ++i) {
        std::string type;
        ar & ::boost::serialization::make_nvp("Type", type);

        if (type == "Group") {
            boost::shared_ptr<GroupKnobSerialization> knob = boost::make_shared<GroupKnobSerialization>();
            ar & ::boost::serialization::make_nvp("item", *knob);
            _children.push_back(knob);
        } else {
            KnobSerializationPtr knob = boost::make_shared<KnobSerialization>();
            ar & ::boost::serialization::make_nvp("item", *knob);
            _children.push_back(knob);
        }
    }
}

inline void addUserKnobToRegularKnobsRecursive(const SERIALIZATION_NAMESPACE::GroupKnobSerialization& k, SERIALIZATION_NAMESPACE::NodeSerialization* node)
{
    for (std::list <boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobSerializationBase> >::const_iterator it = k._children.begin(); it!=k._children.end(); ++it) {
        SERIALIZATION_NAMESPACE::GroupKnobSerialization* isGroup = dynamic_cast<SERIALIZATION_NAMESPACE::GroupKnobSerialization*>(it->get());
        if (isGroup) {
            addUserKnobToRegularKnobsRecursive(*isGroup, node);
        } else {
            SERIALIZATION_NAMESPACE::KnobSerializationPtr isKnob = boost::dynamic_pointer_cast<SERIALIZATION_NAMESPACE::KnobSerialization>(*it);
            assert(isKnob);
            if (isKnob) {
                node->_knobsValues.push_back(isKnob);
            }
        }
    }
}



template<class Archive>
void
SERIALIZATION_NAMESPACE::NodeSerialization::serialize(Archive & ar,
          const unsigned int version)
{

    if (version > NODE_SERIALIZATION_CURRENT_VERSION) {
        throw std::invalid_argument("The project you're trying to load contains data produced by a more recent "
                                    "version of Natron, which makes it unreadable");
    }

    ar & ::boost::serialization::make_nvp("Plugin_label", _nodeLabel);

    if (version >= NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
        ar & ::boost::serialization::make_nvp("Plugin_script_name", _nodeScriptName);
    } else {
        _nodeScriptName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_nodeLabel);
    }
    ar & ::boost::serialization::make_nvp("Plugin_id", _pluginID);

    std::string pythonModule;
    if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE) {
        if ( (version >= NODE_SERIALIZATION_SERIALIZE_PYTHON_MODULE_ALWAYS) || (_pluginID == PLUGINID_NATRON_GROUP) ) {
            ar & ::boost::serialization::make_nvp("PythonModule", pythonModule);
            if (version >= NODE_SERIALIZATION_INTRODUCES_PYTHON_MODULE_VERSION) {
                int pythonModuleVersion;
                ar & ::boost::serialization::make_nvp("PythonModuleVersion", pythonModuleVersion);
            }
        }
    }

    ar & ::boost::serialization::make_nvp("Plugin_major_version", _pluginMajorVersion);
    ar & ::boost::serialization::make_nvp("Plugin_minor_version", _pluginMinorVersion);
    int nbKnobs;
    ar & ::boost::serialization::make_nvp("KnobsCount", nbKnobs);
    for (int i = 0; i < nbKnobs; ++i) {
        KnobSerializationPtr ks = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("item", *ks);
        _knobsValues.push_back(ks);
    }

    // Before Natron 2.2.6 we used to have a string parameter for each dynamic choice parameter.
    // This has been removed, hence handle backward compat by trying to find such string parameter serialization
    for (SERIALIZATION_NAMESPACE::KnobSerializationList::iterator it = _knobsValues.begin(); it != _knobsValues.end(); ++it) {
        if ((*it)->_typeName != NATRON_NAMESPACE::KnobChoice::typeNameStatic()) {
            continue;
        }

        std::string stringParamName = (*it)->_scriptName + "Choice";
        for (SERIALIZATION_NAMESPACE::KnobSerializationList::iterator it2 = _knobsValues.begin(); it2 != _knobsValues.end(); ++it2) {
            if ( (*it2)->_scriptName == stringParamName && (*it2)->_dataType == SERIALIZATION_NAMESPACE::eSerializationValueVariantTypeString) {

                const SERIALIZATION_NAMESPACE::KnobSerialization::PerDimensionValueSerializationVec& perDimValues = (*it2)->_values["Main"];
                if (perDimValues.size() > 0) {

                    SERIALIZATION_NAMESPACE::KnobSerialization::PerDimensionValueSerializationVec& choiceDimValues = (*it)->_values["Main"];
                    choiceDimValues.resize(1);
                    choiceDimValues[0]._value.isString = perDimValues[0]._value.isString;

                    /*if ((*it)->_boostSerializationVersion < KNOB_SERIALIZATION_CHANGE_PLANES_SERIALIZATION) {
                        Compat::checkForPreNatron226String(&choiceDimValues[0]._value.isString);
                    }*/
                }
                
                break;
            }
        }
    }

    if (version < NODE_SERIALIZATION_CHANGE_INPUTS_SERIALIZATION) {
        std::vector<std::string> oldInputs;
        ar & ::boost::serialization::make_nvp("Inputs_map", oldInputs);
        if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
            for (std::size_t i = 0; i < oldInputs.size(); ++i) {
                oldInputs[i] = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(oldInputs[i]);
                std::stringstream ss;
                // Prior to Natron 2, inputs were reversed
                ss << oldInputs.size() - 1 - i;
                _inputs[ss.str()] = oldInputs[i];
            }
        }
    } else {
        ar & ::boost::serialization::make_nvp("Inputs_map", _inputs);

        // The Dot plug-in was serialized with an empty input. If empty replace it with "0"
        std::map<std::string, std::string>::iterator foundEmptyInput = _inputs.find("");
        if (foundEmptyInput != _inputs.end()) {
            std::string v = foundEmptyInput->second;
            _inputs.erase(foundEmptyInput);
            _inputs.insert(std::make_pair("0", v));

        }
    }

    U64 knobsAge;
    ar & ::boost::serialization::make_nvp("KnobsAge", knobsAge);
    ar & ::boost::serialization::make_nvp("MasterNode", _masterNodecriptName);
    if (version < NODE_SERIALIZATION_INTRODUCES_SCRIPT_NAME) {
        _masterNodecriptName = NATRON_NAMESPACE::NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(_masterNodecriptName);
    }

    if (version >= NODE_SERIALIZATION_V_INTRODUCES_ROTO) {
        bool hasRotoContext;
        ar & ::boost::serialization::make_nvp("HasRotoContext", hasRotoContext);
        if (hasRotoContext) {
            Compat::RotoContextSerialization rotoContext;
            KnobItemsTableSerializationPtr tableModel = boost::make_shared<KnobItemsTableSerialization>();
            ar & ::boost::serialization::make_nvp("RotoContext", rotoContext);
            rotoContext.convertRotoContext(tableModel.get());
            _tables.push_back(tableModel);
        }

        if (version >= NODE_SERIALIZATION_INTRODUCES_TRACKER_CONTEXT) {
            bool hasTrackerContext;
            ar & boost::serialization::make_nvp("HasTrackerContext", hasTrackerContext);
            if (hasTrackerContext) {

                Compat::TrackerContextSerialization trackerContext;
                KnobItemsTableSerializationPtr tableModel = boost::make_shared<KnobItemsTableSerialization>();
                ar & boost::serialization::make_nvp("TrackerContext", trackerContext);
                trackerContext.convertTrackerContext(tableModel.get());
                _tables.push_back(tableModel);
            }
        }
    }
    if (version >= NODE_SERIALIZATION_INTRODUCES_MULTI_INSTANCE) {
        std::string multiInstanceParentName;
        ar & ::boost::serialization::make_nvp("MultiInstanceParent", multiInstanceParentName);
    }

    if (version >= NODE_SERIALIZATION_INTRODUCES_USER_KNOBS) {
        int userPagesCount;
        ar & ::boost::serialization::make_nvp("UserPagesCount", userPagesCount);
        for (int i = 0; i < userPagesCount; ++i) {
            boost::shared_ptr<GroupKnobSerialization> s( new GroupKnobSerialization() );
            ar & ::boost::serialization::make_nvp("item", *s);

            addUserKnobToRegularKnobsRecursive(*s, this);
            
        }
        if (version >= NODE_SERIALIZATION_SERIALIZE_PAGE_INDEX) {
            int count;
            ar & ::boost::serialization::make_nvp("PagesCount", count);
            for (int i = 0; i < count; ++i) {
                std::string name;
                ar & ::boost::serialization::make_nvp("name", name);
                _pagesIndexes.push_back(name);
            }
        } else {
            _pagesIndexes.push_back(NATRON_USER_MANAGED_KNOBS_PAGE);
        }
    }

    if (version >= NODE_SERIALIZATION_INTRODUCES_GROUPS) {
        int nodesCount;
        ar & ::boost::serialization::make_nvp("Children", nodesCount);

        for (int i = 0; i < nodesCount; ++i) {
            NodeSerializationPtr s = boost::make_shared<NodeSerialization>();
            ar & ::boost::serialization::make_nvp("item", *s);
            _children.push_back(s);
        }
    }
    if (version >= NODE_SERIALIZATION_INTRODUCES_USER_COMPONENTS) {
        std::list<SERIALIZATION_NAMESPACE::Compat::ImagePlaneDescSerialization> comps;
        ar & ::boost::serialization::make_nvp("UserComponents", comps);
        for (std::list<SERIALIZATION_NAMESPACE::Compat::ImagePlaneDescSerialization>::iterator it = comps.begin(); it!=comps.end(); ++it) {
            SERIALIZATION_NAMESPACE::ImagePlaneDescSerialization s;
            s.planeID = it->_planeID;
            s.planeLabel = it->_planeLabel;
            s.channelsLabel = it->_channelsLabel;
            s.channelNames = it->_channels;
            _userComponents.push_back(s);
        }

    }
    if (version >= NODE_SERIALIZATION_INTRODUCES_CACHE_ID) {
        std::string cacheID;
        ar & ::boost::serialization::make_nvp("CacheID", cacheID);
    }

}


template<class Archive>
void
SERIALIZATION_NAMESPACE::Compat::TrackSerialization::serialize(Archive & ar,
          const unsigned int version)
{
    if (version >= TRACK_SERIALIZATION_ADD_TRACKER_PM) {
        ar & boost::serialization::make_nvp("IsTrackerPM", _isPM);
    }

    ar & boost::serialization::make_nvp("Enabled", _enabled);
    ar & boost::serialization::make_nvp("ScriptName", _scriptName);
    ar & boost::serialization::make_nvp("Label", _label);
    int nbItems;
    ar & boost::serialization::make_nvp("NbItems", nbItems);
    for (int i = 0; i < nbItems; ++i) {
        KnobSerializationPtr s( new KnobSerialization() );
        ar & boost::serialization::make_nvp("Item", *s);
        _knobs.push_back(s);
    }
    int nbUserKeys;
    ar & boost::serialization::make_nvp("NbUserKeys", nbUserKeys);
    for (int i = 0; i < nbUserKeys; ++i) {
        int key;
        ar & boost::serialization::make_nvp("UserKeys", key);
        _userKeys.push_back(key);
    }
}

template<class Archive>
void SERIALIZATION_NAMESPACE::Compat::TrackerContextSerialization::serialize(Archive & ar,
          const unsigned int /*version*/)
{
    int nbItems;
    ar & boost::serialization::make_nvp("NbItems", nbItems);
    for (int i = 0; i < nbItems; ++i) {
        Compat::TrackSerialization s;
        ar & boost::serialization::make_nvp("Item", s);
        _tracks.push_back(s);
    }
}


template<class Archive>
void
SERIALIZATION_NAMESPACE::ProjectSerialization::serialize(Archive & ar,
          const unsigned int version)
{

    // Dead serialization code
    if (version >= PROJECT_SERIALIZATION_INTRODUCES_NATRON_VERSION && version < PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
        std::string natronVersion;
        ar & ::boost::serialization::make_nvp("NatronVersion", natronVersion);
        std::string toFind(NATRON_APPLICATION_NAME " v");
        std::size_t foundV = natronVersion.find(toFind);
        if (foundV != std::string::npos) {
            foundV += toFind.size();
            toFind = " from git branch";
            std::size_t foundFrom = natronVersion.find(toFind);
            if (foundFrom != std::string::npos) {
                std::string vStr;
                for (std::size_t i = foundV; i < foundFrom; ++i) {
                    vStr.push_back(natronVersion[i]);
                }
                QStringList splits = QString::fromUtf8( vStr.c_str() ).split( QLatin1Char('.') );
                if (splits.size() == 3) {
                    _projectLoadedInfo.vMajor = splits[0].toInt();
                    _projectLoadedInfo.vMinor = splits[1].toInt();
                    _projectLoadedInfo.vRev = splits[2].toInt();
                }
            }
        }
    }

    if (version >= PROJECT_SERIALIZATION_CHANGE_VERSION_SERIALIZATION) {
        ar & ::boost::serialization::make_nvp("VersionMajor", _projectLoadedInfo.vMajor);
        ar & ::boost::serialization::make_nvp("VersionMinor", _projectLoadedInfo.vMinor);
        ar & ::boost::serialization::make_nvp("VersionRev", _projectLoadedInfo.vRev);
        ar & ::boost::serialization::make_nvp("GitBranch", _projectLoadedInfo.gitBranch);
        ar & ::boost::serialization::make_nvp("GitCommit", _projectLoadedInfo.gitCommit);
        ar & ::boost::serialization::make_nvp("OS", _projectLoadedInfo.osStr);
        ar & ::boost::serialization::make_nvp("Bits", _projectLoadedInfo.bits);

    }

    if (version < PROJECT_SERIALIZATION_INTRODUCES_GROUPS) {
        int nodesCount;
        ar & ::boost::serialization::make_nvp("NodesCount", nodesCount);
        for (int i = 0; i < nodesCount; ++i) {
            NodeSerializationPtr ns = boost::make_shared<NodeSerialization>();
            ar & ::boost::serialization::make_nvp("item", *ns);
            _nodes.push_back(ns);
        }
    } else {
        Compat::NodeCollectionSerialization nodes;
        ar & ::boost::serialization::make_nvp("NodesCollection", nodes);
        _nodes = nodes.getNodesSerialization();
    }

    int knobsCount;
    ar & ::boost::serialization::make_nvp("ProjectKnobsCount", knobsCount);

    for (int i = 0; i < knobsCount; ++i) {
        KnobSerializationPtr ks = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("item", *ks);
        _projectKnobs.push_back(ks);
    }

    std::list<NATRON_NAMESPACE::Format> formats;
    ar & ::boost::serialization::make_nvp("AdditionalFormats", formats);
    for (std::list<NATRON_NAMESPACE::Format>::iterator it = formats.begin(); it!=formats.end(); ++it) {
        FormatSerialization s;
        s.x1 = it->x1;
        s.y1 = it->y1;
        s.x2 = it->x2;
        s.y2 = it->y2;
        s.par = it->getPixelAspectRatio();
        s.name = it->getName();
        _additionalFormats.push_back(s);
    }


    ar & ::boost::serialization::make_nvp("Timeline_current_time", _timelineCurrent);
    if (version < PROJECT_SERIALIZATION_REMOVES_TIMELINE_BOUNDS) {
        NATRON_NAMESPACE::SequenceTime left, right;
        ar & ::boost::serialization::make_nvp("Timeline_left_bound", left);
        ar & ::boost::serialization::make_nvp("Timeline_right_bound", right);
    }
    if (version < PROJECT_SERIALIZATION_REMOVES_NODE_COUNTERS) {
        std::map<std::string, int> _nodeCounters;
        ar & ::boost::serialization::make_nvp("NodeCounters", _nodeCounters);
    }

    long long creationDate;
    ar & ::boost::serialization::make_nvp("CreationDate", creationDate);

}

template<class Archive>
void SERIALIZATION_NAMESPACE::PythonPanelSerialization::serialize(Archive & ar,
          const unsigned int /*version*/)
{
    ar & ::boost::serialization::make_nvp("Label", name);
    ar & ::boost::serialization::make_nvp("PythonFunction", pythonFunction);
    int nKnobs;
    ar & ::boost::serialization::make_nvp("NumParams", nKnobs);

    for (int i = 0; i < nKnobs; ++i) {
        KnobSerializationPtr k = boost::make_shared<KnobSerialization>();
        ar & ::boost::serialization::make_nvp("item", *k);
        knobs.push_back(k);
    }

    ar & ::boost::serialization::make_nvp("UserData", userData);
}

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RectD);
BOOST_SERIALIZATION_ASSUME_ABSTRACT(SERIALIZATION_NAMESPACE::RectI);
BOOST_CLASS_VERSION(NATRON_NAMESPACE::Format, FORMAT_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(NATRON_NAMESPACE::BezierCP, BEZIER_CP_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::BezierSerialization, BEZIER_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::KnobSerialization, KNOB_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ValueSerialization, VALUE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::MasterSerialization, MASTER_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::GroupKnobSerialization, GROUP_KNOB_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::NodeSerialization, NODE_SERIALIZATION_CURRENT_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::ProjectSerialization, PROJECT_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::RotoDrawableItemSerialization, ROTO_DRAWABLE_ITEM_VERSION)
BOOST_SERIALIZATION_ASSUME_ABSTRACT(SERIALIZATION_NAMESPACE::Compat::RotoDrawableItemSerialization);
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::RotoItemSerialization, ROTO_ITEM_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::RotoLayerSerialization, ROTO_LAYER_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::RotoStrokeItemSerialization, ROTO_STROKE_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::TrackSerialization, TRACK_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::TrackerContextSerialization, TRACKER_CONTEXT_SERIALIZATION_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::RotoContextSerialization, ROTO_CTX_VERSION)
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::Compat::ImagePlaneDescSerialization, IMAGEPLANEDESC_SERIALIZATION_VERSION)

#endif // NATRON_BOOST_SERIALIZATION_COMPAT

#endif // SERIALIZATIONCOMPAT_H
