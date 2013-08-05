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

//ofx
#include "ofxhImageEffect.h"

class OfxNode;
class OfxPushButtonInstance : public OFX::Host::Param::PushbuttonInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxPushButtonInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
};



class OfxIntegerInstance : public OFX::Host::Param::IntegerInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxIntegerInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
};

class OfxDoubleInstance : public OFX::Host::Param::DoubleInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxDoubleInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&);
    OfxStatus get(OfxTime time, double&);
    OfxStatus set(double);
    OfxStatus set(OfxTime time, double);
    OfxStatus derive(OfxTime time, double&);
    OfxStatus integrate(OfxTime time1, OfxTime time2, double&);
};

class OfxBooleanInstance : public OFX::Host::Param::BooleanInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxBooleanInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(bool&);
    OfxStatus get(OfxTime time, bool&);
    OfxStatus set(bool);
    OfxStatus set(OfxTime time, bool);
};

class OfxChoiceInstance : public OFX::Host::Param::ChoiceInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxChoiceInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
};

class OfxRGBAInstance : public OFX::Host::Param::RGBAInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxRGBAInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&,double&);
    OfxStatus set(double,double,double,double);
    OfxStatus set(OfxTime time, double,double,double,double);
};


class OfxRGBInstance : public OFX::Host::Param::RGBInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxRGBInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time, double,double,double);
};

class OfxDouble2DInstance : public OFX::Host::Param::Double2DInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxDouble2DInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&);
    OfxStatus get(OfxTime time,double&,double&);
    OfxStatus set(double,double);
    OfxStatus set(OfxTime time,double,double);
};

class OfxInteger2DInstance : public OFX::Host::Param::Integer2DInstance {
protected:
    OfxNode*   _effect;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxInteger2DInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&);
    OfxStatus get(OfxTime time,int&,int&);
    OfxStatus set(int,int);
    OfxStatus set(OfxTime time,int,int);
};


#endif // OFXPARAMINSTANCE_H

