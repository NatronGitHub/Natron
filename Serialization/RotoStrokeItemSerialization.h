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

#ifndef Engine_RotoStrokeItemSerialization_h
#define Engine_RotoStrokeItemSerialization_h

#include "Serialization/RotoDrawableItemSerialization.h"

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
#define ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES 2
#define ROTO_STROKE_SERIALIZATION_VERSION ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES
#endif

// Corresponds to RotoStrokeType enum
enum RotoStrokeType
{
    eRotoStrokeTypeSolid,
    eRotoStrokeTypeEraser,
    eRotoStrokeTypeClone,
    eRotoStrokeTypeReveal,
    eRotoStrokeTypeBlur,
    eRotoStrokeTypeSharpen,
    eRotoStrokeTypeSmear,
    eRotoStrokeTypeDodge,
    eRotoStrokeTypeBurn,
};

#define kRotoStrokeItemSerializationBrushTypeSolid "Solid"
#define kRotoStrokeItemSerializationBrushTypeEraser "Eraser"
#define kRotoStrokeItemSerializationBrushTypeClone "Clone"
#define kRotoStrokeItemSerializationBrushTypeReveal "Reveal"
#define kRotoStrokeItemSerializationBrushTypeBlur "Blur"
#define kRotoStrokeItemSerializationBrushTypeSmear "Smear"
#define kRotoStrokeItemSerializationBrushTypeDodge "Dodge"
#define kRotoStrokeItemSerializationBrushTypeBurn "Burn"

SERIALIZATION_NAMESPACE_ENTER;

struct StrokePoint
{
    double x, y, pressure;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void serialize(Archive & ar,
                   const unsigned int /*version*/)
    {
        ar & ::boost::serialization::make_nvp("X", x);
        ar & ::boost::serialization::make_nvp("Y", y);
        ar & ::boost::serialization::make_nvp("Press", pressure);
    }
#endif
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

    virtual void encode(YAML_NAMESPACE::Emitter& em) const OVERRIDE;

    virtual void decode(const YAML_NAMESPACE::Node& node) OVERRIDE;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
    template<class Archive>
    void save(Archive & ar,
              const unsigned int /*version*/) const
    {
        throw std::runtime_error("Saving with boost is no longer supported");
    }

    template<class Archive>
    void load(Archive & ar,
              const unsigned int version)
    {
        Q_UNUSED(version);
        boost::serialization::void_cast_register<RotoStrokeItemSerialization, RotoDrawableItemSerialization>(
            static_cast<RotoStrokeItemSerialization *>(NULL),
            static_cast<RotoDrawableItemSerialization *>(NULL)
            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        if (version < ROTO_STROKE_INTRODUCES_MULTIPLE_STROKES) {
            ar & ::boost::serialization::make_nvp("BrushType", _brushType);
            NATRON_NAMESPACE::Curve x,y,p;
            ar & ::boost::serialization::make_nvp("CurveX", x);
            ar & ::boost::serialization::make_nvp("CurveY", y);
            ar & ::boost::serialization::make_nvp("CurveP", p);
            PointCurves subStroke;
            subStroke.x.reset(new CurveSerialization);
            subStroke.y.reset(new CurveSerialization);
            subStroke.pressure.reset(new CurveSerialization);
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
                subStroke.x.reset(new CurveSerialization);
                subStroke.y.reset(new CurveSerialization);
                subStroke.pressure.reset(new CurveSerialization);
                x.toSerialization(subStroke.x.get());
                y.toSerialization(subStroke.y.get());
                p.toSerialization(subStroke.pressure.get());
                _subStrokes.push_back(subStroke);
            }
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
#endif

};

SERIALIZATION_NAMESPACE_EXIT;

#ifdef NATRON_BOOST_SERIALIZATION_COMPAT
BOOST_CLASS_VERSION(SERIALIZATION_NAMESPACE::RotoStrokeItemSerialization, ROTO_STROKE_SERIALIZATION_VERSION)
#endif

#endif // Engine_RotoStrokeItemSerialization_h
