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
#include <QtCore/QObject>
#include <QStringList>
#include <QVector4D>

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
class OfxNode;
class Knob;
class OfxPushButtonInstance :public QObject, public OFX::Host::Param::PushbuttonInstance {
    Q_OBJECT
    
public:
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
public slots:
    void onInstanceChanged();

protected:
    OfxNode* _node;
    Button_Knob *_knob;
    OFX::Host::Param::Descriptor& _descriptor;
public:
    OfxPushButtonInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
};



class OfxIntegerInstance : public QObject,public OFX::Host::Param::IntegerInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    Int_Knob* _knob;
    std::string _paramName;
public:
    
    OfxIntegerInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
    
};

class OfxDoubleInstance :public QObject, public OFX::Host::Param::DoubleInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    std::string _paramName;
    Double_Knob* _knob;
public:
    OfxDoubleInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
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
    
    public slots:
    void onInstanceChanged();
    
};

class OfxBooleanInstance : public QObject,public OFX::Host::Param::BooleanInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    Bool_Knob* _knob;
    std::string _paramName;
public:
    OfxBooleanInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(bool&);
    OfxStatus get(OfxTime time, bool&);
    OfxStatus set(bool);
    OfxStatus set(OfxTime time, bool);
    
    
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
};

class OfxChoiceInstance :public QObject, public OFX::Host::Param::ChoiceInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    std::vector<std::string> _entries;
    ComboBox_Knob* _knob;
    std::string _paramName;
public:
    OfxChoiceInstance(OfxNode* node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&);
    OfxStatus get(OfxTime time, int&);
    OfxStatus set(int);
    OfxStatus set(OfxTime time, int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
};

class OfxRGBAInstance :public QObject, public OFX::Host::Param::RGBAInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    RGBA_Knob* _knob;
    std::string _paramName;
public:
    OfxRGBAInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&,double&);
    OfxStatus set(double,double,double,double);
    OfxStatus set(OfxTime time, double,double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
};


class OfxRGBInstance :public QObject, public OFX::Host::Param::RGBInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    RGBA_Knob* _knob;
    std::string _paramName;
public:
    OfxRGBInstance(OfxNode* node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&,double&);
    OfxStatus get(OfxTime time, double&,double&,double&);
    OfxStatus set(double,double,double);
    OfxStatus set(OfxTime time, double,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
};

class OfxDouble2DInstance : public QObject,public OFX::Host::Param::Double2DInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    Double_Knob* _knob;
    std::string _paramName;
public:
    OfxDouble2DInstance(OfxNode* node, const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(double&,double&);
    OfxStatus get(OfxTime time,double&,double&);
    OfxStatus set(double,double);
    OfxStatus set(OfxTime time,double,double);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    
    public slots:
    void onInstanceChanged();
    
};

class OfxInteger2DInstance :public QObject, public OFX::Host::Param::Integer2DInstance {
    Q_OBJECT
protected:
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    Int_Knob *_knob;
    std::string _paramName;
public:
    OfxInteger2DInstance(OfxNode* node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor);
    OfxStatus get(int&,int&);
    OfxStatus get(OfxTime time,int&,int&);
    OfxStatus set(int,int);
    OfxStatus set(OfxTime time,int,int);
    
    // callback which should set enabled state as appropriate
    virtual void setEnabled();
    
    // callback which should set secret state as appropriate
    virtual void setSecret();
    
    Knob* getKnob() const;
    
    public slots:
    void onInstanceChanged();
    
};
class OfxGroupInstance : public QObject, public OFX::Host::Param::GroupInstance{
    Q_OBJECT
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    std::string _paramName;
    Group_Knob* _groupKnob;
public:
    
    OfxGroupInstance(OfxNode* node,const std::string& name,OFX::Host::Param::Descriptor& descriptor);
    
    void addKnob(Knob* k);
    
    Knob* getKnob() const;
    
    virtual ~OfxGroupInstance(){}
};

class OfxStringInstance : public QObject, public OFX::Host::Param::StringInstance{
    Q_OBJECT
    OfxNode* _node;
    OFX::Host::Param::Descriptor& _descriptor;
    std::string _paramName;
        
    std::map<int,QString> _files;
        
    File_Knob* _fileKnob;
    OutputFile_Knob* _outputFileKnob;
    String_Knob* _stringKnob;
public:
    
    OfxStringInstance(OfxNode* node,const std::string& name,OFX::Host::Param::Descriptor& descriptor);
    
    virtual OfxStatus get(std::string&);
    virtual OfxStatus get(OfxTime time, std::string&);
    virtual OfxStatus set(const char*);
    virtual OfxStatus set(OfxTime time, const char*);
    
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
    
    Knob* getKnob() const;
    
    virtual ~OfxStringInstance(){}
    
    public slots:
    void onInstanceChanged();
    
private:
    void getVideoSequenceFromFilesList();
    int firstFrame();
    int lastFrame();
    int clampToRange(int f);
    
};


#endif // POWITER_ENGINE_OFXPARAMINSTANCE_H_

