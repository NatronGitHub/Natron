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

#ifndef KNOBSERIALIZATION_H
#define KNOBSERIALIZATION_H
#include <map>

#include "Global/Macros.h"
CLANG_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
CLANG_DIAG_ON(unused-parameter)
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>


#include "Engine/Variant.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/CurveSerialization.h"
#include "Engine/StringAnimationManager.h"

#define KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS 2
#define KNOB_SERIALIZATION_VERSION KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS

struct MasterSerialization
{
    int masterDimension;
    std::string masterNodeName;
    std::string masterKnobName;
    
    MasterSerialization()
    : masterDimension(-1)
    , masterNodeName()
    , masterKnobName()
    {
        
    }
    
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        (void)version;
        ar & boost::serialization::make_nvp("MasterDimension",masterDimension);
        ar & boost::serialization::make_nvp("MasterNodeName",masterNodeName);
        ar & boost::serialization::make_nvp("MasterKnobName",masterKnobName);
    }

};

struct ValueSerialization
{
    boost::shared_ptr<KnobI> _knob;
    int _dimension;
    MasterSerialization _master;
    
    ValueSerialization(const boost::shared_ptr<KnobI>& knob,int dimension,bool save);
    
    template<class Archive>
    void save(Archive & ar, const unsigned int /*version*/) const
    {
        boost::shared_ptr<Int_Knob> isInt = boost::dynamic_pointer_cast<Int_Knob>(_knob);
        boost::shared_ptr<Bool_Knob> isBool = boost::dynamic_pointer_cast<Bool_Knob>(_knob);
        boost::shared_ptr<Double_Knob> isDouble = boost::dynamic_pointer_cast<Double_Knob>(_knob);
        boost::shared_ptr<Choice_Knob> isChoice = boost::dynamic_pointer_cast<Choice_Knob>(_knob);
        boost::shared_ptr<String_Knob> isString = boost::dynamic_pointer_cast<String_Knob>(_knob);
        boost::shared_ptr<File_Knob> isFile = boost::dynamic_pointer_cast<File_Knob>(_knob);
        boost::shared_ptr<OutputFile_Knob> isOutputFile = boost::dynamic_pointer_cast<OutputFile_Knob>(_knob);
        boost::shared_ptr<Path_Knob> isPath = boost::dynamic_pointer_cast<Path_Knob>(_knob);
        boost::shared_ptr<Color_Knob> isColor = boost::dynamic_pointer_cast<Color_Knob>(_knob);
        boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(_knob);
        
        ///Make sure the knob is one of these types
        assert(isInt || isBool || isDouble || isChoice || isString || isFile || isOutputFile || isPath || isColor || isParametric);
        
        bool enabled = _knob->isEnabled(_dimension);
        ar & boost::serialization::make_nvp("Enabled",enabled);
        
        bool hasAnimation = _knob->isAnimated(_dimension);
        ar & boost::serialization::make_nvp("HasAnimation",hasAnimation);
        if (hasAnimation) {
            Curve c = *(_knob->getCurve(_dimension));
            ar & boost::serialization::make_nvp("Curve",c);
        }
        
        if (isInt) {
            int v = isInt->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isBool) {
            bool v = isBool->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isDouble) {
            double v = isDouble->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isChoice) {
            int v = isChoice->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isString) {
            std::string v = isString->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isFile) {
            std::string v = isFile->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isOutputFile) {
            std::string v = isOutputFile->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isPath) {
            std::string v = isPath->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        else if (isColor) {
            double v = isColor->getValue(_dimension);
            ar & boost::serialization::make_nvp("Value",v);
        }
        
        bool hasMaster = _knob->isSlave(_dimension);
        ar & boost::serialization::make_nvp("HasMaster",hasMaster);
        if (hasMaster) {
            ar & boost::serialization::make_nvp("Master",_master);
        }
    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int /*version*/)
    {
        
        boost::shared_ptr<Int_Knob> isInt = boost::dynamic_pointer_cast<Int_Knob>(_knob);
        boost::shared_ptr<Bool_Knob> isBool = boost::dynamic_pointer_cast<Bool_Knob>(_knob);
        boost::shared_ptr<Double_Knob> isDouble = boost::dynamic_pointer_cast<Double_Knob>(_knob);
        boost::shared_ptr<Choice_Knob> isChoice = boost::dynamic_pointer_cast<Choice_Knob>(_knob);
        boost::shared_ptr<String_Knob> isString = boost::dynamic_pointer_cast<String_Knob>(_knob);
        boost::shared_ptr<File_Knob> isFile = boost::dynamic_pointer_cast<File_Knob>(_knob);
        boost::shared_ptr<OutputFile_Knob> isOutputFile = boost::dynamic_pointer_cast<OutputFile_Knob>(_knob);
        boost::shared_ptr<Path_Knob> isPath = boost::dynamic_pointer_cast<Path_Knob>(_knob);
        boost::shared_ptr<Color_Knob> isColor = boost::dynamic_pointer_cast<Color_Knob>(_knob);
        boost::shared_ptr<Parametric_Knob> isParametric = boost::dynamic_pointer_cast<Parametric_Knob>(_knob);
        
        ///Make sure the knob is one of these types
        assert(isInt || isBool || isDouble || isChoice || isString || isFile || isOutputFile || isPath || isColor || isParametric);
        
        bool enabled;
        ar & boost::serialization::make_nvp("Enabled",enabled);
        _knob->setEnabled(_dimension, enabled);
        
        bool hasAnimation;
        ar & boost::serialization::make_nvp("HasAnimation",hasAnimation);
        if (hasAnimation) {
            Curve c;
            ar & boost::serialization::make_nvp("Curve",c);
            _knob->getCurve(_dimension)->clone(c);
        }
        
        if (isInt) {
            int v;
            ar & boost::serialization::make_nvp("Value",v);
            isInt->setValue(v,_dimension);
        }
        else if (isBool) {
            bool v;
            ar & boost::serialization::make_nvp("Value",v);
            isBool->setValue(v,_dimension);
        }
        else if (isDouble) {
            double v;
            ar & boost::serialization::make_nvp("Value",v);
            isDouble->setValue(v,_dimension);
        }
        else if (isChoice) {
            int v;
            ar & boost::serialization::make_nvp("Value",v);
            isChoice->setValue(v,_dimension);
        }
        else if (isString) {
            std::string v;
            ar & boost::serialization::make_nvp("Value",v);
            isString->setValue(v,_dimension);
        }
        else if (isFile) {
            std::string v;
            ar & boost::serialization::make_nvp("Value",v);
            isFile->setValue(v,_dimension);
        }
        else if (isOutputFile) {
            std::string v;
            ar & boost::serialization::make_nvp("Value",v);
            isOutputFile->setValue(v,_dimension);
        }
        else if (isPath) {
            std::string v;
            ar & boost::serialization::make_nvp("Value",v);
            isPath->setValue(v,_dimension);
        }
        else if (isColor) {
            double v;
            ar & boost::serialization::make_nvp("Value",v);
            isColor->setValue(v,_dimension);
        }
        
        ///We cannot restore the master yet. It has to be done in another pass.
        bool hasMaster;
        ar & boost::serialization::make_nvp("HasMaster",hasMaster);
        if (hasMaster) {
            ar & boost::serialization::make_nvp("Master",_master);
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

class KnobSerialization
{
    boost::shared_ptr<KnobI> _knob; //< used when serializing
    std::string _typeName;
    int _dimension;
    std::list<MasterSerialization> _masters; //< used when deserializating, we can't restore it before all knobs have been restored.
    std::list< Curve > parametricCurves;
    std::list<Double_Knob::SerializedTrack> slavedTracks; //< same as for master, can't be used right away when deserializing

    friend class boost::serialization::access;
    template<class Archive>
    void save(Archive & ar, const unsigned int /*version*/) const
    {
        
        AnimatingString_KnobHelper* isString = dynamic_cast<AnimatingString_KnobHelper*>(_knob.get());
        Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(_knob.get());
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(_knob.get());
        std::string name = _knob->getName();
        ar & boost::serialization::make_nvp("Name",name);
        
        ar & boost::serialization::make_nvp("Type",_typeName);
        
        ar & boost::serialization::make_nvp("Dimension",_dimension);

        bool secret = _knob->getIsSecret();
        ar & boost::serialization::make_nvp("Secret",secret);
        
        for (int i = 0; i < _knob->getDimension(); ++i) {
            ValueSerialization vs(_knob,i,true);
            ar & boost::serialization::make_nvp("item",vs);
        }
        
        ////restore extra datas
        if (isParametric) {
            std::list< Curve > curves;
            isParametric->saveParametricCurves(&curves);
            ar & boost::serialization::make_nvp("ParametricCurves",curves);
        }
        else if (isString) {
            std::map<int,std::string> extraDatas;
            isString->getAnimation().save(&extraDatas);
            ar & boost::serialization::make_nvp("StringsAnimation",extraDatas);
        }
        else if (isDouble && isDouble->getName() == "center" && isDouble->getDimension() == 2) {
            std::list<Double_Knob::SerializedTrack> tracks;
            isDouble->serializeTracks(&tracks);
            int count = (int)tracks.size();
            ar & boost::serialization::make_nvp("SlavePtsNo",count);
            for (std::list<Double_Knob::SerializedTrack>::iterator it = tracks.begin(); it!=tracks.end(); ++it) {
                ar & boost::serialization::make_nvp("SlavePtNodeName",it->rotoNodeName);
                ar & boost::serialization::make_nvp("SlavePtBezier",it->bezierName);
                ar & boost::serialization::make_nvp("SlavePtIndex",it->cpIndex);
                ar & boost::serialization::make_nvp("SlavePtIsFeather",it->isFeather);
            }
            
        }

    }
    
    template<class Archive>
    void load(Archive & ar, const unsigned int version)
    {
        
        std::string name;
        ar & boost::serialization::make_nvp("Name",name);
        
        ar & boost::serialization::make_nvp("Type",_typeName);
        
        ar & boost::serialization::make_nvp("Dimension",_dimension);
        

        assert(!_knob);
        
        boost::shared_ptr<KnobI> created = createKnob(_typeName, _dimension);
        if (!created) {
            return;
        } else {
            _knob = created;
        }
        _knob->setName(name);
        
        
        bool secret;
        ar & boost::serialization::make_nvp("Secret",secret);
        _knob->setSecret(secret);
        
        AnimatingString_KnobHelper* isStringAnimated = dynamic_cast<AnimatingString_KnobHelper*>(_knob.get());
        Parametric_Knob* isParametric = dynamic_cast<Parametric_Knob*>(_knob.get());
        Double_Knob* isDouble = dynamic_cast<Double_Knob*>(_knob.get());
        for (int i = 0; i < _knob->getDimension(); ++i) {
            ValueSerialization vs(_knob,i,false);
            ar & boost::serialization::make_nvp("item",vs);
            _masters.push_back(vs._master);
        }
        
        ////restore extra datas
        if (isParametric) {
            std::list< Curve > curves;
            ar & boost::serialization::make_nvp("ParametricCurves",curves);
            isParametric->loadParametricCurves(curves);
        }
        else if (isStringAnimated) {
            std::map<int,std::string> extraDatas;
            ar & boost::serialization::make_nvp("StringsAnimation",extraDatas);
            isStringAnimated->loadAnimation(extraDatas);
        }
        else if (version >= KNOB_SERIALIZATION_INTRODUCES_SLAVED_TRACKS &&
                 isDouble && isDouble->getName() == "center" && isDouble->getDimension() == 2) {
            int count ;
            ar & boost::serialization::make_nvp("SlavePtsNo",count);
            for (int i = 0; i < count;++i) {
                
                Double_Knob::SerializedTrack t;
                ar & boost::serialization::make_nvp("SlavePtNodeName",t.rotoNodeName);
                ar & boost::serialization::make_nvp("SlavePtBezier",t.bezierName);
                ar & boost::serialization::make_nvp("SlavePtIndex",t.cpIndex);
                ar & boost::serialization::make_nvp("SlavePtIsFeather",t.isFeather);
                slavedTracks.push_back(t);
            }
        }
        
        
        
    }
    
    BOOST_SERIALIZATION_SPLIT_MEMBER()
public:
    
    ///Constructor used to serialize
    explicit
    KnobSerialization(const boost::shared_ptr<KnobI>& knob)
    : _knob(knob)
    , _typeName(knob->typeName())
    , _dimension(knob->getDimension())
    , _masters()
    {
        
    }
    
    ///Doing the empty param constructor + this function is the same
    ///as calling the constructore above
    void initialize(const boost::shared_ptr<KnobI>& knob)
    {
        _knob = knob;
        _typeName = knob->typeName();
        _dimension = knob->getDimension();
    }
    
    ///Constructor used to deserialize: It will try to deserialize the next knob in the archive
    ///into a knob of the holder. If it couldn't find a knob with the same name as it was serialized
    ///this the deserialization will not succeed.
    KnobSerialization()
    : _knob()
    {
    }
    
    /**
     * @brief This function cannot be called until all knobs of the project have been created.
     **/
    void restoreKnobLinks(const boost::shared_ptr<KnobI>& knob,const std::vector<boost::shared_ptr<Natron::Node> >& allNodes);
    
    boost::shared_ptr<KnobI> getKnob() const { return _knob; }

    
    std::string getName() const { return _knob->getName(); }
    
    static boost::shared_ptr<KnobI> createKnob(const std::string& typeName,int dimension);

    bool restoreTracks(const boost::shared_ptr<KnobI>& knob,const std::vector<boost::shared_ptr<Natron::Node> >& allNodes);
    
private:
    
    
};

BOOST_CLASS_VERSION(KnobSerialization, KNOB_SERIALIZATION_VERSION)


#endif // KNOBSERIALIZATION_H
