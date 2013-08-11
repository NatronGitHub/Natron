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
#ifndef OFXPARAMINSTANCE_H
#define OFXPARAMINSTANCE_H

#include <QtCore/QObject>

//ofx
#include "ofxhImageEffect.h"


/*This file contains the classes that connect the Powiter knobs to the OpenFX params.
 Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
 never call them. When the user interact with a knob, the onInstanceChanged() slot
 is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */
class Knob;
class Button_Knob;
class Int_Knob;
class Int2D_Knob;
class Double_Knob;
class Double2D_Knob;
class Bool_Knob;
class ComboBox_Knob;
class Group_Knob;
class OfxNode;
class OfxPushButtonInstance :public QObject, public OFX::Host::Param::PushbuttonInstance {
    Q_OBJECT
        
    signals:
    void buttonPressed(QString);
    
public:
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();

    
    public slots:
    void emitInstanceChanged();
protected:
    OfxNode*   _effect;
    Button_Knob *_knob;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxPushButtonInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
};



class OfxIntegerInstance : public QObject,public OFX::Host::Param::IntegerInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
    int _value;
    Int_Knob* _knob;
    std::string _paramName;
public:
    
    OfxIntegerInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();

};

class OfxDoubleInstance :public QObject, public OFX::Host::Param::DoubleInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
    double _value;
    std::string _paramName;
    Double_Knob* _knob;
public:
    OfxDoubleInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
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
    public slots:
    void onInstanceChanged();

};

class OfxBooleanInstance : public QObject,public OFX::Host::Param::BooleanInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
    bool _value;
    Bool_Knob* _knob;
    std::string _paramName;
public:
    OfxBooleanInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(bool&);
    OfxStatus get(OfxTime time, bool&);
    OfxStatus set(bool);
    OfxStatus set(OfxTime time, bool);
    
    
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    public slots:
    void onInstanceChanged();

};

class OfxChoiceInstance :public QObject, public OFX::Host::Param::ChoiceInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
    std::string _currentEntry;
    std::vector<std::string> _entries;
    ComboBox_Knob* _knob;
    std::string _paramName;
public:
    OfxChoiceInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();
  
};

class OfxRGBAInstance :public QObject, public OFX::Host::Param::RGBAInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
	double _r,_g,_b,_a;
    std::string _paramName;
public:
    OfxRGBAInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&,double&);
    OfxStatus set(double,double,double,double);
    OfxStatus set(OfxTime time, double,double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();

};


class OfxRGBInstance :public QObject, public OFX::Host::Param::RGBInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
	double _r,_g,_b;
    std::string _paramName;
public:
    OfxRGBInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time, double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();

};

class OfxDouble2DInstance : public QObject,public OFX::Host::Param::Double2DInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
	double _x1,_x2;
    Double2D_Knob* _knob;
    std::string _paramName;
public:
    OfxDouble2DInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&);
    OfxStatus get(OfxTime time,double&,double&);
    OfxStatus set(double,double);
    OfxStatus set(OfxTime time,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();

};

class OfxInteger2DInstance :public QObject, public OFX::Host::Param::Integer2DInstance {
    Q_OBJECT
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
	int _x1,_x2;
    Int2D_Knob *_knob;
    std::string _paramName;
public:
    OfxInteger2DInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&);
    OfxStatus get(OfxTime time,int&,int&);
    OfxStatus set(int,int);
    OfxStatus set(OfxTime time,int,int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    public slots:
    void onInstanceChanged();

};
class OfxGroupInstance : public QObject, public OFX::Host::Param::GroupInstance{
    Q_OBJECT
    OfxNode* _effect;
    OFX::Host::Param::Descriptor& _descriptor;
    std::string _paramName;
    Group_Knob* _knob;
public:
    
    OfxGroupInstance(OfxNode* effect,const std::string& name,OFX::Host::Param::Descriptor& descriptor);
    
    void addKnob(Knob* k);
    
    virtual ~OfxGroupInstance(){}
};


#endif // OFXPARAMINSTANCE_H

