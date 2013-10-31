//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#ifndef POWITER_ENGINE_OFXPARAMINSTANCE_H_
#define POWITER_ENGINE_OFXPARAMINSTANCE_H_

#include <map>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <QStringList>
#include <QVector4D>
#include <QThreadStorage>


#include "Global/GlobalDefines.h"
//ofx
#include "ofxhImageEffect.h"


/*This file contains the classes that connect the Powiter knobs to the OpenFX params.
 Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
 never call them. When the user interact with a knob, the onInstanceChanged() slot
 is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */
class String_Knob;
class File_Knob;
class OutputFile_Knob;
class Button_Knob;
class Tab_Knob;
class RGBA_Knob;
class Int_Knob;
class Double_Knob;
class Bool_Knob;
class ComboBox_Knob;
class Group_Knob;
class RichText_Knob;
class OfxNode;
class Knob;
class OfxPushButtonInstance :public OFX::Host::Param::PushbuttonInstance {
    
public:
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;


protected:
    OfxNode* _node;
    Button_Knob *_knob;
public:
    OfxPushButtonInstance(OfxNode* node, OFX::Host::Param::Descriptor& descriptor);
};



class OfxIntegerInstance :public OFX::Host::Param::IntegerInstance {
    
protected:
    OfxNode* _node;
    Int_Knob* _knob;

public:
    
    OfxIntegerInstance(OfxNode* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    
};

class OfxDoubleInstance : public OFX::Host::Param::DoubleInstance {
    
protected:
    OfxNode* _node;
    Double_Knob* _knob;

public:
    OfxDoubleInstance(OfxNode* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&);
    OfxStatus get(OfxTime time, double&);
    OfxStatus set(double);
    OfxStatus set(OfxTime time, double);
    OfxStatus derive(OfxTime time, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
};

class OfxBooleanInstance : public OFX::Host::Param::BooleanInstance {
    
protected:
    OfxNode* _node;
    Bool_Knob* _knob;
 
public:
    OfxBooleanInstance(OfxNode* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(bool&);
    OfxStatus get(OfxTime time, bool&);
    OfxStatus set(bool);
    OfxStatus set(OfxTime time, bool);
    
    
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;

    
};

class OfxChoiceInstance : public OFX::Host::Param::ChoiceInstance {
    
protected:
    OfxNode* _node;
    std::vector<std::string> _entries;
    ComboBox_Knob* _knob;

public:
    OfxChoiceInstance(OfxNode* node,  OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    

};

class OfxRGBAInstance :public OFX::Host::Param::RGBAInstance {
    
protected:
    OfxNode* _node;
    RGBA_Knob* _knob;

public:
    OfxRGBAInstance(OfxNode* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&,double&);
    OfxStatus set(double,double,double,double);
    OfxStatus set(OfxTime time, double,double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    
};


class OfxRGBInstance : public OFX::Host::Param::RGBInstance {
protected:
    OfxNode* _node;
    RGBA_Knob* _knob;

public:
    OfxRGBInstance(OfxNode* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time, double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
};

class OfxDouble2DInstance :public OFX::Host::Param::Double2DInstance {
protected:
    OfxNode* _node;
    Double_Knob* _knob;

public:
    OfxDouble2DInstance(OfxNode* node,OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&);
    OfxStatus get(OfxTime time,double&,double&);
    OfxStatus set(double,double);
    OfxStatus set(OfxTime time,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;

};

class OfxInteger2DInstance : public OFX::Host::Param::Integer2DInstance {

protected:
    OfxNode* _node;
    Int_Knob *_knob;

public:
    OfxInteger2DInstance(OfxNode* node, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&);
    OfxStatus get(OfxTime time,int&,int&);
    OfxStatus set(int,int);
    OfxStatus set(OfxTime time,int,int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
};
class OfxGroupInstance : public OFX::Host::Param::GroupInstance{
    OfxNode* _node;
    Group_Knob* _groupKnob;
public:
    
    OfxGroupInstance(OfxNode* node,OFX::Host::Param::Descriptor& descriptor);
    
    void addKnob(Knob* k);
    
    Knob* getKnob() const;
    
    virtual ~OfxGroupInstance(){}
};

class OfxStringInstance : public OFX::Host::Param::StringInstance{
    OfxNode* _node;
    File_Knob* _fileKnob;
    OutputFile_Knob* _outputFileKnob;
    String_Knob* _stringKnob;
    RichText_Knob* _multiLineKnob;
    QThreadStorage<std::string> _localString;
public:
    
    OfxStringInstance(OfxNode* node,OFX::Host::Param::Descriptor& descriptor);
    
    virtual OfxStatus get(std::string&) OVERRIDE;
    virtual OfxStatus get(OfxTime time, std::string&) OVERRIDE;
    virtual OfxStatus set(const char*) OVERRIDE;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE;
    
    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE;
    
    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE;

    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();

    // returns true if it is a file param and that it is not empty
    bool isValid() const;
    
    void ifFileKnobPopDialog();
    
    /*Returns the frame name according to the current pattern stored by this param
     for the frame frameIndex.*/
    std::string filenameFromPattern(int frameIndex) const;
    
    /**
     * @brief getRandomFrameName Valid only for OfxStringInstance's of type kOfxParamStringIsFilePath with
     * an OfxImageEffectInstance of type kOfxImageEffectContextGenerator
     * @param f The index of the frame.
     * @return The file name associated to the frame index. Returns an empty string if it couldn't find it.
     */
    const QString getRandomFrameName(int f) const;
    
    Knob* getKnob() const;
    
    virtual ~OfxStringInstance(){}
    

};


#endif // POWITER_ENGINE_OFXPARAMINSTANCE_H_

