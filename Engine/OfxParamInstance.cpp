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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "OfxParamInstance.h"

#include <iostream>
#include <boost/scoped_array.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>
#include <ofxParametricParam.h>
#include "ofxNatron.h"

#include <QDebug>


#include "Engine/AppManager.h"
#include "Global/GlobalDefines.h"
#include "Engine/Knob.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/ViewerInstance.h"
#include "Engine/Curve.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/Format.h"
#include "Engine/Project.h"
#include "Engine/AppInstance.h"

using namespace Natron;


static std::string
getParamLabel(OFX::Host::Param::Instance* param)
{
    std::string label = param->getLabel();

    if ( label.empty() ) {
        label = param->getShortLabel();
    }
    if ( label.empty() ) {
        label = param->getLongLabel();
    }
    if ( label.empty() ) {
        label = param->getName();
    }

    return label;
}


///anonymous namespace to handle keyframes communication support for Ofx plugins
/// in a generalized manner
namespace OfxKeyFrame {
static
OfxStatus
getNumKeys(KnobI* knob,
           unsigned int &nKeys)
{
    int sum = 0;

    if (knob->canAnimate()) {
        for (int i = 0; i < knob->getDimension(); ++i) {
            std::list<std::pair<KnobI*,int> > dependencies;
            if (knob->getExpressionDependencies(i, dependencies)) {
                for (std::list<std::pair<KnobI*,int> >::iterator it = dependencies.begin(); it!=dependencies.end(); ++it) {
                    unsigned int tmp;
                    getNumKeys(it->first, tmp);
                    sum += tmp;
                }
            } else {
                boost::shared_ptr<Curve> curve = knob->getCurve(i);
                assert(curve);
                sum += curve->getKeyFramesCount();
            }
        }
    }
    nKeys =  sum;

    return kOfxStatOK;
}

static
OfxStatus
getKeyTime(boost::shared_ptr<KnobI> knob,
           int nth,
           OfxTime & time)
{
    if (nth < 0) {
        return kOfxStatErrBadIndex;
    }
    int dimension = 0;
    int indexSoFar = 0;
    while ( dimension < knob->getDimension() ) {
        ++dimension;
        int curveKeyFramesCount = knob->getKeyFramesCount(dimension);
        if ( nth >= (int)(curveKeyFramesCount + indexSoFar) ) {
            indexSoFar += curveKeyFramesCount;
            continue;
        } else {
            boost::shared_ptr<Curve> curve = knob->getCurve(dimension);
            assert(curve);
            KeyFrameSet set = curve->getKeyFrames_mt_safe();
            KeyFrameSet::const_iterator it = set.begin();
            while ( it != set.end() ) {
                if (indexSoFar == nth) {
                    time = it->getTime();

                    return kOfxStatOK;
                }
                ++indexSoFar;
                ++it;
            }
        }
    }

    return kOfxStatErrBadIndex;
}

static
OfxStatus
getKeyIndex(boost::shared_ptr<KnobI> knob,
            OfxTime time,
            int direction,
            int & index)
{
    int c = 0;

    for (int i = 0; i < knob->getDimension(); ++i) {
        if (!knob->isAnimated(i)) {
            continue;
        }
        boost::shared_ptr<Curve> curve = knob->getCurve(i);
        assert(curve);
        KeyFrameSet set = curve->getKeyFrames_mt_safe();
        for (KeyFrameSet::const_iterator it = set.begin(); it != set.end(); ++it) {
            if (it->getTime() == time) {
                if (direction == 0) {
                    index = c;
                } else if (direction < 0) {
                    if ( it == set.begin() ) {
                        index = -1;
                    } else {
                        index = c - 1;
                    }
                } else {
                    KeyFrameSet::const_iterator next = it;
                    if (next != set.end()) {
                        ++next;
                    }
                    if ( next != set.end() ) {
                        index = c + 1;
                    } else {
                        index = -1;
                    }
                }

                return kOfxStatOK;
            }
            ++c;
        }
    }

    return kOfxStatFailed;
}

static
OfxStatus
deleteKey(boost::shared_ptr<KnobI> knob,
          OfxTime time)
{
    for (int i = 0; i < knob->getDimension(); ++i) {
        knob->deleteValueAtTime(Natron::eCurveChangeReasonInternal,time, i);
    }

    return kOfxStatOK;
}

static
OfxStatus
deleteAllKeys(boost::shared_ptr<KnobI> knob)
{
    for (int i = 0; i < knob->getDimension(); ++i) {
        knob->removeAnimation(i);
    }

    return kOfxStatOK;
}

// copy one parameter to another, with a range (NULL means to copy all animation)
static
OfxStatus
copyFrom(const boost::shared_ptr<KnobI> & from,
         const boost::shared_ptr<KnobI> &to,
         OfxTime offset,
         const OfxRangeD* range)
{
    ///copy only if type is the same
    if ( from->typeName() == to->typeName() ) {
        to->clone(from,offset,range);
        to->beginChanges();
        int dims = to->getDimension();
        for (int i = 0; i < dims; ++i) {
            to->evaluateValueChange(i, from->getCurrentTime(), Natron::eValueChangedReasonPluginEdited);
        }
        to->endChanges();
    }

    return kOfxStatOK;
}
}

PropertyModified_RAII::PropertyModified_RAII(OfxParamToKnob* h)
: _h(h)
{
    h->setDynamicPropertyModified(true);
}

PropertyModified_RAII::~PropertyModified_RAII()
{
    _h->setDynamicPropertyModified(false);
}

void
OfxParamToKnob::connectDynamicProperties()
{
    boost::shared_ptr<KnobI> knob = getKnob();
    if (!knob) {
        return;
    }
    KnobSignalSlotHandler* handler = knob->getSignalSlotHandler().get();
    if (!handler) {
        return;
    }
    QObject::connect(handler, SIGNAL(descriptionChanged()), this, SLOT(onDescriptionChanged()));
    QObject::connect(handler, SIGNAL(evaluateOnChangeChanged(bool)), this, SLOT(onEvaluateOnChangeChanged(bool)));
    QObject::connect(handler, SIGNAL(secretChanged()), this, SLOT(onSecretChanged()));
    QObject::connect(handler, SIGNAL(enabledChanged()), this, SLOT(onEnabledChanged()));
    QObject::connect(handler, SIGNAL(displayMinMaxChanged(double,double,int)), this, SLOT(onDisplayMinMaxChanged(double,double,int)));
    QObject::connect(handler, SIGNAL(minMaxChanged(double,double,int)), this, SLOT(onMinMaxChanged(double,double,int)));
}

void
OfxParamToKnob::onEvaluateOnChangeChanged(bool evaluate)
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }
    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    param->getProperties().setIntProperty(kOfxParamPropEvaluateOnChange, (int)evaluate);
}

void
OfxParamToKnob::onSecretChanged()
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    boost::shared_ptr<KnobI> knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setIntProperty(kOfxParamPropSecret, (int)knob->getIsSecret());

}

void
OfxParamToKnob::onEnabledChanged()
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    boost::shared_ptr<KnobI> knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setIntProperty(kOfxParamPropEnabled, (int)knob->isEnabled(0));
}

void
OfxParamToKnob::onDescriptionChanged()
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    boost::shared_ptr<KnobI> knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setStringProperty(kOfxPropLabel, knob->getDescription());
}

void
OfxParamToKnob::onDisplayMinMaxChanged(double min,double max, int index)
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    if (hasDoubleMinMaxProps()) {
        param->getProperties().setDoubleProperty(kOfxParamPropDisplayMin, min, index);
        param->getProperties().setDoubleProperty(kOfxParamPropDisplayMax, max, index);
    } else {
        param->getProperties().setIntProperty(kOfxParamPropDisplayMin, (int)min, index);
        param->getProperties().setIntProperty(kOfxParamPropDisplayMax, (int)max, index);
    }
}

void
OfxParamToKnob::onMinMaxChanged(double min,double max, int index)
{
    if (isDynamicPropertyBeingModified()) {
        return;
    }

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    if (hasDoubleMinMaxProps()) {
        param->getProperties().setDoubleProperty(kOfxParamPropMin, min, index);
        param->getProperties().setDoubleProperty(kOfxParamPropMax, max, index);
    } else {
        param->getProperties().setIntProperty(kOfxParamPropMin, (int)min, index);
        param->getProperties().setIntProperty(kOfxParamPropMax, (int)max, index);
    }
}



