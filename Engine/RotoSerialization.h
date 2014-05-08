//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/
#ifndef ROTOSERIALIZATION_H
#define ROTOSERIALIZATION_H

#include "Engine/RotoContext.h"
#include "Engine/RotoContextPrivate.h"

#include <boost/serialization/split_member.hpp>
#include "Engine/KnobSerialization.h"

template<class Archive>
void BezierCP::serialize(Archive & ar, const unsigned int version)
{
    (void)version;
    ar & boost::serialization::make_nvp("X",_imp->x);
    ar & boost::serialization::make_nvp("X_animation",_imp->curveX);
    ar & boost::serialization::make_nvp("Y",_imp->curveY);
    ar & boost::serialization::make_nvp("Y_animation",_imp->curveY);
    ar & boost::serialization::make_nvp("Left_X",_imp->leftX);
    ar & boost::serialization::make_nvp("Left_X_animation",_imp->curveLeftBezierX);
    ar & boost::serialization::make_nvp("Left_Y",_imp->leftY);
    ar & boost::serialization::make_nvp("Left_Y_animation",_imp->curveLeftBezierY);
    ar & boost::serialization::make_nvp("Right_X",_imp->rightX);
    ar & boost::serialization::make_nvp("Right_X_animation",_imp->curveRightBezierX);
    ar & boost::serialization::make_nvp("Right_Y",_imp->rightY);
    ar & boost::serialization::make_nvp("Right_Y_animation",_imp->curveRightBezierY);
}

class BezierSerialization
{
    friend class boost::serialization::access;
    friend class Bezier;
    
public:
    
    BezierSerialization()
    {
        
    }
    
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        assert(_controlPoints.size() == _featherPoints.size());
        int numPoints = (int)_controlPoints.size();
        ar & boost::serialization::make_nvp("NumPoints",numPoints);
        
        std::list< BezierCP >::const_iterator itF = _featherPoints.begin();
        for (std::list< BezierCP >::const_iterator it = _controlPoints.begin(); it!= _controlPoints.end();++it,++itF) {
            ar & boost::serialization::make_nvp("CP",*it);
            ar & boost::serialization::make_nvp("FP",*itF);
        }
        ar & boost::serialization::make_nvp("Closed",_closed);
        ar & boost::serialization::make_nvp("Activated",_activated);
        ar & boost::serialization::make_nvp("Opacity",_opacity);
        ar & boost::serialization::make_nvp("Feather",_feather);
        ar & boost::serialization::make_nvp("FallOff",_featherFallOff);
        ar & boost::serialization::make_nvp("Inverted",_inverted);
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        int numPoints;
        ar & boost::serialization::make_nvp("NumPoints",numPoints);
        for (int i = 0; i < numPoints; ++i) {
            BezierCP cp,fp;
            ar & boost::serialization::make_nvp("CP",cp);
            ar & boost::serialization::make_nvp("FP",fp);
            _controlPoints.push_back(cp);
            _featherPoints.push_back(fp);
        }
        ar & boost::serialization::make_nvp("Closed",_closed);
        ar & boost::serialization::make_nvp("Activated",_activated);
        ar & boost::serialization::make_nvp("Opacity",_opacity);
        ar & boost::serialization::make_nvp("Feather",_feather);
        ar & boost::serialization::make_nvp("FallOff",_featherFallOff);
        ar & boost::serialization::make_nvp("Inverted",_inverted);
        ar & boost::serialization::make_nvp("OC.r",_overlayColor[0]);
        ar & boost::serialization::make_nvp("OC.g",_overlayColor[1]);
        ar & boost::serialization::make_nvp("OC.b",_overlayColor[2]);
        ar & boost::serialization::make_nvp("OC.a",_overlayColor[3]);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    std::list< BezierCP > _controlPoints,_featherPoints;
    bool _closed;
    KnobSerialization _activated;
    KnobSerialization _opacity;
    KnobSerialization _feather;
    KnobSerialization _featherFallOff;
    KnobSerialization _inverted;
    double _overlayColor[4];
};

class RotoContextSerialization
{
    friend class boost::serialization::access;
    friend class RotoContext;
    
public:
    
    RotoContextSerialization()
    {
        
    }
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        
        int numBez = (int)_beziers.size();
        ar & boost::serialization::make_nvp("NumBeziers",numBez);
        for (std::list<BezierSerialization>::const_iterator it = _beziers.begin(); it != _beziers.end();++it) {
            ar & boost::serialization::make_nvp("Bezier",*it);
        }
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        
        int numBez;
        ar & boost::serialization::make_nvp("NumBeziers",numBez);
        for (int i = 0; i < numBez; ++i) {
            BezierSerialization b;
            ar & boost::serialization::make_nvp("Bezier",b);
            _beziers.push_back(b);
        }
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    std::list< BezierSerialization > _beziers;
    bool _autoKeying;
    bool _rippleEdit;
    bool _featherLink;
};

#endif // ROTOSERIALIZATION_H
