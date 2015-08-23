//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */
#ifndef NATRON_ENGINE_OFXPARAMINSTANCE_H_
#define NATRON_ENGINE_OFXPARAMINSTANCE_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Global/Macros.h"
#include <map>
#include <string>
#include <vector>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif
CLANG_DIAG_OFF(deprecated)
#include <QStringList>
CLANG_DIAG_ON(deprecated)

#include "Engine/ThreadStorage.h"


#include "Global/GlobalDefines.h"
//ofx
// ofxhPropertySuite.h:565:37: warning: 'this' pointer cannot be null in well-defined C++ code; comparison may be assumed to always evaluate to true [-Wtautological-undefined-compare]
CLANG_DIAG_OFF(unknown-pragmas)
CLANG_DIAG_OFF(tautological-undefined-compare) // appeared in clang 3.5
#include <ofxhImageEffect.h>
CLANG_DIAG_ON(tautological-undefined-compare)
CLANG_DIAG_ON(unknown-pragmas)
#include "ofxCore.h"
#include "ofxhParametricParam.h"


/*This file contains the classes that connect the knobs to the OpenFX params.
   Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
   never call them. When the user interact with a knob, the onInstanceChanged() slot
   is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */
class Path_Knob;
class String_Knob;
class File_Knob;
class OutputFile_Knob;
class Button_Knob;
class Color_Knob;
class Int_Knob;
class Double_Knob;
class Bool_Knob;
class Choice_Knob;
class Group_Knob;
class RichText_Knob;
class Page_Knob;
class Parametric_Knob;
class OfxEffectInstance;
class OverlaySupport;
class KnobI;
class Format;

namespace Natron {
class OfxOverlayInteract;
}


class OfxParamToKnob
{
    OFX::Host::Interact::Descriptor interactDesc;

public:

    OfxParamToKnob()
    {
    }

    virtual boost::shared_ptr<KnobI> getKnob() const = 0;
    OFX::Host::Interact::Descriptor & getInteractDesc()
    {
        return interactDesc;
    }
};

class OfxPushButtonInstance
    : public OFX::Host::Param::PushbuttonInstance, public OfxParamToKnob
{
public:
    OfxPushButtonInstance(OfxEffectInstance* node,
                          OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

private:
    boost::weak_ptr<Button_Knob> _knob;
};


class OfxIntegerInstance
    :  public QObject, public OFX::Host::Param::IntegerInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:

    OfxIntegerInstance(OfxEffectInstance* node,
                       OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;
    
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int dim,int lvl);

private:
    boost::weak_ptr<Int_Knob> _knob;
};

class OfxDoubleInstance
    :  public QObject,  public OFX::Host::Param::DoubleInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxDoubleInstance(OfxEffectInstance* node,
                      OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<Double_Knob> _knob;
    OfxEffectInstance* _node;
};

class OfxBooleanInstance
    :  public QObject,  public OFX::Host::Param::BooleanInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxBooleanInstance(OfxEffectInstance* node,
                       OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(bool &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, bool &) OVERRIDE FINAL;
    virtual OfxStatus set(bool) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, bool) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<Bool_Knob> _knob;
};

class OfxChoiceInstance
    : public QObject, public OFX::Host::Param::ChoiceInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxChoiceInstance(OfxEffectInstance* node,
                      OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    // callback which should set option as appropriate
    virtual void setOption(int num) OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    std::vector<std::string> _entries;
    boost::weak_ptr<Choice_Knob> _knob;
};

class OfxRGBAInstance
    :  public QObject, public OFX::Host::Param::RGBAInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxRGBAInstance(OfxEffectInstance* node,
                    OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<Color_Knob> _knob;
};


class OfxRGBInstance
    :  public QObject,  public OFX::Host::Param::RGBInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxRGBInstance(OfxEffectInstance* node,
                   OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<Color_Knob> _knob;
};

class OfxDouble2DInstance
    :  public QObject, public OFX::Host::Param::Double2DInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxDouble2DInstance(OfxEffectInstance* node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<Double_Knob> _knob;
};