////////////////////////// OfxPushButtonInstance /////////////////////////////////////////////////

OfxPushButtonInstance::OfxPushButtonInstance(OfxEffectInstance* node,
                                             OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::PushbuttonInstance( descriptor, node->effectInstance() )
{
    boost::shared_ptr<KnobButton> k = Natron::createKnob<KnobButton>( node, getParamLabel(this) );
    _knob = k;
    const std::string & iconFilePath = descriptor.getProperties().getStringProperty(kOfxPropIcon,1);
    k->setIconFilePath(iconFilePath);
}

// callback which should set enabled state as appropriate
void
OfxPushButtonInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxPushButtonInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxPushButtonInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxPushButtonInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI> OfxPushButtonInstance::getKnob() const
{
    return _knob.lock();
}

////////////////////////// OfxIntegerInstance /////////////////////////////////////////////////


OfxIntegerInstance::OfxIntegerInstance(OfxEffectInstance* node,
                                       OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::IntegerInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();

    boost::shared_ptr<KnobInt> k = Natron::createKnob<KnobInt>( node, getParamLabel(this) );
    _knob = k;

    int min = properties.getIntProperty(kOfxParamPropMin);
    int max = properties.getIntProperty(kOfxParamPropMax);
    int def = properties.getIntProperty(kOfxParamPropDefault);
    int displayMin = properties.getIntProperty(kOfxParamPropDisplayMin);
    int displayMax = properties.getIntProperty(kOfxParamPropDisplayMax);
    k->setDisplayMinimum(displayMin);
    k->setDisplayMaximum(displayMax);

    k->setMinimum(min);
    k->setIncrement(1); // kOfxParamPropIncrement only exists for Double
    k->setMaximum(max);
    k->setDefaultValue(def,0);
    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,0);
    k->setDimensionName(0, dimensionName);
}

