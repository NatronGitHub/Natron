/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QStringList>
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)


#include "Global/GlobalDefines.h"

//ofx
#include <ofxInteract.h>
#include <ofxhParametricParam.h>
#include <ofxhInteract.h>

#include "Engine/KnobTypes.h"
#include "Engine/ViewIdx.h"
#include "Engine/EffectInstance.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/*This file contains the classes that connect the knobs to the OpenFX params.
   Note that all the get(...) and set(...) functions are called BY PLUGIN and you should
   never call them. When the user interact with a knob, the onInstanceChanged() slot
   is called. In turn, the plug-in will fetch the value that has changed by calling get(...).
 */


class OfxParamToKnob;

class PropertyModified_RAII
{
    OfxParamToKnob* _h;

public:

    PropertyModified_RAII(OfxParamToKnob* h);

    ~PropertyModified_RAII();
};

#define SET_DYNAMIC_PROPERTY_EDITED() PropertyModified_RAII dynamic_prop_edited_raii(this)

#define DYNAMIC_PROPERTY_CHECK() if ( isDynamicPropertyBeingModified() ) { \
        return; \
} \
    SET_DYNAMIC_PROPERTY_EDITED();

class OfxParamToKnob
    : public QObject
{
    Q_OBJECT

    friend class PropertyModified_RAII;

    OFX::Host::Interact::Descriptor interactDesc;
    mutable QMutex dynamicPropModifiedMutex;
    int _dynamicPropModified;
    boost::weak_ptr<EffectInstance> _effect;
    int _nRefreshAutoKeyingRequests;

public:


    OfxParamToKnob(const EffectInstancePtr& effect)
        : dynamicPropModifiedMutex()
        , _dynamicPropModified(0)
        , _effect(effect)
        , _nRefreshAutoKeyingRequests(0)
    {
    }

    virtual OfxPluginEntryPoint* getCustomOverlayInteractEntryPoint(const OFX::Host::Param::Instance* param) const
    {
        GCC_DIAG_PEDANTIC_OFF
        return (OfxPluginEntryPoint*)param->getProperties().getPointerProperty(kOfxParamPropInteractV1);
        GCC_DIAG_PEDANTIC_ON
    }

    virtual KnobIPtr getKnob() const = 0;
    virtual OFX::Host::Param::Instance* getOfxParam() = 0;
    OFX::Host::Interact::Descriptor & getInteractDesc()
    {
        return interactDesc;
    }

    EffectInstancePtr getKnobHolder() const;

    void connectDynamicProperties();

    //these are per ofxparam thread-local data
    struct OfxParamTLSData
    {
        //only for string-param for now
        std::string str;
    };

    static std::string getParamLabel(OFX::Host::Param::Instance* param)
    {
        std::string label = param->getLabel();

        /*if ( label.empty() ) {
            label = param->getShortLabel();
        }
        if ( label.empty() ) {
            label = param->getLongLabel();
        }*/

        return label;
    }

    template <typename TYPE>
    boost::shared_ptr<TYPE>
    getOrCreateKnob(const std::string& scriptName,
                    OFX::Host::Param::Instance* param,
                    int dimension)
    {
        EffectInstancePtr holder = getKnobHolder();
        boost::shared_ptr<TYPE> ret = holder->getOrCreateKnob<TYPE>(scriptName, dimension);
        if (!ret) {
            return ret;
        }


        /**
         * For readers/writers embedded in a ReadNode or WriteNode, the holder will be the ReadNode and WriteNode
         * but to ensure that all functions such as getKnobByName actually work, we add them to the knob vector so that
         * interacting with the Reader or the container is actually the same.
         **/
        EffectInstancePtr thisEffect = _effect.lock();
        if ( ret->getHolder() != thisEffect && !thisEffect->getKnobByName(scriptName) ) {
            thisEffect->addKnob(ret);
            ret->setActualCloneForHolder(thisEffect);
        }

        ret->setLabel(getParamLabel(param));
        
        return ret;
    }

public Q_SLOTS:

    /*
     These are called when the properties are changed on the Natron side
     */
    void onEvaluateOnChangeChanged(bool evaluate);
    void onSecretChanged();
    void onHintTooltipChanged();
    void onEnabledChanged();
    void onLabelChanged();
    void onDisplayMinMaxChanged(DimSpec dimension);
    void onMinMaxChanged(DimSpec dimension);
    void onMustRefreshGuiTriggered(ViewSetSpec,DimSpec,ValueChangedReasonEnum);
    void onChoiceMenuReset();
    void onChoiceMenuPopulated();
    void onChoiceMenuEntryAppended();
    void onInViewportSecretChanged();
    void onInViewportLabelChanged();

    void onMustRefreshAutoKeyingPropsLaterReceived();

    void refreshAutoKeyingPropsLater();

    void refreshAutoKeyingPropsNow();

Q_SIGNALS:

    void mustRefreshAnimationLevelLater();

protected:

    void setDynamicPropertyModified(bool dynamicPropModified)
    {
        QMutexLocker k(&dynamicPropModifiedMutex);

        if (dynamicPropModified) {
            ++_dynamicPropModified;
        } else {
            if (_dynamicPropModified > 0) {
                --_dynamicPropModified;
            }
        }
    }

    bool isDynamicPropertyBeingModified() const
    {
        QMutexLocker k(&dynamicPropModifiedMutex);

        return _dynamicPropModified > 0;
    }

    virtual bool hasDoubleMinMaxProps() const { return true; }
};