class OfxInteger2DInstance
    :  public QObject, public OFX::Host::Param::Integer2DInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxInteger2DInstance(OfxEffectInstance* node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &,int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus set(int,int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,int,int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<Int_Knob> _knob;
};

class OfxDouble3DInstance
    :  public QObject, public OFX::Host::Param::Double3DInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxDouble3DInstance(OfxEffectInstance* node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,double &,double &,double &) OVERRIDE FINAL;
    virtual OfxStatus set(double,double,double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,double,double,double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setLabel() OVERRIDE FINAL;
    
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<Double_Knob> _knob;
};

class OfxInteger3DInstance
    :  public QObject, public OFX::Host::Param::Integer3DInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxInteger3DInstance(OfxEffectInstance* node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time,int &,int &,int &) OVERRIDE FINAL;
    virtual OfxStatus set(int,int,int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time,int,int,int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    virtual void setLabel() OVERRIDE FINAL;
    
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<Int_Knob> _knob;
};

class OfxGroupInstance
    : public OFX::Host::Param::GroupInstance, public OfxParamToKnob
{
public:

    OfxGroupInstance(OfxEffectInstance* node,
                     OFX::Host::Param::Descriptor & descriptor);

    void addKnob(boost::shared_ptr<KnobI> k);

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    virtual ~OfxGroupInstance()
    {
    }

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

private:
    boost::weak_ptr<Group_Knob> _groupKnob;
};

class OfxPageInstance
    : public OFX::Host::Param::PageInstance, public OfxParamToKnob
{
public:


    OfxPageInstance(OfxEffectInstance* node,
                    OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

private:
    boost::weak_ptr<Page_Knob> _pageKnob;
};


class OfxStringInstance
    : public QObject, public OFX::Host::Param::StringInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxStringInstance(OfxEffectInstance* node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual OfxStatus get(std::string &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, std::string &) OVERRIDE FINAL;
    virtual OfxStatus set(const char*) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    virtual ~OfxStringInstance()
    {
    }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    /**
     * @brief Expand any project path contained in str
     **/
    void projectEnvVar_getProxy(std::string& str) const;
    
    /**
     * @brief Find any project path contained in str and replace it by the associated name
     **/
    void projectEnvVar_setProxy(std::string& str) const;
    
    OfxEffectInstance* _node;
    boost::weak_ptr<File_Knob> _fileKnob;
    boost::weak_ptr<OutputFile_Knob> _outputFileKnob;
    boost::weak_ptr<String_Knob> _stringKnob;
    boost::weak_ptr<Path_Knob> _pathKnob;
    Natron::ThreadStorage<std::string> _localString;
};


class OfxCustomInstance
    : public QObject, public OFX::Host::Param::CustomInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:
    OfxCustomInstance(OfxEffectInstance* node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual OfxStatus get(std::string &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, std::string &) OVERRIDE FINAL;
    virtual OfxStatus set(const char*) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, const char*) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(va_list arg) OVERRIDE FINAL;

    /// implementation of var args function
    /// Be careful: the char* is only valid until next API call
    /// see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#ArchitectureStrings
    virtual OfxStatus getV(OfxTime time, va_list arg) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

    virtual ~OfxCustomInstance()
    {
    }

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:

    void getCustomParamAtTime(double time,std::string &str) const;

    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);

    OfxEffectInstance* _node;
    boost::weak_ptr<String_Knob> _knob;
    customParamInterpolationV1Entry_t _customParamInterpolationV1Entry;
    Natron::ThreadStorage<std::string> _localString;
};


class OfxParametricInstance
    : public QObject, public OFX::Host::ParametricParam::ParametricInstance, public OfxParamToKnob
{
CLANG_DIAG_OFF(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON(inconsistent-missing-override)

public:

    explicit OfxParametricInstance(OfxEffectInstance* node,
                                   OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxParametricInstance();

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should update label
    virtual void setLabel() OVERRIDE FINAL;
    

    /// callback which should set
    virtual void setDisplayRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    void onCurvesDefaultInitialized();
    
    ///derived from CurveHolder
    virtual OfxStatus getValue(int curveIndex,OfxTime time,double parametricPosition,double *returnValue) OVERRIDE FINAL;
    virtual OfxStatus getNControlPoints(int curveIndex,double time,int *returnValue) OVERRIDE FINAL;
    virtual OfxStatus getNthControlPoint(int curveIndex,
                                         double time,
                                         int nthCtl,
                                         double *key,
                                         double *value) OVERRIDE FINAL;
    virtual OfxStatus setNthControlPoint(int curveIndex,
                                         double time,
                                         int nthCtl,
                                         double key,
                                         double value,
                                         bool addAnimationKey) OVERRIDE FINAL;
    virtual OfxStatus addControlPoint(int curveIndex,
                                      double time,
                                      double key,
                                      double value,
                                      bool addAnimationKey) OVERRIDE FINAL;
    virtual OfxStatus  deleteControlPoint(int curveIndex, int nthCtl) OVERRIDE FINAL;
    virtual OfxStatus  deleteAllControlPoints(int curveIndex) OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;

public Q_SLOTS:

    void onCustomBackgroundDrawingRequested();

    void initializeInteract(OverlaySupport* widget);


private:

    OFX::Host::Param::Descriptor & _descriptor;
    Natron::OfxOverlayInteract* _overlayInteract;
    OfxEffectInstance* _effect;
    boost::weak_ptr<Parametric_Knob> _knob;
};

#endif // NATRON_ENGINE_OFXPARAMINSTANCE_H_