OfxStatus
OfxIntegerInstance::get(int & v)
{
    v = _knob.lock()->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::get(OfxTime time,
                        int & v)
{
    v = _knob.lock()->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(int v)
{
    _knob.lock()->setValueFromPlugin(v,0);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(OfxTime time,
                        int v)
{
    _knob.lock()->setValueAtTimeFromPlugin(time,v,0);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxIntegerInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxIntegerInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxIntegerInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxIntegerInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI> OfxIntegerInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxIntegerInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxIntegerInstance::getKeyTime(int nth,
                               OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxIntegerInstance::getKeyIndex(OfxTime time,
                                int direction,
                                int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxIntegerInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxIntegerInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxIntegerInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                             OfxTime offset,
                             const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxIntegerInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

void
OfxIntegerInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    int displayMin = getProperties().getIntProperty(kOfxParamPropDisplayMin);
    int displayMax = getProperties().getIntProperty(kOfxParamPropDisplayMax);

    _knob.lock()->setDisplayMinimum(displayMin);
    _knob.lock()->setDisplayMaximum(displayMax);
}

void
OfxIntegerInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    int mini = getProperties().getIntProperty(kOfxParamPropMin);
    int maxi = getProperties().getIntProperty(kOfxParamPropMax);
    
    _knob.lock()->setMinimum(mini);
    _knob.lock()->setMaximum(maxi);
}

////////////////////////// OfxDoubleInstance /////////////////////////////////////////////////


OfxDoubleInstance::OfxDoubleInstance(OfxEffectInstance* node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::DoubleInstance( descriptor,node->effectInstance() )
      , _node(node)
{
    const OFX::Host::Property::Set &properties = getProperties();
    const std::string & coordSystem = getDefaultCoordinateSystem();

    boost::shared_ptr<KnobDouble> dblKnob = Natron::createKnob<KnobDouble>( node, getParamLabel(this) );
    _knob = dblKnob;

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedX) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXAbsolute) ) {
        dblKnob->setNormalizedState(0, KnobDouble::eNormalizedStateX);
    } else if ( (doubleType == kOfxParamDoubleTypeNormalisedY) ||
                ( doubleType == kOfxParamDoubleTypeNormalisedYAbsolute) ) {
        dblKnob->setNormalizedState(0, KnobDouble::eNormalizedStateY);
    }

    double min = properties.getDoubleProperty(kOfxParamPropMin);
    double max = properties.getDoubleProperty(kOfxParamPropMax);
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    double def = properties.getDoubleProperty(kOfxParamPropDefault);
    int decimals = properties.getIntProperty(kOfxParamPropDigits);


    dblKnob->setMinimum(min);
    dblKnob->setMaximum(max);
    setDisplayRange();
    if (incr > 0) {
        dblKnob->setIncrement(incr);
    }
    if (decimals > 0) {
        dblKnob->setDecimals(decimals);
    }

    if (coordSystem == kOfxParamCoordinatesNormalised) {
        // The defaults should be stored as is, not premultiplied by the project size.
        // The fact that the default value is normalized should be stored in Knob or KnobDouble.

        // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
        // and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
        dblKnob->setDefaultValuesNormalized(def);
    } else {
        dblKnob->setDefaultValue(def,0);
    }

    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,0);
    dblKnob->setDimensionName(0, dimensionName);
}

OfxStatus
OfxDoubleInstance::get(double & v)
{
    v = _knob.lock()->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::get(OfxTime time,
                       double & v)
{
    v = _knob.lock()->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(double v)
{
    _knob.lock()->setValueFromPlugin(v,0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(OfxTime time,
                       double v)
{
    _knob.lock()->setValueAtTimeFromPlugin(time,v,0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::derive(OfxTime time,
                          double & v)
{
    v = _knob.lock()->getDerivativeAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::integrate(OfxTime time1,
                             OfxTime time2,
                             double & v)
{
    v = _knob.lock()->getIntegrateFromTimeToTime(time1, time2);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxDoubleInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxDoubleInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxDoubleInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxDoubleInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxDoubleInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    double displayMin = getProperties().getDoubleProperty(kOfxParamPropDisplayMin);
    double displayMax = getProperties().getDoubleProperty(kOfxParamPropDisplayMax);

    _knob.lock()->setDisplayMinimum(displayMin);
    _knob.lock()->setDisplayMaximum(displayMax);
}

void
OfxDoubleInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    double mini = getProperties().getDoubleProperty(kOfxParamPropMin);
    double maxi = getProperties().getDoubleProperty(kOfxParamPropMax);
    
    _knob.lock()->setMinimum(mini);
    _knob.lock()->setMaximum(maxi);
}

boost::shared_ptr<KnobI>
OfxDoubleInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxDoubleInstance::isAnimated() const
{
    return _knob.lock()->isAnimated(0);
}

OfxStatus
OfxDoubleInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxDoubleInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxDoubleInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxDoubleInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxDoubleInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxDoubleInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxDoubleInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxBooleanInstance /////////////////////////////////////////////////

OfxBooleanInstance::OfxBooleanInstance(OfxEffectInstance* node,
                                       OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::BooleanInstance( descriptor,node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();

    boost::shared_ptr<KnobBool> b = Natron::createKnob<KnobBool>( node, getParamLabel(this) );
    _knob = b;
    int def = properties.getIntProperty(kOfxParamPropDefault);
    b->setDefaultValue( (bool)def,0 );
}

OfxStatus
OfxBooleanInstance::get(bool & b)
{
    b = _knob.lock()->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::get(OfxTime time,
                        bool & b)
{
    assert( KnobBool::canAnimateStatic() );
    b = _knob.lock()->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(bool b)
{
    _knob.lock()->setValueFromPlugin(b,0);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(OfxTime time,
                        bool b)
{

    assert( KnobBool::canAnimateStatic() );
    _knob.lock()->setValueAtTimeFromPlugin(time, b, 0);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxBooleanInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxBooleanInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxBooleanInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxBooleanInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI> OfxBooleanInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxBooleanInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxBooleanInstance::getKeyTime(int nth,
                               OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxBooleanInstance::getKeyIndex(OfxTime time,
                                int direction,
                                int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxBooleanInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxBooleanInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxBooleanInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                             OfxTime offset,
                             const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxBooleanInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxChoiceInstance /////////////////////////////////////////////////

OfxChoiceInstance::OfxChoiceInstance(OfxEffectInstance* node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::ChoiceInstance( descriptor,node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();


    boost::shared_ptr<KnobChoice> choice = Natron::createKnob<KnobChoice>( node, getParamLabel(this) );
    _knob = choice;

    
    int dim = getProperties().getDimension(kOfxParamPropChoiceOption);
    int labelOptionDim = getProperties().getDimension(kOfxParamPropChoiceLabelOption);
    
    std::vector<std::string> entires;
    std::vector<std::string> helpStrings;
    bool hashelp = false;
    for (int i = 0; i < dim; ++i) {
        std::string str = getProperties().getStringProperty(kOfxParamPropChoiceOption,i);
        std::string help;
        if (i < labelOptionDim) {
            help = getProperties().getStringProperty(kOfxParamPropChoiceLabelOption,i);
        }
        if ( !help.empty() ) {
            hashelp = true;
        }
        entires.push_back(str);
        helpStrings.push_back(help);
    }
    if (!hashelp) {
        helpStrings.clear();
    }
    choice->populateChoices(entires,helpStrings);

    int def = properties.getIntProperty(kOfxParamPropDefault);
    choice->setDefaultValue(def,0);
    
    bool cascading = properties.getIntProperty(kNatronOfxParamPropChoiceCascading) != 0;
    choice->setCascading(cascading);
    
    bool canAddOptions = (int)properties.getIntProperty(kNatronOfxParamPropChoiceHostCanAddOptions);
    if (canAddOptions) {
        choice->setHostCanAddOptions(true);
    }
}

OfxStatus
OfxChoiceInstance::get(int & v)
{
    v = _knob.lock()->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::get(OfxTime time,
                       int & v)
{
    assert( KnobChoice::canAnimateStatic() );
    v = _knob.lock()->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::set(int v)
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    std::vector<std::string> entries = knob->getEntries_mt_safe();
    if ( (0 <= v) && ( v < (int)entries.size() ) ) {
        knob->setValueFromPlugin(v,0);
        return kOfxStatOK;
    } else {
        return kOfxStatErrBadIndex;
    }
}

OfxStatus
OfxChoiceInstance::set(OfxTime time,
                       int v)
{
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    std::vector<std::string> entries = knob->getEntries_mt_safe();
    if ( (0 <= v) && ( v < (int)entries.size() ) ) {
        knob->setValueAtTimeFromPlugin(time, v, 0);

        return kOfxStatOK;
    } else {
        return kOfxStatErrBadIndex;
    }
}

// callback which should set enabled state as appropriate
void
OfxChoiceInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxChoiceInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxChoiceInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxChoiceInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxChoiceInstance::setOption(int num)
{
    int dim = getProperties().getDimension(kOfxParamPropChoiceOption);
    boost::shared_ptr<KnobChoice> knob = _knob.lock();
    if (dim == 0) {
        knob->resetChoices();
        return;
    }
    //Only 2 kind of behaviours are supported: either resetOptions (dim == 0) or
    //appendOption, hence num == dim -1
    assert(num == dim - 1);
    if (num != (dim -1)) {
        return;
    }
    std::string entry = getProperties().getStringProperty(kOfxParamPropChoiceOption,num);
    std::string help;
    int labelOptionDim = getProperties().getDimension(kOfxParamPropChoiceLabelOption);
    if (num < labelOptionDim) {
        help = getProperties().getStringProperty(kOfxParamPropChoiceLabelOption,num);
    }
    knob->appendChoice(entry,help);
}

boost::shared_ptr<KnobI>
OfxChoiceInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxChoiceInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxChoiceInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxChoiceInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxChoiceInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxChoiceInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxChoiceInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxChoiceInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxRGBAInstance /////////////////////////////////////////////////

OfxRGBAInstance::OfxRGBAInstance(OfxEffectInstance* node,
                                 OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::RGBAInstance( descriptor,node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();

    boost::shared_ptr<KnobColor> color = Natron::createKnob<KnobColor>(node, getParamLabel(this),4);
    _knob = color;

    double defR = properties.getDoubleProperty(kOfxParamPropDefault,0);
    double defG = properties.getDoubleProperty(kOfxParamPropDefault,1);
    double defB = properties.getDoubleProperty(kOfxParamPropDefault,2);
    double defA = properties.getDoubleProperty(kOfxParamPropDefault,3);
    color->setDefaultValue(defR,0);
    color->setDefaultValue(defG,1);
    color->setDefaultValue(defB,2);
    color->setDefaultValue(defA,3);

    const int dims = 4;
    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getDoubleProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getDoubleProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        color->setDimensionName(i, dimensionName);
    }

    color->setMinimumsAndMaximums(minimum, maximum);
    color->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

OfxStatus
OfxRGBAInstance::get(double & r,
                     double & g,
                     double & b,
                     double & a)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getValue(0);
    g = color->getValue(1);
    b = color->getValue(2);
    a = color->getValue(3);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::get(OfxTime time,
                     double &r,
                     double & g,
                     double & b,
                     double & a)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getValueAtTime(time,0);
    g = color->getValueAtTime(time,1);
    b = color->getValueAtTime(time,2);
    a = color->getValueAtTime(time,3);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(double r,
                     double g,
                     double b,
                     double a)
{
    _knob.lock()->setValues(r, g, b, a, Natron::eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(OfxTime time,
                     double r,
                     double g,
                     double b,
                     double a)
{
    _knob.lock()->setValuesAtTime(std::floor(time + 0.5), r, g, b, a, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::derive(OfxTime time,
                        double &r,
                        double & g,
                        double & b,
                        double & a)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getDerivativeAtTime(time,0);
    g = color->getDerivativeAtTime(time,1);
    b = color->getDerivativeAtTime(time,2);
    a = color->getDerivativeAtTime(time,3);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::integrate(OfxTime time1,
                           OfxTime time2,
                           double &r,
                           double & g,
                           double & b,
                           double & a)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getIntegrateFromTimeToTime(time1, time2, 0);
    g = color->getIntegrateFromTimeToTime(time1, time2, 1);
    b = color->getIntegrateFromTimeToTime(time1, time2, 2);
    a = color->getIntegrateFromTimeToTime(time1, time2, 3);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxRGBAInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxRGBAInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxRGBAInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}


void
OfxRGBAInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI>
OfxRGBAInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxRGBAInstance::isAnimated(int dimension) const
{
    return _knob.lock()->isAnimated(dimension);
}

bool
OfxRGBAInstance::isAnimated() const
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    return color->isAnimated(0) || color->isAnimated(1) || color->isAnimated(2) || color->isAnimated(3);
}

OfxStatus
OfxRGBAInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxRGBAInstance::getKeyTime(int nth,
                            OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxRGBAInstance::getKeyIndex(OfxTime time,
                             int direction,
                             int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxRGBAInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxRGBAInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxRGBAInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                          OfxTime offset,
                          const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxRGBAInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxRGBInstance /////////////////////////////////////////////////

OfxRGBInstance::OfxRGBInstance(OfxEffectInstance* node,
                               OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::RGBInstance( descriptor,node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();

    boost::shared_ptr<KnobColor> color  = Natron::createKnob<KnobColor>(node, getParamLabel(this),3);
    _knob = color;

    double defR = properties.getDoubleProperty(kOfxParamPropDefault,0);
    double defG = properties.getDoubleProperty(kOfxParamPropDefault,1);
    double defB = properties.getDoubleProperty(kOfxParamPropDefault,2);
    color->setDefaultValue(defR, 0);
    color->setDefaultValue(defG, 1);
    color->setDefaultValue(defB, 2);

    const int dims = 3;
    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getDoubleProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getDoubleProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        color->setDimensionName(i, dimensionName);
    }

    color->setMinimumsAndMaximums(minimum, maximum);
    color->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

OfxStatus
OfxRGBInstance::get(double & r,
                    double & g,
                    double & b)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getValue(0);
    g = color->getValue(1);
    b = color->getValue(2);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::get(OfxTime time,
                    double & r,
                    double & g,
                    double & b)
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    r = color->getValueAtTime(time,0);
    g = color->getValueAtTime(time,1);
    b = color->getValueAtTime(time,2);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(double r,
                    double g,
                    double b)
{
    _knob.lock()->setValues(r, g, b,  Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(OfxTime time,
                    double r,
                    double g,
                    double b)
{
    _knob.lock()->setValuesAtTime(std::floor(time + 0.5), r, g, b,  Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::derive(OfxTime time,
                       double & r,
                       double & g,
                       double & b)
{
    r = _knob.lock()->getDerivativeAtTime(time,0);
    g = _knob.lock()->getDerivativeAtTime(time,1);
    b = _knob.lock()->getDerivativeAtTime(time,2);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::integrate(OfxTime time1,
                          OfxTime time2,
                          double &r,
                          double & g,
                          double & b)
{
    r = _knob.lock()->getIntegrateFromTimeToTime(time1, time2, 0);
    g = _knob.lock()->getIntegrateFromTimeToTime(time1, time2, 1);
    b = _knob.lock()->getIntegrateFromTimeToTime(time1, time2, 2);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxRGBInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxRGBInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxRGBInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxRGBInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI>
OfxRGBInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxRGBInstance::isAnimated(int dimension) const
{
    return _knob.lock()->isAnimated(dimension);
}

bool
OfxRGBInstance::isAnimated() const
{
    boost::shared_ptr<KnobColor> color = _knob.lock();
    return color->isAnimated(0) || color->isAnimated(1) || color->isAnimated(2);
}

OfxStatus
OfxRGBInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxRGBInstance::getKeyTime(int nth,
                           OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxRGBInstance::getKeyIndex(OfxTime time,
                            int direction,
                            int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxRGBInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxRGBInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxRGBInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                         OfxTime offset,
                         const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(),getKnob(), offset, range);
}

void
OfxRGBInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxDouble2DInstance /////////////////////////////////////////////////

OfxDouble2DInstance::OfxDouble2DInstance(OfxEffectInstance* node,
                                         OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::Double2DInstance( descriptor,node->effectInstance() )
      , _node(node)
{
    const OFX::Host::Property::Set &properties = getProperties();
    const std::string & coordSystem = getDefaultCoordinateSystem();
    const int dims = 2;

    boost::shared_ptr<KnobDouble> dblKnob = Natron::createKnob<KnobDouble>(node, getParamLabel(this),dims);
    _knob = dblKnob;

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ) {
        dblKnob->setNormalizedState(0, KnobDouble::eNormalizedStateX);
        dblKnob->setNormalizedState(1, KnobDouble::eNormalizedStateY);
    }
    
    bool isSpatial = doubleType == kOfxParamDoubleTypeNormalisedXY ||
    doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute ||
    doubleType == kOfxParamDoubleTypeXY ||
    doubleType == kOfxParamDoubleTypeXYAbsolute;

    dblKnob->setSpatial(isSpatial);
    
    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> increment(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    std::vector<int> decimals(dims);
    boost::scoped_array<double> def(new double[dims]);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        increment[i] = incr;
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault,i);

        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        dblKnob->setDimensionName(i, dimensionName);
        
    }
    dblKnob->setMinimumsAndMaximums(minimum, maximum);
    setDisplayRange();
    dblKnob->setIncrement(increment);
    dblKnob->setDecimals(decimals);

    // Only create native overlays if there is no interact or kOfxParamPropUseHostOverlayHandle is set
    // see https://github.com/MrKepzie/Natron/issues/932
    // only create automatic overlay for kOfxParamDoubleTypeXYAbsolute
    if ((!_node->effectInstance()->getOverlayInteractMainEntry() && getDoubleType() == kOfxParamDoubleTypeXYAbsolute) ||
        properties.getIntProperty(kOfxParamPropUseHostOverlayHandle) == 1) {
        dblKnob->setHasNativeOverlayHandle(true);
    }

    if (coordSystem == kOfxParamCoordinatesNormalised) {
        dblKnob->setDefaultValuesNormalized( dims,def.get() );
    } else {
        dblKnob->setDefaultValue(def[0], 0);
        dblKnob->setDefaultValue(def[1], 1);
    }
}

OfxStatus
OfxDouble2DInstance::get(double & x1,
                         double & x2)
{
    boost::shared_ptr<KnobDouble> dblKnob = _knob.lock();
    x1 = dblKnob->getValue(0);
    x2 = dblKnob->getValue(1);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::get(OfxTime time,
                         double & x1,
                         double & x2)
{
    boost::shared_ptr<KnobDouble> dblKnob = _knob.lock();
    x1 = dblKnob->getValueAtTime(time,0);
    x2 = dblKnob->getValueAtTime(time,1);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(double x1,
                         double x2)
{
    _knob.lock()->setValues(x1, x2, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(OfxTime time,
                         double x1,
                         double x2)
{
    
    _knob.lock()->setValuesAtTime(time, x1, x2, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::derive(OfxTime time,
                            double &x1,
                            double & x2)
{
    boost::shared_ptr<KnobDouble> dblKnob = _knob.lock();
    x1 = dblKnob->getDerivativeAtTime(time,0);
    x2 = dblKnob->getDerivativeAtTime(time,1);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2)
{
    boost::shared_ptr<KnobDouble> dblKnob = _knob.lock();
    x1 = dblKnob->getIntegrateFromTimeToTime(time1, time2, 0);
    x2 = dblKnob->getIntegrateFromTimeToTime(time1, time2, 1);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxDouble2DInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxDouble2DInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxDouble2DInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}


void
OfxDouble2DInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxDouble2DInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<double> displayMins(2);
    std::vector<double> displayMaxs(2);

    displayMins[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,0);
    displayMins[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,1);
    displayMaxs[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,0);
    displayMaxs[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,1);
    _knob.lock()->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxDouble2DInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<double> displayMins(2);
    std::vector<double> displayMaxs(2);
    
    displayMins[0] = getProperties().getDoubleProperty(kOfxParamPropMin,0);
    displayMins[1] = getProperties().getDoubleProperty(kOfxParamPropMin,1);
    displayMaxs[0] = getProperties().getDoubleProperty(kOfxParamPropMax,0);
    displayMaxs[1] = getProperties().getDoubleProperty(kOfxParamPropMax,1);
    _knob.lock()->setMinimumsAndMaximums(displayMins, displayMaxs);

}


boost::shared_ptr<KnobI>
OfxDouble2DInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxDouble2DInstance::isAnimated(int dimension) const
{
    return _knob.lock()->isAnimated(dimension);
}

bool
OfxDouble2DInstance::isAnimated() const
{
    boost::shared_ptr<KnobDouble> dblKnob = _knob.lock();
    return dblKnob->isAnimated(0) || dblKnob->isAnimated(1);
}

OfxStatus
OfxDouble2DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxDouble2DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxDouble2DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxDouble2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxDouble2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxDouble2DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                              OfxTime offset,
                              const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxDouble2DInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxInteger2DInstance /////////////////////////////////////////////////

OfxInteger2DInstance::OfxInteger2DInstance(OfxEffectInstance* node,
                                           OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::Integer2DInstance( descriptor,node->effectInstance() )
      , _node(node)
{
    const int dims = 2;
    const OFX::Host::Property::Set &properties = getProperties();


    boost::shared_ptr<KnobInt> iKnob = Natron::createKnob<KnobInt>(node, getParamLabel(this), dims);
    _knob = iKnob;

    std::vector<int> minimum(dims);
    std::vector<int> maximum(dims);
    std::vector<int> increment(dims);
    std::vector<int> displayMins(dims);
    std::vector<int> displayMaxs(dims);
    boost::scoped_array<int> def(new int[dims]);

    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getIntProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getIntProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getIntProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getIntProperty(kOfxParamPropMax,i);
        increment[i] = 1; // kOfxParamPropIncrement only exists for Double
        def[i] = properties.getIntProperty(kOfxParamPropDefault,i);
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        iKnob->setDimensionName(i, dimensionName);
    }

    iKnob->setMinimumsAndMaximums(minimum, maximum);
    iKnob->setIncrement(increment);
    iKnob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    iKnob->setDefaultValue(def[0], 0);
    iKnob->setDefaultValue(def[1], 1);
}

OfxStatus
OfxInteger2DInstance::get(int & x1,
                          int & x2)
{
    boost::shared_ptr<KnobInt> iKnob = _knob.lock();
    x1 = iKnob->getValue(0);
    x2 = iKnob->getValue(1);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::get(OfxTime time,
                          int & x1,
                          int & x2)
{
    boost::shared_ptr<KnobInt> iKnob = _knob.lock();
    x1 = iKnob->getValueAtTime(time,0);
    x2 = iKnob->getValueAtTime(time,1);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(int x1,
                          int x2)
{
    _knob.lock()->setValues(x1, x2 , Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(OfxTime time,
                          int x1,
                          int x2)
{
    _knob.lock()->setValuesAtTime(time, x1, x2 , Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxInteger2DInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxInteger2DInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxInteger2DInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}


void
OfxInteger2DInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<int> displayMins(2);
    std::vector<int> displayMaxs(2);
    
    displayMins[0] = getProperties().getIntProperty(kOfxParamPropDisplayMin,0);
    displayMins[1] = getProperties().getIntProperty(kOfxParamPropDisplayMin,1);
    displayMaxs[0] = getProperties().getIntProperty(kOfxParamPropDisplayMax,0);
    displayMaxs[1] = getProperties().getIntProperty(kOfxParamPropDisplayMax,1);
    _knob.lock()->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    
}

void
OfxInteger2DInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    
    std::vector<int> displayMins(2);
    std::vector<int> displayMaxs(2);
    
    displayMins[0] = getProperties().getIntProperty(kOfxParamPropMin,0);
    displayMins[1] = getProperties().getIntProperty(kOfxParamPropMin,1);
    displayMaxs[0] = getProperties().getIntProperty(kOfxParamPropMax,0);
    displayMaxs[1] = getProperties().getIntProperty(kOfxParamPropMax,1);
    _knob.lock()->setMinimumsAndMaximums(displayMins, displayMaxs);
    
}

void
OfxInteger2DInstance::setEvaluateOnChange()
{
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI>
OfxInteger2DInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxInteger2DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxInteger2DInstance::getKeyTime(int nth,
                                 OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxInteger2DInstance::getKeyIndex(OfxTime time,
                                  int direction,
                                  int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxInteger2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxInteger2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxInteger2DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                               OfxTime offset,
                               const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxInteger2DInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxDouble3DInstance /////////////////////////////////////////////////

OfxDouble3DInstance::OfxDouble3DInstance(OfxEffectInstance* node,
                                         OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::Double3DInstance( descriptor,node->effectInstance() )
      , _node(node)
{
    const int dims = 3;
    const OFX::Host::Property::Set &properties = getProperties();


    boost::shared_ptr<KnobDouble> knob = Natron::createKnob<KnobDouble>(node, getParamLabel(this),dims);
    _knob = knob;

    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> increment(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    std::vector<int> decimals(dims);
    boost::scoped_array<double> def(new double[dims]);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getDoubleProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getDoubleProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        increment[i] = incr;
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault,i);
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        knob->setDimensionName(i, dimensionName);
    }

    knob->setMinimumsAndMaximums(minimum, maximum);
    knob->setIncrement(increment);
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    knob->setDecimals(decimals);
    knob->setDefaultValue(def[0],0);
    knob->setDefaultValue(def[1],1);
    knob->setDefaultValue(def[2],2);
}

OfxStatus
OfxDouble3DInstance::get(double & x1,
                         double & x2,
                         double & x3)
{
    boost::shared_ptr<KnobDouble> knob = _knob.lock();
    x1 = knob->getValue(0);
    x2 = knob->getValue(1);
    x3 = knob->getValue(2);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::get(OfxTime time,
                         double & x1,
                         double & x2,
                         double & x3)
{
    boost::shared_ptr<KnobDouble> knob = _knob.lock();
    x1 = knob->getValueAtTime(time,0);
    x2 = knob->getValueAtTime(time,1);
    x3 = knob->getValueAtTime(time,2);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(double x1,
                         double x2,
                         double x3)
{
    
    _knob.lock()->setValues(x1, x2 , x3, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(OfxTime time,
                         double x1,
                         double x2,
                         double x3)
{
    _knob.lock()->setValuesAtTime(time, x1, x2 , x3, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::derive(OfxTime time,
                            double & x1,
                            double & x2,
                            double & x3)
{
    boost::shared_ptr<KnobDouble> knob = _knob.lock();
    x1 = knob->getDerivativeAtTime(time,0);
    x2 = knob->getDerivativeAtTime(time,1);
    x3 = knob->getDerivativeAtTime(time,2);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2,
                               double & x3)
{
    boost::shared_ptr<KnobDouble> knob = _knob.lock();
    x1 = knob->getIntegrateFromTimeToTime(time1, time2, 0);
    x2 = knob->getIntegrateFromTimeToTime(time1, time2, 1);
    x3 = knob->getIntegrateFromTimeToTime(time1, time2, 2);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxDouble3DInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxDouble3DInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxDouble3DInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}


void
OfxDouble3DInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<double> displayMins(3);
    std::vector<double> displayMaxs(3);
    
    
    displayMins[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,0);
    displayMins[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,1);
    displayMins[2] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,2);
    displayMaxs[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,0);
    displayMaxs[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,1);
    displayMaxs[2] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,2);
    _knob.lock()->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxDouble3DInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<double> displayMins(3);
    std::vector<double> displayMaxs(3);
    
    displayMins[0] = getProperties().getDoubleProperty(kOfxParamPropMin,0);
    displayMins[1] = getProperties().getDoubleProperty(kOfxParamPropMin,1);
    displayMins[2] = getProperties().getDoubleProperty(kOfxParamPropMin,2);
    displayMaxs[0] = getProperties().getDoubleProperty(kOfxParamPropMax,0);
    displayMaxs[1] = getProperties().getDoubleProperty(kOfxParamPropMax,1);
    displayMaxs[2] = getProperties().getDoubleProperty(kOfxParamPropMax,2);
    _knob.lock()->setMinimumsAndMaximums(displayMins, displayMaxs);
    
}

void
OfxDouble3DInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI>
OfxDouble3DInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxDouble3DInstance::isAnimated(int dimension) const
{
    return _knob.lock()->isAnimated(dimension);
}

bool
OfxDouble3DInstance::isAnimated() const
{
    boost::shared_ptr<KnobDouble> knob = _knob.lock();
    return knob->isAnimated(0) || knob->isAnimated(1) || knob->isAnimated(2);
}

OfxStatus
OfxDouble3DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxDouble3DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxDouble3DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxDouble3DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxDouble3DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxDouble3DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                              OfxTime offset,
                              const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxDouble3DInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxInteger3DInstance /////////////////////////////////////////////////

OfxInteger3DInstance::OfxInteger3DInstance(OfxEffectInstance*node,
                                           OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::Integer3DInstance( descriptor,node->effectInstance() )
      , _node(node)
{
    const int dims = 3;
    const OFX::Host::Property::Set &properties = getProperties();


    boost::shared_ptr<KnobInt> knob = Natron::createKnob<KnobInt>(node, getParamLabel(this), dims);
    _knob = knob;

    std::vector<int> minimum(dims);
    std::vector<int> maximum(dims);
    std::vector<int> increment(dims);
    std::vector<int> displayMins(dims);
    std::vector<int> displayMaxs(dims);
    boost::scoped_array<int> def(new int[dims]);

    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getIntProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getIntProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getIntProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getIntProperty(kOfxParamPropMax,i);
        int incr = properties.getIntProperty(kOfxParamPropIncrement,i);
        increment[i] = incr != 0 ?  incr : 1;
        def[i] = properties.getIntProperty(kOfxParamPropDefault,i);

        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel,i);
        knob->setDimensionName(i, dimensionName);
    }

    knob->setMinimumsAndMaximums(minimum, maximum);
    knob->setIncrement(increment);
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    knob->setDefaultValue(def[0],0);
    knob->setDefaultValue(def[1],1);
    knob->setDefaultValue(def[2],2);
}

OfxStatus
OfxInteger3DInstance::get(int & x1,
                          int & x2,
                          int & x3)
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();
    x1 = knob->getValue(0);
    x2 = knob->getValue(1);
    x3 = knob->getValue(2);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::get(OfxTime time,
                          int & x1,
                          int & x2,
                          int & x3)
{
    boost::shared_ptr<KnobInt> knob = _knob.lock();
    x1 = knob->getValueAtTime(time,0);
    x2 = knob->getValueAtTime(time,1);
    x3 = knob->getValueAtTime(time,2);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(int x1,
                          int x2,
                          int x3)
{
   
    _knob.lock()->setValues(x1, x2 , x3, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(OfxTime time,
                          int x1,
                          int x2,
                          int x3)
{
    _knob.lock()->setValuesAtTime(time, x1, x2 , x3, Natron::eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxInteger3DInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxInteger3DInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxInteger3DInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}


void
OfxInteger3DInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<int> displayMins(3);
    std::vector<int> displayMaxs(3);
    
    displayMins[0] = getProperties().getIntProperty(kOfxParamPropDisplayMin,0);
    displayMins[1] = getProperties().getIntProperty(kOfxParamPropDisplayMin,1);
    displayMins[2] = getProperties().getIntProperty(kOfxParamPropDisplayMin,2);
    displayMaxs[0] = getProperties().getIntProperty(kOfxParamPropDisplayMax,0);
    displayMaxs[1] = getProperties().getIntProperty(kOfxParamPropDisplayMax,1);
    displayMaxs[2] = getProperties().getIntProperty(kOfxParamPropDisplayMax,2);
    _knob.lock()->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    
}

void
OfxInteger3DInstance::setRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    std::vector<int> displayMins(3);
    std::vector<int> displayMaxs(3);
    
    displayMins[0] = getProperties().getIntProperty(kOfxParamPropMin,0);
    displayMins[1] = getProperties().getIntProperty(kOfxParamPropMin,1);
    displayMins[2] = getProperties().getIntProperty(kOfxParamPropMin,2);
    displayMaxs[0] = getProperties().getIntProperty(kOfxParamPropMax,0);
    displayMaxs[1] = getProperties().getIntProperty(kOfxParamPropMax,1);
    displayMaxs[2] = getProperties().getIntProperty(kOfxParamPropMax,2);
    _knob.lock()->setMinimumsAndMaximums(displayMins, displayMaxs);
    
}

void
OfxInteger3DInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

boost::shared_ptr<KnobI>
OfxInteger3DInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxInteger3DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxInteger3DInstance::getKeyTime(int nth,
                                 OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxInteger3DInstance::getKeyIndex(OfxTime time,
                                  int direction,
                                  int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxInteger3DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxInteger3DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxInteger3DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                               OfxTime offset,
                               const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxInteger3DInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxGroupInstance /////////////////////////////////////////////////

OfxGroupInstance::OfxGroupInstance(OfxEffectInstance* node,
                                   OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::GroupInstance( descriptor,node->effectInstance() )
      , _groupKnob()
{
    const OFX::Host::Property::Set &properties = getProperties();
    int isTab = properties.getIntProperty(kFnOfxParamPropGroupIsTab);

    _groupKnob = Natron::createKnob<KnobGroup>( node, getParamLabel(this) );
    int opened = properties.getIntProperty(kOfxParamPropGroupOpen);
    if (isTab) {
        _groupKnob.lock()->setAsTab();
    }
    _groupKnob.lock()->setDefaultValue(opened,0);
}

void
OfxGroupInstance::addKnob(boost::shared_ptr<KnobI> k)
{
    _groupKnob.lock()->addKnob(k);
}

boost::shared_ptr<KnobI> OfxGroupInstance::getKnob() const
{
    return _groupKnob.lock();
}

void
OfxGroupInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _groupKnob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxGroupInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _groupKnob.lock()->setSecret( getSecret() );
}

void
OfxGroupInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _groupKnob.lock()->setDescription(getParamLabel(this));
}

////////////////////////// OfxPageInstance /////////////////////////////////////////////////


OfxPageInstance::OfxPageInstance(OfxEffectInstance* node,
                                 OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::PageInstance( descriptor,node->effectInstance() )
      , _pageKnob()
{
    _pageKnob = Natron::createKnob<KnobPage>( node, getParamLabel(this) );
}

// callback which should set enabled state as appropriate
void
OfxPageInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _pageKnob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxPageInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _pageKnob.lock()->setAllDimensionsEnabled( getSecret() );
}

void
OfxPageInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _pageKnob.lock()->setDescription(getParamLabel(this));
}

boost::shared_ptr<KnobI> OfxPageInstance::getKnob() const
{
    return _pageKnob.lock();
}

////////////////////////// OfxStringInstance /////////////////////////////////////////////////


OfxStringInstance::OfxStringInstance(OfxEffectInstance* node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::StringInstance( descriptor,node->effectInstance() )
      , _node(node)
      , _fileKnob()
      , _outputFileKnob()
      , _stringKnob()
      , _pathKnob()
{
    const OFX::Host::Property::Set &properties = getProperties();
    std::string mode = properties.getStringProperty(kOfxParamPropStringMode);
    bool richText = mode == kOfxParamStringIsRichTextFormat;

    if (mode == kOfxParamStringIsFilePath) {
        int fileIsImage = ((node->isReader() ||
                            node->isWriter() ) &&
                           (getScriptName() == kOfxImageEffectFileParamName ||
                            getScriptName() == kOfxImageEffectProxyParamName));
        int fileIsOutput = !properties.getIntProperty(kOfxParamPropStringFilePathExists);
        int filePathSupportsImageSequences = getCanAnimate();


        if (!fileIsOutput) {
            _fileKnob = Natron::createKnob<KnobFile>( node, getParamLabel(this) );
            if (fileIsImage) {
                _fileKnob.lock()->setAsInputImage();
            }
            if (!filePathSupportsImageSequences) {
                _fileKnob.lock()->setAnimationEnabled(false);
            }
        } else {
            _outputFileKnob = Natron::createKnob<KnobOutputFile>( node, getParamLabel(this) );
            if (fileIsImage) {
                _outputFileKnob.lock()->setAsOutputImageFile();
            } else {
                _outputFileKnob.lock()->turnOffSequences();
            }
            if (!filePathSupportsImageSequences) {
                _outputFileKnob.lock()->setAnimationEnabled(false);
            }
        }

    } else if (mode == kOfxParamStringIsDirectoryPath) {
        _pathKnob = Natron::createKnob<KnobPath>( node, getParamLabel(this) );
        _pathKnob.lock()->setMultiPath(false);
        
    } else if ( (mode == kOfxParamStringIsSingleLine) || (mode == kOfxParamStringIsLabel) || (mode == kOfxParamStringIsMultiLine) || richText ) {
        _stringKnob = Natron::createKnob<KnobString>( node, getParamLabel(this) );
        if (mode == kOfxParamStringIsLabel) {
            _stringKnob.lock()->setAllDimensionsEnabled(false);
            _stringKnob.lock()->setAsLabel();
        }
        if ( (mode == kOfxParamStringIsMultiLine) || richText ) {
            ///only QTextArea support rich text anyway
            _stringKnob.lock()->setUsesRichText(richText);
            _stringKnob.lock()->setAsMultiLine();
        }
    }
    std::string defaultVal = properties.getStringProperty(kOfxParamPropDefault).c_str();
    if ( !defaultVal.empty() ) {
        if (_fileKnob.lock()) {
            projectEnvVar_setProxy(defaultVal);
            _fileKnob.lock()->setDefaultValue(defaultVal,0);
        } else if (_outputFileKnob.lock()) {
            projectEnvVar_setProxy(defaultVal);
            _outputFileKnob.lock()->setDefaultValue(defaultVal,0);
        } else if (_stringKnob.lock()) {
            _stringKnob.lock()->setDefaultValue(defaultVal,0);
        } else if (_pathKnob.lock()) {
            projectEnvVar_setProxy(defaultVal);
            _pathKnob.lock()->setDefaultValue(defaultVal,0);
        }
    }
}


void
OfxStringInstance::projectEnvVar_getProxy(std::string& str) const
{
    _node->getApp()->getProject()->canonicalizePath(str);
}

void
OfxStringInstance::projectEnvVar_setProxy(std::string& str) const
{
    _node->getApp()->getProject()->simplifyPath(str);
   
}

OfxStatus
OfxStringInstance::get(std::string &str)
{
    assert( _node->effectInstance() );
    int currentFrame = _node->getApp()->getTimeLine()->currentFrame();
    if (_fileKnob.lock()) {
        str = _fileKnob.lock()->getFileName(currentFrame);
        projectEnvVar_getProxy(str);
    } else if (_outputFileKnob.lock()) {
        str = _outputFileKnob.lock()->generateFileNameAtTime(currentFrame).toStdString();
        projectEnvVar_getProxy(str);
    } else if (_stringKnob.lock()) {
        str = _stringKnob.lock()->getValueAtTime(currentFrame,0);
    } else if (_pathKnob.lock()) {
        str = _pathKnob.lock()->getValue();
        projectEnvVar_getProxy(str);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::get(OfxTime time,
                       std::string & str)
{
    assert( _node->effectInstance() );
    if (_fileKnob.lock()) {
        str = _fileKnob.lock()->getFileName(std::floor(time + 0.5));
        projectEnvVar_getProxy(str);
    } else if (_outputFileKnob.lock()) {
        str = _outputFileKnob.lock()->generateFileNameAtTime(time).toStdString();
        projectEnvVar_getProxy(str);
    } else if (_stringKnob.lock()) {
        str = _stringKnob.lock()->getValueAtTime(std::floor(time + 0.5), 0);
    } else if (_pathKnob.lock()) {
        str = _pathKnob.lock()->getValue();
        projectEnvVar_getProxy(str);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(const char* str)
{
    if (_fileKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _fileKnob.lock()->setValueFromPlugin(s,0);
    }
    if (_outputFileKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _outputFileKnob.lock()->setValueFromPlugin(s,0);
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setValueFromPlugin(str,0);
    }
    if (_pathKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _pathKnob.lock()->setValueFromPlugin(s,0);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(OfxTime time,
                       const char* str)
{

    assert( !KnobString::canAnimateStatic() );
    if (_fileKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _fileKnob.lock()->setValueAtTimeFromPlugin(time,s,0);
    }
    if (_outputFileKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _outputFileKnob.lock()->setValueAtTimeFromPlugin(time,s,0);
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setValueAtTimeFromPlugin( (int)time,str,0 );
    }
    if (_pathKnob.lock()) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        _pathKnob.lock()->setValueAtTimeFromPlugin(time,s,0);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::getV(va_list arg)
{
    const char **value = va_arg(arg, const char **);
    OfxStatus stat;

    std::string& tls = _localString.localData();
    stat = get(tls);

    *value = tls.c_str();

    return stat;
}

OfxStatus
OfxStringInstance::getV(OfxTime time,
                        va_list arg)
{
    const char **value = va_arg(arg, const char **);
    OfxStatus stat;

    std::string& tls = _localString.localData();
    stat = get( time,tls);
    *value = tls.c_str();

    return stat;
}

boost::shared_ptr<KnobI>
OfxStringInstance::getKnob() const
{
    if (_fileKnob.lock()) {
        return _fileKnob.lock();
    }
    if (_outputFileKnob.lock()) {
        return _outputFileKnob.lock();
    }
    if (_stringKnob.lock()) {
        return _stringKnob.lock();
    }
    if (_pathKnob.lock()) {
        return _pathKnob.lock();
    }

    return boost::shared_ptr<KnobI>();
}

// callback which should set enabled state as appropriate
void
OfxStringInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    if (_fileKnob.lock()) {
        _fileKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if (_outputFileKnob.lock()) {
        _outputFileKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if (_pathKnob.lock()) {
        _pathKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
}

void
OfxStringInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    if (_fileKnob.lock()) {
        _fileKnob.lock()->setDescription(getParamLabel(this));
    }
    if (_outputFileKnob.lock()) {
        _outputFileKnob.lock()->setDescription(getParamLabel(this));
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setDescription(getParamLabel(this));
    }
    if (_pathKnob.lock()) {
        _pathKnob.lock()->setDescription(getParamLabel(this));
    }
}

// callback which should set secret state as appropriate
void
OfxStringInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    if (_fileKnob.lock()) {
        _fileKnob.lock()->setSecret( getSecret() );
    }
    if (_outputFileKnob.lock()) {
        _outputFileKnob.lock()->setSecret( getSecret() );
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setSecret( getSecret() );
    }
    if (_pathKnob.lock()) {
        _pathKnob.lock()->setSecret( getSecret() );
    }
}

void
OfxStringInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    if (_fileKnob.lock()) {
        _fileKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if (_outputFileKnob.lock()) {
        _outputFileKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if (_stringKnob.lock()) {
        _stringKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if (_pathKnob.lock()) {
        _pathKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
}

OfxStatus
OfxStringInstance::getNumKeys(unsigned int &nKeys) const
{
    boost::shared_ptr<KnobI> knob;

    if (_stringKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_stringKnob.lock());
    } else if (_fileKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_fileKnob.lock());
    } else {
        return nKeys = 0;
    }

    return OfxKeyFrame::getNumKeys(knob.get(), nKeys);
}

OfxStatus
OfxStringInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    boost::shared_ptr<KnobI> knob;

    if (_stringKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_stringKnob.lock());
    } else if (_fileKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_fileKnob.lock());
    } else {
        return kOfxStatErrBadIndex;
    }

    return OfxKeyFrame::getKeyTime(knob, nth, time);
}

OfxStatus
OfxStringInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    boost::shared_ptr<KnobI> knob;

    if (_stringKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_stringKnob.lock());
    } else if (_fileKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_fileKnob.lock());
    } else {
        return kOfxStatFailed;
    }

    return OfxKeyFrame::getKeyIndex(knob, time, direction, index);
}

OfxStatus
OfxStringInstance::deleteKey(OfxTime time)
{
    boost::shared_ptr<KnobI> knob;

    if (_stringKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_stringKnob.lock());
    } else if (_fileKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_fileKnob.lock());
    } else {
        return kOfxStatErrBadIndex;
    }

    return OfxKeyFrame::deleteKey(knob, time);
}

OfxStatus
OfxStringInstance::deleteAllKeys()
{
    boost::shared_ptr<KnobI> knob;

    if (_stringKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_stringKnob.lock());
    } else if (_fileKnob.lock()) {
        knob = boost::dynamic_pointer_cast<KnobI>(_fileKnob.lock());
    } else {
        return kOfxStatOK;
    }

    return OfxKeyFrame::deleteAllKeys(knob);
}

OfxStatus
OfxStringInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxStringInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    ///This assert might crash Natron when reading a project made with a version
    ///of Natron prior to 0.96 when file params still had keyframes.
    //assert(l == Natron::eAnimationLevelNone || getCanAnimate());
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxCustomInstance /////////////////////////////////////////////////


/*
   http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamTypeCustom

   Custom parameters contain null terminated char * C strings, and may animate. They are designed to provide plugins with a way of storing data that is too complicated or impossible to store in a set of ordinary parameters.

   If a custom parameter animates, it must set its kOfxParamPropCustomInterpCallbackV1 property, which points to a OfxCustomParamInterpFuncV1 function. This function is used to interpolate keyframes in custom params.

   Custom parameters have no interface by default. However,

 * if they animate, the host's animation sheet/editor should present a keyframe/curve representation to allow positioning of keys and control of interpolation. The 'normal' (ie: paged or hierarchical) interface should not show any gui.
 * if the custom param sets its kOfxParamPropInteractV1 property, this should be used by the host in any normal (ie: paged or hierarchical) interface for the parameter.

   Custom parameters are mandatory, as they are simply ASCII C strings. However, animation of custom parameters an support for an in editor interact is optional.
 */


OfxCustomInstance::OfxCustomInstance(OfxEffectInstance* node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::Param::CustomInstance( descriptor,node->effectInstance() )
      , _node(node)
      , _knob()
      , _customParamInterpolationV1Entry(0)
{
    const OFX::Host::Property::Set &properties = getProperties();


    boost::shared_ptr<KnobString> knob = Natron::createKnob<KnobString>( node, getParamLabel(this) );
    _knob = knob;

    knob->setAsCustom();

    knob->setDefaultValue(properties.getStringProperty(kOfxParamPropDefault),0);

    _customParamInterpolationV1Entry = (customParamInterpolationV1Entry_t)properties.getPointerProperty(kOfxParamPropCustomInterpCallbackV1);
    if (_customParamInterpolationV1Entry) {
        knob->setCustomInterpolation( _customParamInterpolationV1Entry, (void*)getHandle() );
    }
}

OfxStatus
OfxCustomInstance::get(std::string &str)
{
    assert( _node->effectInstance() );
    int currentFrame = (int)_node->effectInstance()->timeLineGetTime();
    str = _knob.lock()->getValueAtTime(currentFrame, 0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::get(OfxTime time,
                       std::string & str)
{
    assert( KnobString::canAnimateStatic() );
    // it should call _customParamInterpolationV1Entry
    assert( _node->effectInstance() );
    str = _knob.lock()->getValueAtTime(time, 0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(const char* str)
{
    _knob.lock()->setValueFromPlugin(str,0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(OfxTime time,
                       const char* str)
{

    assert( KnobString::canAnimateStatic() );
    _knob.lock()->setValueAtTimeFromPlugin(time,str,0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::getV(va_list arg)
{
    const char **value = va_arg(arg, const char **);
    OfxStatus stat = get( _localString.localData() );

    *value = _localString.localData().c_str();

    return stat;
}

OfxStatus
OfxCustomInstance::getV(OfxTime time,
                        va_list arg)
{
    const char **value = va_arg(arg, const char **);
    OfxStatus stat = get( time,_localString.localData() );

    *value = _localString.localData().c_str();

    return stat;
}

boost::shared_ptr<KnobI> OfxCustomInstance::getKnob() const
{
    return _knob.lock();
}

// callback which should set enabled state as appropriate
void
OfxCustomInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxCustomInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxCustomInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription(getParamLabel(this));
}

void
OfxCustomInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

OfxStatus
OfxCustomInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys);
}

OfxStatus
OfxCustomInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time);
}

OfxStatus
OfxCustomInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index);
}

OfxStatus
OfxCustomInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time);
}

OfxStatus
OfxCustomInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock());
}

OfxStatus
OfxCustomInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

void
OfxCustomInstance::onKnobAnimationLevelChanged(int,int lvl)
{
    Natron::AnimationLevelEnum l = (Natron::AnimationLevelEnum)lvl;

    assert( l == Natron::eAnimationLevelNone || getCanAnimate() );
    getProperties().setIntProperty(kOfxParamPropIsAnimating, l != Natron::eAnimationLevelNone);
    getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == Natron::eAnimationLevelInterpolatedValue);
}

////////////////////////// OfxParametricInstance /////////////////////////////////////////////////

OfxParametricInstance::OfxParametricInstance(OfxEffectInstance* node,
                                             OFX::Host::Param::Descriptor & descriptor)
    : OFX::Host::ParametricParam::ParametricInstance( descriptor,node->effectInstance() )
      , _descriptor(descriptor)
      , _overlayInteract(NULL)
      , _effect(node)
{
    const OFX::Host::Property::Set &properties = getProperties();
    int parametricDimension = properties.getIntProperty(kOfxParamPropParametricDimension);


    boost::shared_ptr<KnobParametric> knob = Natron::createKnob<KnobParametric>(node, getParamLabel(this),parametricDimension);
    _knob = knob;

    setLabel(); //set label on all curves

    std::vector<double> color(3 * parametricDimension);
    properties.getDoublePropertyN(kOfxParamPropParametricUIColour, &color[0], 3 * parametricDimension);

    for (int i = 0; i < parametricDimension; ++i) {
        knob->setCurveColor(i, color[i * 3], color[i * 3 + 1], color[i * 3 + 2]);
    }

    QObject::connect( knob.get(),SIGNAL( mustInitializeOverlayInteract(OverlaySupport*) ),this,SLOT( initializeInteract(OverlaySupport*) ) );
    setDisplayRange();
}

void
OfxParametricInstance::onCurvesDefaultInitialized()
{
    _knob.lock()->setDefaultCurvesFromCurves();
}

void
OfxParametricInstance::initializeInteract(OverlaySupport* widget)
{
    OfxPluginEntryPoint* interactEntryPoint = (OfxPluginEntryPoint*)getProperties().getPointerProperty(kOfxParamPropParametricInteractBackground);

    if (interactEntryPoint) {
        _overlayInteract = new Natron::OfxOverlayInteract( ( *_effect->effectInstance() ),8,true );
        _overlayInteract->setCallingViewport(widget);
        _overlayInteract->createInstanceAction();
        QObject::connect( _knob.lock().get(), SIGNAL( customBackgroundRequested() ), this, SLOT( onCustomBackgroundDrawingRequested() ) );
    }
}

OfxParametricInstance::~OfxParametricInstance()
{
    delete _overlayInteract;
}

boost::shared_ptr<KnobI> OfxParametricInstance::getKnob() const
{
    return _knob.lock();
}

// callback which should set enabled state as appropriate
void
OfxParametricInstance::setEnabled()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxParametricInstance::setSecret()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setSecret( getSecret() );
}

void
OfxParametricInstance::setEvaluateOnChange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

/// callback which should update label
void
OfxParametricInstance::setLabel()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    _knob.lock()->setDescription( getParamLabel(this) );
    for (int i = 0; i < _knob.lock()->getDimension(); ++i) {
        const std::string & curveName = getProperties().getStringProperty(kOfxParamPropDimensionLabel,i);
        _knob.lock()->setDimensionName(i, curveName);
    }
}

void
OfxParametricInstance::setDisplayRange()
{
    SET_DYNAMIC_PROPERTY_EDITED();
    double range_min = getProperties().getDoubleProperty(kOfxParamPropParametricRange,0);
    double range_max = getProperties().getDoubleProperty(kOfxParamPropParametricRange,1);

    assert(range_max > range_min);

    _knob.lock()->setParametricRange(range_min, range_max);
}

OfxStatus
OfxParametricInstance::getValue(int curveIndex,
                                OfxTime /*time*/,
                                double parametricPosition,
                                double *returnValue)
{
    Natron::StatusEnum stat = _knob.lock()->getValue(curveIndex, parametricPosition, returnValue);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::getNControlPoints(int curveIndex,
                                         double /*time*/,
                                         int *returnValue)
{
    Natron::StatusEnum stat = _knob.lock()->getNControlPoints(curveIndex, returnValue);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::getNthControlPoint(int curveIndex,
                                          double /*time*/,
                                          int nthCtl,
                                          double *key,
                                          double *value)
{
    Natron::StatusEnum stat = _knob.lock()->getNthControlPoint(curveIndex, nthCtl, key, value);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::setNthControlPoint(int curveIndex,
                                          double /*time*/,
                                          int nthCtl,
                                          double key,
                                          double value,
                                          bool /*addAnimationKey*/)
{
    Natron::StatusEnum stat = _knob.lock()->setNthControlPoint(curveIndex, nthCtl, key, value);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::addControlPoint(int curveIndex,
                                       double time,
                                       double key,
                                       double value,
                                       bool /* addAnimationKey*/)
{
    if (time != time || // check for NaN
        boost::math::isinf(time) ||
        key != key || // check for NaN
        boost::math::isinf(key) ||
        value != value || // check for NaN
        boost::math::isinf(value)) {
        return kOfxStatFailed;
    }
    
    Natron::StatusEnum stat;
    if (_effect->getPluginID() == PLUGINID_OFX_COLORCORRECT) {
        stat = _knob.lock()->addHorizontalControlPoint(curveIndex, key, value);
    } else {
        stat = _knob.lock()->addControlPoint(curveIndex, key, value);
    }

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteControlPoint(int curveIndex,
                                          int nthCtl)
{
    Natron::StatusEnum stat = _knob.lock()->deleteControlPoint(curveIndex, nthCtl);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteAllControlPoints(int curveIndex)
{
    Natron::StatusEnum stat = _knob.lock()->deleteAllControlPoints(curveIndex);

    if (stat == Natron::eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

void
OfxParametricInstance::onCustomBackgroundDrawingRequested()
{
    if (_overlayInteract) {
        RenderScale s;
        _overlayInteract->getPixelScale(s.x, s.y);
        _overlayInteract->drawAction(_effect->getApp()->getTimeLine()->currentFrame(), s);
    }
}

OfxStatus
OfxParametricInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                                OfxTime offset,
                                const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

