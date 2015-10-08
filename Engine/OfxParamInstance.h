/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_OFXPARAMINSTANCE_H
#define NATRON_ENGINE_OFXPARAMINSTANCE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
#include <QMutex>
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
class KnobPath;
class KnobString;
class KnobFile;
class KnobOutputFile;
class KnobButton;
class KnobColor;
class KnobInt;
class KnobDouble;
class KnobBool;
class KnobChoice;
class KnobGroup;
class RichText_Knob;
class KnobPage;
class KnobParametric;
class OfxEffectInstance;
class OverlaySupport;
class KnobI;
class Format;

namespace Natron {
class OfxOverlayInteract;
}


class OfxParamToKnob;
class PropertyModified_RAII
{
    OfxParamToKnob* _h;
public:
    
    PropertyModified_RAII(OfxParamToKnob* h);
    
    ~PropertyModified_RAII();
};

#define SET_DYNAMIC_PROPERTY_EDITED() PropertyModified_RAII dynamic_prop_edited_raii(this)

class OfxParamToKnob : public QObject
{
    
    Q_OBJECT

    friend class PropertyModified_RAII;
    
    OFX::Host::Interact::Descriptor interactDesc;
    mutable QMutex dynamicPropModifiedMutex;
    bool _dynamicPropModified;
    
    
public:
   

    OfxParamToKnob()
    : dynamicPropModifiedMutex()
    , _dynamicPropModified(false)
    {
    }

    virtual boost::shared_ptr<KnobI> getKnob() const = 0;
    virtual OFX::Host::Param::Instance* getOfxParam() = 0;
    
    OFX::Host::Interact::Descriptor & getInteractDesc()
    {
        return interactDesc;
    }
    
    
    
    void connectDynamicProperties();

public Q_SLOTS:
    
    /*
     These are called when the properties are changed on the Natron side
     */
    void onEvaluateOnChangeChanged(bool evaluate);
    void onSecretChanged();
    void onEnabledChanged();
    void onDescriptionChanged();
    void onDisplayMinMaxChanged(double min,double max, int index);
    void onMinMaxChanged(double min,double max, int index);
    
    
protected:
    
    void setDynamicPropertyModified(bool dynamicPropModified)
    {
        QMutexLocker k(&dynamicPropModifiedMutex);
        _dynamicPropModified = dynamicPropModified;
    }
    
    bool isDynamicPropertyBeingModified() const
    {
        QMutexLocker k(&dynamicPropModifiedMutex);
        return _dynamicPropModified;
    }
    
    virtual bool hasDoubleMinMaxProps() const { return true; }
    
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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    boost::weak_ptr<KnobButton> _knob;
};


class OfxIntegerInstance
    :  public OfxParamToKnob, public OFX::Host::Param::IntegerInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int dim,int lvl);

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobInt> _knob;
};

class OfxDoubleInstance
:   public OfxParamToKnob, public OFX::Host::Param::DoubleInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    boost::weak_ptr<KnobDouble> _knob;
    OfxEffectInstance* _node;
};

class OfxBooleanInstance
:   public OfxParamToKnob, public OFX::Host::Param::BooleanInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    boost::weak_ptr<KnobBool> _knob;
};

class OfxChoiceInstance
    : public OfxParamToKnob, public OFX::Host::Param::ChoiceInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    std::vector<std::string> _entries;
    boost::weak_ptr<KnobChoice> _knob;
};

class OfxRGBAInstance
    :  public OfxParamToKnob, public OFX::Host::Param::RGBAInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<KnobColor> _knob;
};


class OfxRGBInstance
    :  public OfxParamToKnob,  public OFX::Host::Param::RGBInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    boost::weak_ptr<KnobColor> _knob;
};

class OfxDouble2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double2DInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<KnobDouble> _knob;
};


class OfxInteger2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer2DInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    OfxEffectInstance* _node;
    boost::weak_ptr<KnobInt> _knob;
};

class OfxDouble3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double3DInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    bool isAnimated(int dimension) const;
    bool isAnimated() const;

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    OfxEffectInstance* _node;
    boost::weak_ptr<KnobDouble> _knob;
};

class OfxInteger3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer3DInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

public Q_SLOTS:

    void onKnobAnimationLevelChanged(int,int lvl);

private:
    
    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }
    
    OfxEffectInstance* _node;
    boost::weak_ptr<KnobInt> _knob;
};

class OfxGroupInstance
    : public OFX::Host::Param::GroupInstance, public OfxParamToKnob
{
public:

    OfxGroupInstance(OfxEffectInstance* node,
                     OFX::Host::Param::Descriptor & descriptor);

    void addKnob(boost::shared_ptr<KnobI> k);

    virtual boost::shared_ptr<KnobI> getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    virtual ~OfxGroupInstance()
    {
    }

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

private:
    boost::weak_ptr<KnobGroup> _groupKnob;
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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
private:
    boost::weak_ptr<KnobPage> _pageKnob;
};


class OfxStringInstance
    : public OfxParamToKnob, public OFX::Host::Param::StringInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
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
    boost::weak_ptr<KnobFile> _fileKnob;
    boost::weak_ptr<KnobOutputFile> _outputFileKnob;
    boost::weak_ptr<KnobString> _stringKnob;
    boost::weak_ptr<KnobPath> _pathKnob;
    Natron::ThreadStorage<std::string> _localString;
};


class OfxCustomInstance
    : public OfxParamToKnob, public OFX::Host::Param::CustomInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
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
    boost::weak_ptr<KnobString> _knob;
    customParamInterpolationV1Entry_t _customParamInterpolationV1Entry;
    Natron::ThreadStorage<std::string> _localString;
};


class OfxParametricInstance
    : public OfxParamToKnob, public OFX::Host::ParametricParam::ParametricInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }
    
public Q_SLOTS:

    void onCustomBackgroundDrawingRequested();

    void initializeInteract(OverlaySupport* widget);


private:

    OFX::Host::Param::Descriptor & _descriptor;
    Natron::OfxOverlayInteract* _overlayInteract;
    OfxEffectInstance* _effect;
    boost::weak_ptr<KnobParametric> _knob;
};

#endif // NATRON_ENGINE_OFXPARAMINSTANCE_H