class OfxPushButtonInstance
    : public OfxParamToKnob, public OFX::Host::Param::PushbuttonInstance
{
public:
    OfxPushButtonInstance(const OfxEffectInstancePtr& node,
                          OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    virtual void setHint() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    KnobButtonWPtr _knob;
};


class OfxIntegerInstance
    :  public OfxParamToKnob, public OFX::Host::Param::IntegerInstance
{
public:

    OfxIntegerInstance(const OfxEffectInstancePtr& node,
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
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }

    KnobIntWPtr _knob;
};

class OfxDoubleInstance
    :   public OfxParamToKnob, public OFX::Host::Param::DoubleInstance
{
public:
    OfxDoubleInstance(const OfxEffectInstancePtr& node,
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
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;



    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:

    KnobDoubleWPtr _knob;
};

class OfxBooleanInstance
    :   public OfxParamToKnob, public OFX::Host::Param::BooleanInstance
{
public:
    OfxBooleanInstance(const OfxEffectInstancePtr& node,
                       OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(bool &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, bool &) OVERRIDE FINAL;
    virtual OfxStatus set(bool) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, bool) OVERRIDE FINAL;


    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }

    KnobBoolWPtr _knob;
};

class OfxChoiceInstance
    : public OfxParamToKnob, public OFX::Host::Param::ChoiceInstance
{
public:
    OfxChoiceInstance(const OfxEffectInstancePtr& node,
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
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set option as appropriate
    virtual void setOption(int num) OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }

    KnobChoiceWPtr _knob;
};

class OfxRGBAInstance
    :  public OfxParamToKnob, public OFX::Host::Param::RGBAInstance
{
public:
    OfxRGBAInstance(const OfxEffectInstancePtr& node,
                    OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double, double, double, double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double, double, double, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    KnobColorWPtr _knob;
};


class OfxRGBInstance
    :  public OfxParamToKnob,  public OFX::Host::Param::RGBInstance
{
public:
    OfxRGBInstance(const OfxEffectInstancePtr& node,
                   OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double, double, double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double, double, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:
    KnobColorWPtr _knob;
};

class OfxDouble2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double2DInstance
{
public:
    OfxDouble2DInstance(const OfxEffectInstancePtr& node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &, double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double, double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:

    int _startIndex;
    KnobDoubleWPtr _knob;
};


class OfxInteger2DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer2DInstance
{
public:
    OfxInteger2DInstance(const OfxEffectInstancePtr& node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &, int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int, int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }

    int _startIndex;
    KnobIntWPtr _knob;
};

class OfxDouble3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Double3DInstance
{
public:
    OfxDouble3DInstance(const OfxEffectInstancePtr& node,
                        OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus set(double, double, double) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, double, double, double) OVERRIDE FINAL;
    virtual OfxStatus derive(OfxTime time, double &, double &, double &) OVERRIDE FINAL;
    virtual OfxStatus integrate(OfxTime time1, OfxTime time2, double &, double &, double &) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


private:

    int _startIndex;
    KnobDoubleWPtr _knob;
};

class OfxInteger3DInstance
    :  public OfxParamToKnob, public OFX::Host::Param::Integer3DInstance
{
public:
    OfxInteger3DInstance(const OfxEffectInstancePtr& node,
                         OFX::Host::Param::Descriptor & descriptor);
    virtual OfxStatus get(int &, int &, int &) OVERRIDE FINAL;
    virtual OfxStatus get(OfxTime time, int &, int &, int &) OVERRIDE FINAL;
    virtual OfxStatus set(int, int, int) OVERRIDE FINAL;
    virtual OfxStatus set(OfxTime time, int, int, int) OVERRIDE FINAL;

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual void setRange() OVERRIDE FINAL;
    virtual void setDisplayRange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    virtual bool hasDoubleMinMaxProps() const OVERRIDE FINAL { return false; }

    KnobIntWPtr _knob;
};

class OfxGroupInstance
    : public OfxParamToKnob, public OFX::Host::Param::GroupInstance
{
public:

    OfxGroupInstance(const OfxEffectInstancePtr& node,
                     OFX::Host::Param::Descriptor & descriptor);

    void addKnob(KnobIPtr k);

    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    virtual ~OfxGroupInstance()
    {
    }

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setOpen() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


private:
    KnobGroupWPtr _groupKnob;
};

class OfxPageInstance
    : public OfxParamToKnob, public OFX::Host::Param::PageInstance
{
public:


    OfxPageInstance(const OfxEffectInstancePtr& node,
                    OFX::Host::Param::Descriptor & descriptor);

    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;
    virtual void setHint() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:
    KnobPageWPtr _pageKnob;
};


struct OfxStringInstancePrivate;
class OfxStringInstance
    : public OfxParamToKnob, public OFX::Host::Param::StringInstance
{
public:
    OfxStringInstance(const OfxEffectInstancePtr& node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxStringInstance();

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
    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;


    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

private:

    boost::scoped_ptr<OfxStringInstancePrivate> _imp;
};

struct OfxCustomInstancePrivate;
class OfxCustomInstance
    : public OfxParamToKnob, public OFX::Host::Param::CustomInstance
{
public:
    OfxCustomInstance(const OfxEffectInstancePtr& node,
                      OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxCustomInstance();


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

    virtual void setHint() OVERRIDE FINAL;
    virtual void setDefault() OVERRIDE FINAL;
    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;
    virtual void setLabel() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }

    ///keyframes support
    virtual OfxStatus getNumKeys(unsigned int &nKeys) const OVERRIDE FINAL;
    virtual OfxStatus getKeyTime(int nth, OfxTime & time) const OVERRIDE FINAL;
    virtual OfxStatus getKeyIndex(OfxTime time, int direction, int & index) const OVERRIDE FINAL;
    virtual OfxStatus deleteKey(OfxTime time) OVERRIDE FINAL;
    virtual OfxStatus deleteAllKeys() OVERRIDE FINAL;
    virtual OfxStatus copyFrom(const OFX::Host::Param::Instance &instance, OfxTime offset, const OfxRangeD* range) OVERRIDE FINAL;

    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);

private:

    void getCustomParamAtTime(double time, std::string &str) const;


    boost::scoped_ptr<OfxCustomInstancePrivate> _imp;
};


class OfxParametricInstance
    : public OfxParamToKnob, public OFX::Host::ParametricParam::ParametricInstance
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    explicit OfxParametricInstance(const OfxEffectInstancePtr& node,
                                   OFX::Host::Param::Descriptor & descriptor);

    virtual ~OfxParametricInstance();

    virtual OfxPluginEntryPoint* getCustomOverlayInteractEntryPoint(const OFX::Host::Param::Instance* param) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
    virtual void setHint() OVERRIDE FINAL;
    // callback which should set enabled state as appropriate
    virtual void setEnabled() OVERRIDE FINAL;

    // callback which should set secret state as appropriate
    virtual void setSecret() OVERRIDE FINAL;

    /// callback which should update label
    virtual void setLabel() OVERRIDE FINAL;

    virtual void setInViewportSecret() OVERRIDE FINAL;

    virtual void setInViewportLabel() OVERRIDE FINAL;

    virtual void setRange() OVERRIDE FINAL;

    /// callback which should set
    virtual void setDisplayRange() OVERRIDE FINAL;

    /// callback which should set evaluate on change
    virtual void setEvaluateOnChange() OVERRIDE FINAL;

    void onCurvesDefaultInitialized();

    ///derived from CurveHolder
    virtual OfxStatus getValue(int curveIndex, OfxTime time, double parametricPosition, double *returnValue) OVERRIDE FINAL;
    virtual OfxStatus getNControlPoints(int curveIndex, double time, int *returnValue) OVERRIDE FINAL;
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
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual OFX::Host::Param::Instance* getOfxParam() OVERRIDE FINAL { return this; }


    KnobParametricWPtr _knob;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_OFXPARAMINSTANCE_H

