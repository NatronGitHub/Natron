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

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/list.hpp>
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

class RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoItem;
    
public:
    
    RotoItemSerialization()
    : name()
    , activated(false)
    , parentLayerName()
    , locked(false)
    {
        
    }
    
    virtual ~RotoItemSerialization() {}
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("Name",name);
        ar & boost::serialization::make_nvp("Activated",activated);
        ar & boost::serialization::make_nvp("ParentLayer",parentLayerName);
        ar & boost::serialization::make_nvp("Locked",locked);
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    std::string name;
    bool activated;
    std::string parentLayerName;
    bool locked;
    
};

//BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoItemSerialization);

class RotoDrawableItemSerialization : public RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoDrawableItem;
    
public:
    
    RotoDrawableItemSerialization()
    : RotoItemSerialization()
    {
        
    }
    
    virtual ~RotoDrawableItemSerialization() {}
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
                                                                                                      static_cast<RotoDrawableItemSerialization *>(NULL),
                                                                                                      static_cast<RotoItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
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
        boost::serialization::void_cast_register<RotoDrawableItemSerialization,RotoItemSerialization>(
                                                                                                      static_cast<RotoDrawableItemSerialization *>(NULL),
                                                                                                      static_cast<RotoItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        
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
    
    KnobSerialization _activated;
    KnobSerialization _opacity;
    KnobSerialization _feather;
    KnobSerialization _featherFallOff;
    KnobSerialization _inverted;
    double _overlayColor[4];
    
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(RotoDrawableItemSerialization);

class BezierSerialization : public RotoDrawableItemSerialization
{
    friend class boost::serialization::access;
    friend class Bezier;
    
public:
    
    BezierSerialization()
    : RotoDrawableItemSerialization()
    {
        
    }
    
    virtual ~BezierSerialization() {}
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        boost::serialization::void_cast_register<BezierSerialization,RotoDrawableItemSerialization>(
                                                                                        static_cast<BezierSerialization *>(NULL),
                                                                                        static_cast<RotoDrawableItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
        assert(_controlPoints.size() == _featherPoints.size());
        int numPoints = (int)_controlPoints.size();
        ar & boost::serialization::make_nvp("NumPoints",numPoints);
        
        std::list< BezierCP >::const_iterator itF = _featherPoints.begin();
        for (std::list< BezierCP >::const_iterator it = _controlPoints.begin(); it!= _controlPoints.end();++it,++itF) {
            ar & boost::serialization::make_nvp("CP",*it);
            ar & boost::serialization::make_nvp("FP",*itF);
        }
        ar & boost::serialization::make_nvp("Closed",_closed);
        
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        boost::serialization::void_cast_register<BezierSerialization,RotoDrawableItemSerialization>(
                                                                                                    static_cast<BezierSerialization *>(NULL),
                                                                                                    static_cast<RotoDrawableItemSerialization *>(NULL)
                                                                                                    );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoDrawableItemSerialization);
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
        
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    std::list< BezierCP > _controlPoints,_featherPoints;
    bool _closed;
    
};


class RotoLayerSerialization : public RotoItemSerialization
{
    friend class boost::serialization::access;
    friend class RotoLayer;
    
public:
    
    RotoLayerSerialization()
    : RotoItemSerialization()
    {
        
    }
        
    virtual ~RotoLayerSerialization()
    {
     
    }
    
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
        
        boost::serialization::void_cast_register<RotoLayerSerialization,RotoItemSerialization>(
                                                            static_cast<RotoLayerSerialization *>(NULL),
                                                            static_cast<RotoItemSerialization *>(NULL)
                                                                                                      );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        int numChildren = (int)children.size();
        ar & boost::serialization::make_nvp("NumChildren",numChildren);
        for (std::list < boost::shared_ptr<RotoItemSerialization> >::const_iterator it = children.begin() ; it!=children.end(); ++it) {
            BezierSerialization* isBezier = dynamic_cast<BezierSerialization*>(it->get());
            RotoLayerSerialization* isLayer = dynamic_cast<RotoLayerSerialization*>(it->get());
            bool bezier = isBezier != NULL;
            ar & boost::serialization::make_nvp("IsBezier",bezier);
            if (isBezier) {
                ar & boost::serialization::make_nvp("Item",*isBezier);
            } else {
                assert(isLayer);
                ar & boost::serialization::make_nvp("Item",*isLayer);
            }
        }
        
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        boost::serialization::void_cast_register<RotoLayerSerialization,RotoItemSerialization>(
                                                                    static_cast<RotoLayerSerialization *>(NULL),
                                                                    static_cast<RotoItemSerialization *>(NULL)
                                                                                                            );
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RotoItemSerialization);
        
        int numChildren;
        ar & boost::serialization::make_nvp("NumChildren",numChildren);
        for (int i = 0; i < numChildren ;++i)
        {
            bool bezier;
            ar & boost::serialization::make_nvp("IsBezier",bezier);
            if (bezier) {
                boost::shared_ptr<BezierSerialization> b(new BezierSerialization);
                ar & boost::serialization::make_nvp("Item",*b);
                children.push_back(b);
            } else {
                boost::shared_ptr<RotoLayerSerialization> l(new RotoLayerSerialization);
                ar & boost::serialization::make_nvp("Item",*l);
                children.push_back(l);
            }
        }
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    std::list < boost::shared_ptr<RotoItemSerialization> > children;
    
};


    

class RotoContextSerialization
{
    friend class boost::serialization::access;
    friend class RotoContext;
    
public:
    
    RotoContextSerialization()
    : _baseLayer()
    {
        
    }
    
    ~RotoContextSerialization()
    {
    }
    
private:
    
    
    template<class Archive>
    void save(Archive & ar, const unsigned int version) const
    {
        (void)version;
    
        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
        ar & boost::serialization::make_nvp("Counters",_itemCounters);
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        (void)version;
        
        ar & boost::serialization::make_nvp("BaseLayer",_baseLayer);
        ar & boost::serialization::make_nvp("AutoKeying",_autoKeying);
        ar & boost::serialization::make_nvp("RippleEdit",_rippleEdit);
        ar & boost::serialization::make_nvp("FeatherLink",_featherLink);
        ar & boost::serialization::make_nvp("Selection",_selectedItems);
        ar & boost::serialization::make_nvp("Counters",_itemCounters);
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
    
    RotoLayerSerialization  _baseLayer;
    std::list< std::string > _selectedItems;
    bool _autoKeying;
    bool _rippleEdit;
    bool _featherLink;
    std::map<std::string,int> _itemCounters;
};

#endif // ROTOSERIALIZATION_H
