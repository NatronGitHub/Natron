/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "SerializationCompat.h"

#include "Serialization/KnobTableItemSerialization.h"
#include "Serialization/BezierSerialization.h"
#include "Serialization/RotoStrokeItemSerialization.h"

#include "Engine/RotoPaint.h"
#include "Engine/TrackerNode.h"

SERIALIZATION_NAMESPACE_ENTER

//namespace Compat {

void
Compat::RotoItemSerialization::convertRotoItemSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization)
{
    outSerialization->scriptName = name;
    outSerialization->label = label;
}

void
Compat::RotoDrawableItemSerialization::convertRotoDrawableItemSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization)
{
    Compat::RotoItemSerialization::convertRotoItemSerialization(outSerialization);
    outSerialization->knobs = _knobs;
} // convertRotoDrawableItemSerialization


void
Compat::RotoLayerSerialization::convertRotoLayerSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization)
{
    Compat::RotoItemSerialization::convertRotoItemSerialization(outSerialization);
    outSerialization->verbatimTag = kSerializationRotoGroupTag;
    for (std::list<boost::shared_ptr<Compat::RotoItemSerialization> >::const_iterator it = children.begin(); it != children.end(); ++it) {
        Compat::BezierSerialization* isBezier = dynamic_cast<Compat::BezierSerialization*>(it->get());
        Compat::RotoLayerSerialization* isLayer = dynamic_cast<Compat::RotoLayerSerialization*>(it->get());
        Compat::RotoStrokeItemSerialization* isStroke = dynamic_cast<Compat::RotoStrokeItemSerialization*>(it->get());
        if (isBezier) {
            SERIALIZATION_NAMESPACE::BezierSerializationPtr bezier(new SERIALIZATION_NAMESPACE::BezierSerialization(isBezier->_isOpenBezier));
            isBezier->convertBezierSerialization(bezier.get());
            outSerialization->children.push_back(bezier);
        } else if (isLayer) {
            SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr layer(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
            isLayer->convertRotoItemSerialization(layer.get());
            outSerialization->children.push_back(layer);
        } else if (isStroke) {
            SERIALIZATION_NAMESPACE::RotoStrokeItemSerializationPtr stroke(new SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization);
            isStroke->convertStrokeSerialization(stroke.get());
            outSerialization->children.push_back(stroke);
        }
    }
} // convertRotoLayerSerialization

void
Compat::BezierSerialization::convertBezierSerialization(SERIALIZATION_NAMESPACE::BezierSerialization* outSerialization)
{
    SERIALIZATION_NAMESPACE::BezierSerialization::Shape& s = outSerialization->_shapes["Main"];
    outSerialization->_isOpenBezier = _isOpenBezier;
    outSerialization->verbatimTag = _isOpenBezier ? kSerializationOpenedBezierTag : kSerializationClosedBezierTag;
    s.closed = _closed;
    for (std::list<ControlPoint>::const_iterator it = _controlPoints.begin(); it != _controlPoints.end(); ++it) {
        SERIALIZATION_NAMESPACE::BezierSerialization::ControlPoint c;
        c.featherPoint = it->featherPoint;
        c.innerPoint = it->innerPoint;
        s.controlPoints.push_back(c);
    }
    Compat::RotoDrawableItemSerialization::convertRotoItemSerialization(outSerialization);
} // convertBezierSerialization

void
Compat::RotoStrokeItemSerialization::convertStrokeSerialization(SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization* outSerialization)
{
    outSerialization->verbatimTag = _brushType;
    for (std::list<PointCurves>::const_iterator it = _subStrokes.begin(); it != _subStrokes.end(); ++it) {
        SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization::PointCurves p;
        p.pressure = it->pressure;
        p.x = it->x;
        p.y = it->y;
        outSerialization->_subStrokes.push_back(p);
    }
    Compat::RotoDrawableItemSerialization::convertRotoItemSerialization(outSerialization);
} // convertStrokeSerialization

void
Compat::TrackSerialization::convertTrackSerialization(SERIALIZATION_NAMESPACE::KnobTableItemSerialization* outSerialization)
{
    outSerialization->scriptName = _scriptName;
    outSerialization->label = _label;
    outSerialization->verbatimTag = kSerializationTrackTag;
    CurveSerialization& userKeys = outSerialization->animationCurves["Main"];
    for (std::list<int>::const_iterator it = _userKeys.begin(); it != _userKeys.end(); ++it) {
        KeyFrameSerialization k;
        k.time = *it;
        userKeys.keys.push_back(k);

    }
    outSerialization->knobs = _knobs;
} // convertTrackSerialization

void
Compat::RotoContextSerialization::convertRotoContext(SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* outSerialization)
{
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr layer(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
    _baseLayer.convertRotoLayerSerialization(layer.get());
    outSerialization->items.push_back(layer);
    outSerialization->tableIdentifier = kRotoPaintItemsTableName;
    
} // convertRotoContext

void
Compat::TrackerContextSerialization::convertTrackerContext(SERIALIZATION_NAMESPACE::KnobItemsTableSerialization* outSerialization)
{
    for (std::list<Compat::TrackSerialization>::iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
        boost::shared_ptr<SERIALIZATION_NAMESPACE::KnobTableItemSerialization> s(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
        it->convertTrackSerialization(s.get());
        outSerialization->items.push_back(s);
    }
    outSerialization->tableIdentifier = kTrackerParamTracksTableName;
} // convertTrackerContext

//} // namespace Compat

SERIALIZATION_NAMESPACE_EXIT
