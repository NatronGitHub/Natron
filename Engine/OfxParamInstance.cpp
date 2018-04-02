/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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
#include <cassert>
#include <cfloat>
#include <stdexcept>

#include <boost/scoped_array.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/math/special_functions/fpclassify.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>
#include <ofxParametricParam.h>
#include <ofxNatron.h>

#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"
#include "Engine/AppManager.h"
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
#include "Engine/ReadNode.h"
#include "Engine/Node.h"
#include "Engine/TLSHolder.h"
#include "Engine/ViewIdx.h"
#include "Engine/WriteNode.h"

NATRON_NAMESPACE_ENTER


struct OfxKeyFrames_compare
{
    bool operator() (double lhs,
                     double rhs) const
    {
        if (std::abs(rhs - lhs) < NATRON_CURVE_X_SPACING_EPSILON) {
            return false;
        }

        return lhs < rhs;
    }
};

typedef std::set<double, OfxKeyFrames_compare> OfxKeyFramesSet;

static void
getOfxKeyFrames(KnobI* knob,
                OfxKeyFramesSet &keyframes,
                int startDim = -1,
                int endDim = -1)
{
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getDimension();
    }
    assert(startDim < endDim && startDim >= 0);
    if ( knob->canAnimate() ) {
        for (int i = startDim; i < endDim; ++i) {


            // Some plug-ins (any of our GeneratorPlugin derivatives) use getNumKeys to figure out
            // if the knob is animated.
            // If the knob has an expression, it may be using e.g a random based function and does not necessarily
            // have a fixed number of keyframes. In that case we return 2 fake keyframes so the plug-in thinks
            // there's an animation.
            // If we don't do this, a simple constant node with a random() as expression of the color knob will
            // make the node return true in the isIdentity action of the GeneratorPlugin (in openfx-supportext)
            // hence the color will not vary over time.
            std::string expr = knob->getExpression(0);
            if (!expr.empty()) {
                keyframes.insert(0);
                keyframes.insert(1);
            } else {
                CurvePtr curve = knob->getCurve(ViewIdx(0), i);
                if (curve) {
                    KeyFrameSet dimKeys = curve->getKeyFrames_mt_safe();
                    for (KeyFrameSet::iterator it = dimKeys.begin(); it != dimKeys.end(); ++it) {
                        keyframes.insert( it->getTime() );
                    }
                }
            }
        }
    }
}

///anonymous namespace to handle keyframes communication support for Ofx plugins
/// in a generalized manner
namespace OfxKeyFrame {
static
OfxStatus
getNumKeys(KnobI* knob,
           unsigned int &nKeys,
           int startDim = -1,
           int endDim = -1)
{
    OfxKeyFramesSet keyframes;

    getOfxKeyFrames(knob, keyframes, startDim, endDim);
    nKeys = (unsigned int)keyframes.size();

    return kOfxStatOK;
}

static
OfxStatus
getKeyTime(const KnobIPtr& knob,
           int nth,
           OfxTime & time,
           int startDim = -1,
           int endDim = -1)
{
    if (nth < 0) {
        return kOfxStatErrBadIndex;
    }
    OfxKeyFramesSet keyframes;
    getOfxKeyFrames(knob.get(), keyframes, startDim, endDim);
    if ( nth >= (int)keyframes.size() ) {
        return kOfxStatErrBadIndex;
    }
    OfxKeyFramesSet::iterator it = keyframes.begin();
    std::advance(it, nth);
    time = *it;

    return kOfxStatOK;
}

static
OfxStatus
getKeyIndex(const KnobIPtr& knob,
            OfxTime time,
            int direction,
            int & index,
            int startDim = -1,
            int endDim = -1)
{
    OfxKeyFramesSet keyframes;

    getOfxKeyFrames(knob.get(), keyframes, startDim, endDim);
    int i = 0;
    if (direction == 0) {
        //search for key at indicated time
        for (OfxKeyFramesSet::iterator it = keyframes.begin(); it != keyframes.end(); ++it, ++i) {
            if (std::abs(*it - time) < NATRON_CURVE_X_SPACING_EPSILON) {
                index = i;

                return kOfxStatOK;
            }
        }
    } else if (direction < 0) {
        //search for key before indicated time
        OfxKeyFramesSet::iterator next = keyframes.begin();
        if ( !keyframes.empty() ) {
            ++next;
        }
        for (OfxKeyFramesSet::iterator it = keyframes.begin(); it != keyframes.end(); ++it, ++i) {
            if (*it < time) {
                if ( next != keyframes.end() ) {
                    if (*next > time) {
                        index = i;

                        return kOfxStatOK;
                    }
                } else {
                    index = i;
                    assert(i == (int)keyframes.size() - 1);

                    return kOfxStatOK;
                }
            } else {
                return kOfxStatFailed;
            }
            if ( !keyframes.empty() ) {
                ++next;
            }
        }
    } else if (direction > 0) {
        ///Find the first keyframe that is considered to be after the given time
        for (OfxKeyFramesSet::iterator it = keyframes.begin(); it != keyframes.end(); ++it, ++i) {
            if (*it > time) {
                index = i;

                return kOfxStatOK;
            }
        }
    }

    return kOfxStatFailed;
} // getKeyIndex

static
OfxStatus
deleteKey(const KnobIPtr& knob,
          OfxTime time,
          int startDim = -1,
          int endDim = -1)
{
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getDimension();
    }
    assert(startDim < endDim && startDim >= 0);
    for (int i = startDim; i < endDim; ++i) {
        knob->deleteValueAtTime(eCurveChangeReasonInternal, time, ViewSpec::all(), i, false);
    }

    return kOfxStatOK;
}

static
OfxStatus
deleteAllKeys(const KnobIPtr& knob,
              int startDim = -1,
              int endDim = -1)
{
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getDimension();
    }
    assert(startDim < endDim && startDim >= 0);
    for (int i = startDim; i < endDim; ++i) {
        knob->removeAnimation(ViewSpec::all(), i);
    }

    return kOfxStatOK;
}

// copy one parameter to another, with a range (NULL means to copy all animation)
static
OfxStatus
copyFrom(const KnobIPtr & from,
         const KnobIPtr &to,
         OfxTime offset,
         const OfxRangeD* range,
         int startDim = -1,
         int endDim = -1)
{
    ///copy only if type is the same
    if ( from->typeName() == to->typeName() ) {
        to->beginChanges();
        for (int i = startDim; i < endDim; ++i) {
            to->clone(from, offset, range, i);
        }
        int dims = std::min( endDim, to->getDimension() );
        double curTime = from->getCurrentTime();
        for (int i = startDim; i < dims; ++i) {
            to->evaluateValueChange(i, curTime, ViewIdx(0),  eValueChangedReasonPluginEdited);
        }
        to->endChanges();
    }

    return kOfxStatOK;
}
} // namespace OfxKeyFrame

PropertyModified_RAII::PropertyModified_RAII(OfxParamToKnob* h)
    : _h(h)
{
    h->setDynamicPropertyModified(true);
}

PropertyModified_RAII::~PropertyModified_RAII()
{
    _h->setDynamicPropertyModified(false);
}

EffectInstancePtr
OfxParamToKnob::getKnobHolder() const
{
    EffectInstancePtr effect = _effect.lock();

    if (!effect) {
        return EffectInstancePtr();
    }

#ifdef NATRON_ENABLE_IO_META_NODES
    NodePtr node = effect->getNode();
    assert(node);
    std::string pluginID = node->getPluginID();
    /*
       For readers and writers
     */
    bool wasProjectCreatedWithLowerCaseIDs = node->getApp()->wasProjectCreatedWithLowerCaseIDs();
    if ( ReadNode::isBundledReader(pluginID, wasProjectCreatedWithLowerCaseIDs) ||
         WriteNode::isBundledWriter(pluginID, wasProjectCreatedWithLowerCaseIDs) ) {
        NodePtr iocontainer = node->getIOContainer();
        assert(iocontainer);

        return iocontainer->getEffectInstance();
    }
#endif

    return effect;
}

void
OfxParamToKnob::connectDynamicProperties()
{
    KnobIPtr knob = getKnob();

    if (!knob) {
        return;
    }
    KnobSignalSlotHandler* handler = knob->getSignalSlotHandler().get();
    if (!handler) {
        return;
    }
    QObject::connect( handler, SIGNAL(labelChanged()), this, SLOT(onLabelChanged()) );
    QObject::connect( handler, SIGNAL(evaluateOnChangeChanged(bool)), this, SLOT(onEvaluateOnChangeChanged(bool)) );
    QObject::connect( handler, SIGNAL(secretChanged()), this, SLOT(onSecretChanged()) );
    QObject::connect( handler, SIGNAL(enabledChanged()), this, SLOT(onEnabledChanged()) );
    QObject::connect( handler, SIGNAL(displayMinMaxChanged(double,double,int)), this, SLOT(onDisplayMinMaxChanged(double,double,int)) );
    QObject::connect( handler, SIGNAL(minMaxChanged(double,double,int)), this, SLOT(onMinMaxChanged(double,double,int)) );
    QObject::connect( handler, SIGNAL(helpChanged()), this, SLOT(onHintTooltipChanged()) );
    QObject::connect( handler, SIGNAL(inViewerContextLabelChanged()), this, SLOT(onInViewportLabelChanged()) );
    QObject::connect( handler, SIGNAL(viewerContextSecretChanged()), this, SLOT(onInViewportSecretChanged()) );
}

void
OfxParamToKnob::onKnobAnimationLevelChanged(ViewSpec /*view*/,
                                            int /*dimension*/)
{
    OFX::Host::Param::Instance* param = getOfxParam();

    assert(param);

    AnimationLevelEnum l = getKnob()->getAnimationLevel(0);

    ///This assert might crash Natron when reading a project made with a version
    ///of Natron prior to 0.96 when file params still had keyframes.
    //assert(l == eAnimationLevelNone || getCanAnimate());
    param->getProperties().setIntProperty(kOfxParamPropIsAnimating, l != eAnimationLevelNone);
    param->getProperties().setIntProperty(kOfxParamPropIsAutoKeying, l == eAnimationLevelInterpolatedValue);
}

void
OfxParamToKnob::onChoiceMenuReset()
{
    onChoiceMenuPopulated();
}

static void
setStringPropertyN(const std::vector<std::string>& entries,
                   OFX::Host::Param::Instance* param,
                   const std::string& property)
{
    try {
        OFX::Host::Property::String *prop = param->getProperties().fetchStringProperty(property);
        if (prop) {
            prop->reset();
            for (std::size_t i = 0; i < entries.size(); ++i) {
                prop->setValue(entries[i], i);
            }
        }
    } catch (...) {
    }
}

void
OfxParamToKnob::onChoiceMenuPopulated()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
    if (!isChoice) {
        return;
    }
    std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();

    std::vector<std::string> ids(entries.size());
    std::vector<std::string> tooltips(entries.size());
    std::vector<std::string> labels(entries.size());

    for (std::size_t i = 0; i < entries.size(); ++i) {
        ids[i] = entries[i].id;
        tooltips[i] = entries[i].tooltip;
        labels[i] = entries[i].label;
    }
    setStringPropertyN(ids, param, kOfxParamPropChoiceOption);
    setStringPropertyN(tooltips, param, kOfxParamPropChoiceLabelOption);
}

void
OfxParamToKnob::onChoiceMenuEntryAppended()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    int nProps = param->getProperties().getDimension(kOfxParamPropChoiceOption);
    int nLabelProps = param->getProperties().getDimension(kOfxParamPropChoiceLabelOption);
    assert(nProps == nLabelProps);

    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }

    KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knob.get());
    assert(isChoice);
    std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();

    // Add the difference since last append
    for (std::size_t i = (std::size_t)nProps; i < entries.size(); ++i) {
        param->getProperties().setStringProperty(kOfxParamPropChoiceOption, entries[i].id, i);
        param->getProperties().setStringProperty(kOfxParamPropChoiceLabelOption, entries[i].tooltip, i);
    }
}

void
OfxParamToKnob::onEvaluateOnChangeChanged(bool evaluate)
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    param->getProperties().setIntProperty(kOfxParamPropEvaluateOnChange, (int)evaluate);
}

void
OfxParamToKnob::onSecretChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setIntProperty( kOfxParamPropSecret, (int)knob->getIsSecret() );
}

void
OfxParamToKnob::onHintTooltipChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setStringProperty( kOfxParamPropHint, knob->getHintToolTip() );
}

void
OfxParamToKnob::onEnabledChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setIntProperty( kOfxParamPropEnabled, (int)knob->isEnabled(0) );
}

void
OfxParamToKnob::onLabelChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setStringProperty( kOfxPropLabel, knob->getLabel() );
}

void
OfxParamToKnob::onInViewportSecretChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setIntProperty( kNatronOfxParamPropInViewerContextSecret, (int)knob->getInViewerContextSecret() );
}

void
OfxParamToKnob::onInViewportLabelChanged()
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    param->getProperties().setStringProperty( kNatronOfxParamPropInViewerContextLabel, knob->getInViewerContextLabel() );
}

void
OfxParamToKnob::onDisplayMinMaxChanged(double min,
                                       double max,
                                       int index)
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    if ( hasDoubleMinMaxProps() ) {
        param->getProperties().setDoubleProperty(kOfxParamPropDisplayMin, min, index);
        param->getProperties().setDoubleProperty(kOfxParamPropDisplayMax, max, index);
    } else {
        param->getProperties().setIntProperty(kOfxParamPropDisplayMin, (int)min, index);
        param->getProperties().setIntProperty(kOfxParamPropDisplayMax, (int)max, index);
    }
}

void
OfxParamToKnob::onMinMaxChanged(double min,
                                double max,
                                int index)
{
    DYNAMIC_PROPERTY_CHECK();

    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    if ( hasDoubleMinMaxProps() ) {
        param->getProperties().setDoubleProperty(kOfxParamPropMin, min, index);
        param->getProperties().setDoubleProperty(kOfxParamPropMax, max, index);
    } else {
        param->getProperties().setIntProperty(kOfxParamPropMin, (int)min, index);
        param->getProperties().setIntProperty(kOfxParamPropMax, (int)max, index);
    }
}

////////////////////////// OfxPushButtonInstance /////////////////////////////////////////////////

OfxPushButtonInstance::OfxPushButtonInstance(const OfxEffectInstancePtr& node,
                                             OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::PushbuttonInstance( descriptor, node->effectInstance() )
{
    _knob = checkIfKnobExistsWithNameOrCreate<KnobButton>(descriptor.getName(), this, 1);
}

// callback which should set enabled state as appropriate
void
OfxPushButtonInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

void
OfxPushButtonInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}


// callback which should set secret state as appropriate
void
OfxPushButtonInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}

void
OfxPushButtonInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxPushButtonInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxPushButtonInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxPushButtonInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobButtonPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
OfxPushButtonInstance::getKnob() const
{
    return _knob.lock();
}

////////////////////////// OfxIntegerInstance /////////////////////////////////////////////////


OfxIntegerInstance::OfxIntegerInstance(const OfxEffectInstancePtr& node,
                                       OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::IntegerInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobIntPtr k = checkIfKnobExistsWithNameOrCreate<KnobInt>(descriptor.getName(), this, 1);

    _knob = k;
    setRange();
    setDisplayRange();

    int def = properties.getIntProperty(kOfxParamPropDefault);
    k->setIncrement(1); // kOfxParamPropIncrement only exists for Double
    k->blockValueChanges();
    k->setDefaultValue(def, 0);
    k->unblockValueChanges();
    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, 0);
    k->setDimensionName(0, dimensionName);
}

OfxStatus
OfxIntegerInstance::get(int & v)
{
    KnobIntPtr knob = _knob.lock();

    v = knob->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::get(OfxTime time,
                        int & v)
{
    KnobIntPtr knob = _knob.lock();

    v = knob->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(int v)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValueFromPlugin(v, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(OfxTime time,
                        int v)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValueAtTimeFromPlugin(time, v, ViewSpec::current(), 0);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxIntegerInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxIntegerInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxIntegerInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxIntegerInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxIntegerInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxIntegerInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0), 0);
}

void
OfxIntegerInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxIntegerInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
OfxIntegerInstance::getKnob() const
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
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
OfxIntegerInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    int displayMin = getProperties().getIntProperty(kOfxParamPropDisplayMin);
    int displayMax = getProperties().getIntProperty(kOfxParamPropDisplayMax);

    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimum(displayMin);
    knob->setDisplayMaximum(displayMax);
}

void
OfxIntegerInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    int mini = getProperties().getIntProperty(kOfxParamPropMin);
    int maxi = getProperties().getIntProperty(kOfxParamPropMax);

    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimum(mini);
    knob->setMaximum(maxi);
}

////////////////////////// OfxDoubleInstance /////////////////////////////////////////////////


OfxDoubleInstance::OfxDoubleInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::DoubleInstance( descriptor, node->effectInstance() )

{
    const OFX::Host::Property::Set &properties = getProperties();
    const std::string & coordSystem = getDefaultCoordinateSystem();
    KnobDoublePtr dblKnob = checkIfKnobExistsWithNameOrCreate<KnobDouble>(descriptor.getName(), this, 1);

    _knob = dblKnob;
    setRange();
    setDisplayRange();

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedX) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXAbsolute) ) {
        dblKnob->setValueIsNormalized(0, eValueIsNormalizedX);
    } else if ( (doubleType == kOfxParamDoubleTypeNormalisedY) ||
                ( doubleType == kOfxParamDoubleTypeNormalisedYAbsolute) ) {
        dblKnob->setValueIsNormalized(0, eValueIsNormalizedY);
    }

    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    double def = properties.getDoubleProperty(kOfxParamPropDefault);
    int decimals = properties.getIntProperty(kOfxParamPropDigits);

    if (incr > 0) {
        dblKnob->setIncrement(incr);
    }
    if (decimals > 0) {
        dblKnob->setDecimals(decimals);
    }

    // The defaults should be stored as is, not premultiplied by the project size.
    // The fact that the default value is normalized should be stored in Knob or KnobDouble.

    // see http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#kOfxParamPropDefaultCoordinateSystem
    // and http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#APIChanges_1_2_SpatialParameters
    if ( (doubleType == kOfxParamDoubleTypeNormalisedX) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXAbsolute) ) {
        dblKnob->setValueIsNormalized(0, eValueIsNormalizedX);
    } else if ( (doubleType == kOfxParamDoubleTypeNormalisedY) ||
                ( doubleType == kOfxParamDoubleTypeNormalisedYAbsolute) ) {
        dblKnob->setValueIsNormalized(0, eValueIsNormalizedY);
    }
    dblKnob->setDefaultValuesAreNormalized(coordSystem == kOfxParamCoordinatesNormalised ||
                                           doubleType == kOfxParamDoubleTypeNormalisedX ||
                                           doubleType == kOfxParamDoubleTypeNormalisedXAbsolute ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedY ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedYAbsolute ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedXY ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedXYAbsolute);
    dblKnob->blockValueChanges();
    dblKnob->setDefaultValue(def, 0);
    dblKnob->unblockValueChanges();
    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, 0);
    dblKnob->setDimensionName(0, dimensionName);
}

OfxStatus
OfxDoubleInstance::get(double & v)
{
    KnobDoublePtr knob = _knob.lock();

    v = knob->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::get(OfxTime time,
                       double & v)
{
    KnobDoublePtr knob = _knob.lock();

    v = knob->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(double v)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValueFromPlugin(v, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(OfxTime time,
                       double v)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValueAtTimeFromPlugin(time, v, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::derive(OfxTime time,
                          double & v)
{
    KnobDoublePtr knob = _knob.lock();

    v = knob->getDerivativeAtTime( time, ViewSpec::current() );

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::integrate(OfxTime time1,
                             OfxTime time2,
                             double & v)
{
    KnobDoublePtr knob = _knob.lock();

    v = knob->getIntegrateFromTimeToTime( time1, time2, ViewSpec::current() );

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxDoubleInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

void
OfxDoubleInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxDoubleInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(_properties.getDoubleProperty(kOfxParamPropDefault, 0), 0);
}

// callback which should set secret state as appropriate
void
OfxDoubleInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxDoubleInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxDoubleInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxDoubleInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxDoubleInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxDoubleInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    double displayMin = getProperties().getDoubleProperty(kOfxParamPropDisplayMin);
    double displayMax = getProperties().getDoubleProperty(kOfxParamPropDisplayMax);

    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimum(displayMin);
    knob->setDisplayMaximum(displayMax);
}

void
OfxDoubleInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    double mini = getProperties().getDoubleProperty(kOfxParamPropMin);
    double maxi = getProperties().getDoubleProperty(kOfxParamPropMax);

    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimum(mini);
    knob->setMaximum(maxi);
}

KnobIPtr
OfxDoubleInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxDoubleInstance::isAnimated() const
{
    KnobDoublePtr knob = _knob.lock();

    return knob->isAnimated( 0, ViewSpec::current() );
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxDoubleInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxBooleanInstance /////////////////////////////////////////////////

OfxBooleanInstance::OfxBooleanInstance(const OfxEffectInstancePtr& node,
                                       OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::BooleanInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobBoolPtr b = checkIfKnobExistsWithNameOrCreate<KnobBool>(descriptor.getName(), this, 1);

    _knob = b;
    int def = properties.getIntProperty(kOfxParamPropDefault);
    b->blockValueChanges();
    b->setDefaultValue( (bool)def, 0 );
    b->unblockValueChanges();
}

OfxStatus
OfxBooleanInstance::get(bool & b)
{
    KnobBoolPtr knob = _knob.lock();

    b = knob->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::get(OfxTime time,
                        bool & b)
{
    assert( KnobBool::canAnimateStatic() );
    KnobBoolPtr knob = _knob.lock();
    b = knob->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(bool b)
{
    KnobBoolPtr knob = _knob.lock();

    knob->setValueFromPlugin(b, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(OfxTime time,
                        bool b)
{
    assert( KnobBool::canAnimateStatic() );
    KnobBoolPtr knob = _knob.lock();
    knob->setValueAtTimeFromPlugin(time, b, ViewSpec::current(), 0);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxBooleanInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxBooleanInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxBooleanInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxBooleanInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxBooleanInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxBooleanInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0), 0);
}

void
OfxBooleanInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxBooleanInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobBoolPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
OfxBooleanInstance::getKnob() const
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxBooleanInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                             OfxTime offset,
                             const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxChoiceInstance /////////////////////////////////////////////////

OfxChoiceInstance::OfxChoiceInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::ChoiceInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobChoicePtr choice = checkIfKnobExistsWithNameOrCreate<KnobChoice>(descriptor.getName(), this, 1);

    _knob = choice;


    int dim = getProperties().getDimension(kOfxParamPropChoiceOption);
    int labelOptionDim = getProperties().getDimension(kOfxParamPropChoiceLabelOption);
    int enumOptionDim = getProperties().getDimension(kOfxParamPropChoiceEnum);
    std::vector<ChoiceOption> entries(dim);
    for (int i = 0; i < dim; ++i) {

        ChoiceOption& option = entries[i];
        option.label = getProperties().getStringProperty(kOfxParamPropChoiceOption, i);
        if (i < enumOptionDim) {
            option.id = getProperties().getStringProperty(kOfxParamPropChoiceEnum, i);
        } else {
            option.id = option.label;
        }
        if (i < labelOptionDim) {
            option.tooltip = getProperties().getStringProperty(kOfxParamPropChoiceLabelOption, i);
        }
    }

    choice->populateChoices(entries);

    int def = properties.getIntProperty(kOfxParamPropDefault);
    choice->blockValueChanges();
    choice->setDefaultValue(def, 0);
    choice->unblockValueChanges();
    bool cascading = properties.getIntProperty(kNatronOfxParamPropChoiceCascading) != 0;
    choice->setCascading(cascading);

    bool canAddOptions = (int)properties.getIntProperty(kNatronOfxParamPropChoiceHostCanAddOptions);
    if (canAddOptions) {
        choice->setHostCanAddOptions(true);
    }
    QObject::connect( choice.get(), SIGNAL(populated()), this, SLOT(onChoiceMenuPopulated()) );
    QObject::connect( choice.get(), SIGNAL(entryAppended()), this, SLOT(onChoiceMenuEntryAppended()) );
    QObject::connect( choice.get(), SIGNAL(entriesReset()), this, SLOT(onChoiceMenuReset()) );
}

OfxStatus
OfxChoiceInstance::get(int & v)
{
    KnobChoicePtr knob = _knob.lock();

    v = knob->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::get(OfxTime time,
                       int & v)
{
    assert( KnobChoice::canAnimateStatic() );
    KnobChoicePtr knob = _knob.lock();
    v = knob->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::set(int v)
{
    KnobChoicePtr knob = _knob.lock();

    if (!knob) {
        return kOfxStatFailed;
    }
    int nEntries = knob->getNumEntries();
    if ( (0 <= v) && ( v < nEntries ) ) {
        knob->setValueFromPlugin(v, ViewSpec::current(), 0);

        return kOfxStatOK;
    } else {
        return kOfxStatErrBadIndex;
    }
}

OfxStatus
OfxChoiceInstance::set(OfxTime time,
                       int v)
{
    KnobChoicePtr knob = _knob.lock();

    if (!knob) {
        return kOfxStatFailed;
    }
     int nEntries = knob->getNumEntries();
    if ( (0 <= v) && ( v < nEntries ) ) {
        knob->setValueAtTimeFromPlugin(time, v, ViewSpec::current(), 0);

        return kOfxStatOK;
    } else {
        return kOfxStatErrBadIndex;
    }
}

void
OfxChoiceInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}


void
OfxChoiceInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0), 0);
}

// callback which should set enabled state as appropriate
void
OfxChoiceInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxChoiceInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxChoiceInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxChoiceInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxChoiceInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxChoiceInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobChoicePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxChoiceInstance::setOption(int num)
{
    DYNAMIC_PROPERTY_CHECK();

    /*
     This function can serve 3 type of behaviours depending on num:
     - 0: meaning resetOptions
     - num == nDim - 1: meaning appendOption
     - num == nDim: meaning setOptions
     */
    int dim = getProperties().getDimension(kOfxParamPropChoiceOption);
    KnobChoicePtr knob = _knob.lock();
    if (dim == 0) {
        knob->resetChoices();
        return;
    }

    assert(num == dim - 1 || num == dim);

    if ( num == (dim - 1) ) {
        ChoiceOption option;
        option.id = getProperties().getStringProperty(kOfxParamPropChoiceOption, num);
        option.label = option.id;
        int labelOptionDim = getProperties().getDimension(kOfxParamPropChoiceLabelOption);
        int optionIDDim = getProperties().getDimension(kOfxParamPropChoiceEnum);
        if (num < optionIDDim) {
            option.id = getProperties().getStringProperty(kOfxParamPropChoiceEnum, num);
        }
        if (num < labelOptionDim) {
            option.tooltip = getProperties().getStringProperty(kOfxParamPropChoiceLabelOption, num);
        }
        knob->appendChoice(option);
    } else if (num == dim) {

        int labelOptionDim = getProperties().getDimension(kOfxParamPropChoiceLabelOption);
        assert(labelOptionDim == 0 || labelOptionDim == dim);
        int optionIDDim = getProperties().getDimension(kOfxParamPropChoiceEnum);
        assert(optionIDDim == 0 || optionIDDim == dim);
        std::vector<ChoiceOption> entries(dim);
        for (std::size_t i = 0; i < entries.size(); ++i) {
            entries[i].id = getProperties().getStringProperty(kOfxParamPropChoiceOption, i);
            entries[i].label = entries[i].id;
            if ( (int)i < optionIDDim ) {
                entries[i].id = getProperties().getStringProperty(kOfxParamPropChoiceEnum, i);
            }
            if ( (int)i < labelOptionDim ) {
                entries[i].tooltip = getProperties().getStringProperty(kOfxParamPropChoiceLabelOption, i);
            }
        }
        knob->populateChoices(entries);
    }
}

KnobIPtr
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxChoiceInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxRGBAInstance /////////////////////////////////////////////////

OfxRGBAInstance::OfxRGBAInstance(const OfxEffectInstancePtr& node,
                                 OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::RGBAInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobColorPtr knob = checkIfKnobExistsWithNameOrCreate<KnobColor>(descriptor.getName(), this, 4);

    _knob = knob;
    setRange();
    setDisplayRange();

    const int dims = 4;
    std::vector<double> def(dims);

    properties.getDoublePropertyN(kOfxParamPropDefault, &def[0], dims);
    knob->blockValueChanges();
    for (int i = 0; i < dims; ++i) {
        knob->setDefaultValue(def[i], i);
    }
    knob->unblockValueChanges();

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(i, dimensionName);
    }
}

OfxStatus
OfxRGBAInstance::get(double & r,
                     double & g,
                     double & b,
                     double & a)
{
    KnobColorPtr color = _knob.lock();

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
    KnobColorPtr color = _knob.lock();

    r = color->getValueAtTime( time, 0, ViewSpec::current() );
    g = color->getValueAtTime( time, 1, ViewSpec::current() );
    b = color->getValueAtTime( time, 2, ViewSpec::current() );
    a = color->getValueAtTime( time, 3, ViewSpec::current() );

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(double r,
                     double g,
                     double b,
                     double a)
{
    KnobColorPtr color = _knob.lock();

    color->setValues(r, g, b, a, ViewSpec::current(), eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(OfxTime time,
                     double r,
                     double g,
                     double b,
                     double a)
{
    KnobColorPtr color = _knob.lock();

    color->setValuesAtTime(time, r, g, b, a, ViewSpec::current(), eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::derive(OfxTime time,
                        double &r,
                        double & g,
                        double & b,
                        double & a)
{
    KnobColorPtr color = _knob.lock();

    r = color->getDerivativeAtTime(time, ViewSpec::current(), 0);
    g = color->getDerivativeAtTime(time, ViewSpec::current(), 1);
    b = color->getDerivativeAtTime(time, ViewSpec::current(), 2);
    a = color->getDerivativeAtTime(time, ViewSpec::current(), 3);

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
    KnobColorPtr color = _knob.lock();

    r = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 0);
    g = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 1);
    b = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 2);
    a = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 3);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxRGBAInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxRGBAInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxRGBAInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxRGBAInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxRGBAInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxRGBAInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxRGBAInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    double def[4];
    _properties.getDoublePropertyN(kOfxParamPropDefault, def, 4);
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(def[0], def[1], def[2], def[3]);
}

void
OfxRGBAInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxRGBAInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> displayMins(4);
    std::vector<double> displayMaxs(4);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxRGBAInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> mins(4);
    std::vector<double> maxs(4);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

KnobIPtr
OfxRGBAInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxRGBAInstance::isAnimated(int dimension) const
{
    KnobColorPtr color = _knob.lock();

    return color->isAnimated( dimension, ViewSpec::current() );
}

bool
OfxRGBAInstance::isAnimated() const
{
    KnobColorPtr color = _knob.lock();

    return color->isAnimated( 0, ViewSpec::current() ) || color->isAnimated( 1, ViewSpec::current() ) || color->isAnimated( 2, ViewSpec::current() ) || color->isAnimated( 3, ViewSpec::current() );
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxRGBAInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                          OfxTime offset,
                          const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxRGBInstance /////////////////////////////////////////////////

OfxRGBInstance::OfxRGBInstance(const OfxEffectInstancePtr& node,
                               OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::RGBInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobColorPtr knob  = checkIfKnobExistsWithNameOrCreate<KnobColor>(descriptor.getName(), this, 3);

    _knob = knob;
    setRange();
    setDisplayRange();

    const int dims = 3;
    std::vector<double> def(dims);
    properties.getDoublePropertyN(kOfxParamPropDefault, &def[0], dims);
    knob->blockValueChanges();
    for (int i = 0; i < dims; ++i) {
        knob->setDefaultValue(def[i], i);
    }
    knob->unblockValueChanges();

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(i, dimensionName);
    }
}

OfxStatus
OfxRGBInstance::get(double & r,
                    double & g,
                    double & b)
{
    KnobColorPtr color = _knob.lock();

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
    KnobColorPtr color = _knob.lock();

    r = color->getValueAtTime(time, 0);
    g = color->getValueAtTime(time, 1);
    b = color->getValueAtTime(time, 2);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(double r,
                    double g,
                    double b)
{
    KnobColorPtr color = _knob.lock();

    color->setValues(r, g, b, ViewSpec::current(),  eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(OfxTime time,
                    double r,
                    double g,
                    double b)
{
    KnobColorPtr color = _knob.lock();

    color->setValuesAtTime(time, r, g, b, ViewSpec::current(), eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::derive(OfxTime time,
                       double & r,
                       double & g,
                       double & b)
{
    KnobColorPtr color = _knob.lock();

    r = color->getDerivativeAtTime(time, ViewSpec::current(), 0);
    g = color->getDerivativeAtTime(time, ViewSpec::current(), 1);
    b = color->getDerivativeAtTime(time, ViewSpec::current(), 2);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::integrate(OfxTime time1,
                          OfxTime time2,
                          double &r,
                          double & g,
                          double & b)
{
    KnobColorPtr color = _knob.lock();

    r = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 0);
    g = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 1);
    b = color->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 2);

    return kOfxStatOK;
}

void
OfxRGBInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxRGBInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    double def[3];
    _properties.getDoublePropertyN(kOfxParamPropDefault, def, 3);
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(def[0], def[1], def[2]);
}

// callback which should set enabled state as appropriate
void
OfxRGBInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxRGBInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxRGBInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxRGBInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxRGBInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxRGBInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxRGBInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> displayMins(3);
    std::vector<double> displayMaxs(3);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxRGBInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> mins(3);
    std::vector<double> maxs(3);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

KnobIPtr
OfxRGBInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxRGBInstance::isAnimated(int dimension) const
{
    KnobColorPtr color = _knob.lock();

    return color->isAnimated( dimension, ViewSpec::current() );
}

bool
OfxRGBInstance::isAnimated() const
{
    KnobColorPtr color = _knob.lock();

    return color->isAnimated( 0, ViewSpec::current() ) || color->isAnimated( 1, ViewSpec::current() ) || color->isAnimated( 2, ViewSpec::current() );
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxRGBInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                         OfxTime offset,
                         const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxDouble2DInstance /////////////////////////////////////////////////

OfxDouble2DInstance::OfxDouble2DInstance(const OfxEffectInstancePtr& node,
                                         OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::Double2DInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    const std::string & coordSystem = getDefaultCoordinateSystem();
    const int ofxDims = 2;
    int knobDims = 2;

    _startIndex = 0;

    int isRectangleType = properties.getIntProperty(kNatronOfxParamPropTypeRectangle);
    std::string paramName = descriptor.getName();
    if (isRectangleType == 1) {
        std::size_t found = paramName.find("_position");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 4;
    } else if (isRectangleType == 2) {
        std::size_t found = paramName.find("_size");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 4;
        _startIndex = 2;
    }

    KnobDoublePtr dblKnob = node->getKnobByNameAndType<KnobDouble>(paramName);
    if (!dblKnob) {
        dblKnob = checkIfKnobExistsWithNameOrCreate<KnobDouble>(paramName, this, knobDims);
    }
    _knob = dblKnob;
    setRange();
    setDisplayRange();

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ) {
        dblKnob->setValueIsNormalized(0, eValueIsNormalizedX);
        dblKnob->setValueIsNormalized(1, eValueIsNormalizedY);
    }
    // disable slider if the type is an absolute position
    if ( (doubleType == kOfxParamDoubleTypeXYAbsolute) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ||
         ( isRectangleType > 0) ) {
        dblKnob->disableSlider();
    }
    // only auto-merge dimensions and show a single slider if this is a scale or a size
    dblKnob->setCanAutoFoldDimensions( (doubleType == kOfxParamDoubleTypeScale) ||
                           (doubleType == kOfxParamDoubleTypeXY) ||
                           (doubleType == kOfxParamDoubleTypeNormalisedXY) );

    if (isRectangleType > 0) {
        dblKnob->setAsRectangle();
    }
    std::vector<double> increment(ofxDims);
    std::vector<int> decimals(ofxDims);
    std::vector<double> def(ofxDims);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i = 0; i < ofxDims; ++i) {
        increment[i] = incr;
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault, i);

        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        dblKnob->setDimensionName(i + _startIndex, dimensionName);

        dblKnob->setIncrement(incr, _startIndex + i);
        dblKnob->setDecimals(decimals[i], _startIndex + i);
    }

    // Only create native overlays if there is no interact or kOfxParamPropUseHostOverlayHandle is set
    // see https://github.com/MrKepzie/Natron/issues/932
    // only create automatic overlay for kOfxParamDoubleTypeXYAbsolute and kOfxParamDoubleTypeNormalisedXYAbsolute
    if ( ( !node->effectInstance()->getOverlayInteractMainEntry() && !isRectangleType &&
           ( ( getDoubleType() == kOfxParamDoubleTypeXYAbsolute) ||
             ( getDoubleType() == kOfxParamDoubleTypeNormalisedXYAbsolute) ) ) ||
         ( properties.getIntProperty(kOfxParamPropUseHostOverlayHandle) == 1) ) {
        dblKnob->setHasHostOverlayHandle(true);
    }

    dblKnob->setSpatial(doubleType == kOfxParamDoubleTypeNormalisedXY ||
                        doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute ||
                        doubleType == kOfxParamDoubleTypeXY ||
                        doubleType == kOfxParamDoubleTypeXYAbsolute);
    if ( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ) {
        dblKnob->setValueIsNormalized(0 + _startIndex, eValueIsNormalizedX);
        dblKnob->setValueIsNormalized(1 + _startIndex, eValueIsNormalizedY);
    }
    dblKnob->setDefaultValuesAreNormalized(coordSystem == kOfxParamCoordinatesNormalised ||
                                           doubleType == kOfxParamDoubleTypeNormalisedXY ||
                                           doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute);
    dblKnob->blockValueChanges();
    for (int i = 0; i < ofxDims; ++i) {
        dblKnob->setDefaultValue(def[i], i + _startIndex);
    }
    dblKnob->unblockValueChanges();
}

OfxStatus
OfxDouble2DInstance::get(double & x1,
                         double & x2)
{
    KnobDoublePtr dblKnob = _knob.lock();

    x1 = dblKnob->getValue(_startIndex);
    x2 = dblKnob->getValue(_startIndex + 1);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::get(OfxTime time,
                         double & x1,
                         double & x2)
{
    KnobDoublePtr dblKnob = _knob.lock();

    x1 = dblKnob->getValueAtTime(time, _startIndex);
    x2 = dblKnob->getValueAtTime(time, _startIndex + 1);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(double x1,
                         double x2)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValues(x1, x2, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(OfxTime time,
                         double x1,
                         double x2)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValuesAtTime(time, x1, x2, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::derive(OfxTime time,
                            double &x1,
                            double & x2)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getDerivativeAtTime(time, ViewSpec::current(), _startIndex);
    x2 = knob->getDerivativeAtTime(time, ViewSpec::current(), 1 + _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), _startIndex);
    x2 = knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 1 + _startIndex);

    return kOfxStatOK;
}

void
OfxDouble2DInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxDouble2DInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    double def[2];
    _properties.getDoublePropertyN(kOfxParamPropDefault, def, 2);
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(def[0], def[1]);
}

// callback which should set enabled state as appropriate
void
OfxDouble2DInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxDouble2DInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxDouble2DInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxDouble2DInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxDouble2DInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxDouble2DInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

void
OfxDouble2DInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> displayMins(2);
    std::vector<double> displayMaxs(2);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxDouble2DInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> mins(2);
    std::vector<double> maxs(2);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

KnobIPtr
OfxDouble2DInstance::getKnob() const
{
    return _knob.lock();
}

#if 0
bool
OfxDouble2DInstance::isAnimated(int dimension) const
{
    KnobDoublePtr knob = _knob.lock();

    return knob->isAnimated( dimension + _startIndex, ViewSpec::current() );
}

#endif
bool
OfxDouble2DInstance::isAnimated() const
{
    KnobDoublePtr dblKnob = _knob.lock();

    return dblKnob->isAnimated( 0 + _startIndex, ViewSpec::current() ) || dblKnob->isAnimated( 1 + _startIndex, ViewSpec::current() );
}

OfxStatus
OfxDouble2DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock(), _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                              OfxTime offset,
                              const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range, _startIndex, _startIndex + 2);
}

////////////////////////// OfxInteger2DInstance /////////////////////////////////////////////////

OfxInteger2DInstance::OfxInteger2DInstance(const OfxEffectInstancePtr& node,
                                           OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::Integer2DInstance( descriptor, node->effectInstance() )

{
    const OFX::Host::Property::Set &properties = getProperties();
    const int ofxDims = 2;
    int knobDims = 2;

    _startIndex = 0;
    int isRectangleType = properties.getIntProperty(kNatronOfxParamPropTypeRectangle);
    std::string paramName = descriptor.getName();
    if (isRectangleType == 1) {
        std::size_t found = paramName.find("_position");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 4;
    } else if (isRectangleType == 2) {
        std::size_t found = paramName.find("_size");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 4;
        _startIndex = 2;
    }


    KnobIntPtr iKnob = node->getKnobByNameAndType<KnobInt>(paramName);
    if (!iKnob) {
        iKnob = checkIfKnobExistsWithNameOrCreate<KnobInt>(paramName, this, knobDims);
    }
    _knob = iKnob;
    setRange();
    setDisplayRange();

    if (isRectangleType) {
        iKnob->setAsRectangle();
    }

    std::vector<int> increment(ofxDims);
    boost::scoped_array<int> def(new int[ofxDims]);

    properties.getIntPropertyN(kOfxParamPropDefault, &def[0], ofxDims);
    for (int i = 0; i < ofxDims; ++i) {
        increment[i] = 1; // kOfxParamPropIncrement only exists for Double
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        iKnob->setDimensionName(i + _startIndex, dimensionName);
        iKnob->setIncrement(increment[i], _startIndex + i);
    }

    iKnob->blockValueChanges();
    iKnob->setDefaultValue(def[0], 0 + _startIndex);
    iKnob->setDefaultValue(def[1], 1 + _startIndex);
    iKnob->unblockValueChanges();
}

OfxStatus
OfxInteger2DInstance::get(int & x1,
                          int & x2)
{
    KnobIntPtr iKnob = _knob.lock();

    x1 = iKnob->getValue(_startIndex);
    x2 = iKnob->getValue(_startIndex + 1);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::get(OfxTime time,
                          int & x1,
                          int & x2)
{
    KnobIntPtr iKnob = _knob.lock();

    x1 = iKnob->getValueAtTime(time, _startIndex);
    x2 = iKnob->getValueAtTime(time, _startIndex + 1);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(int x1,
                          int x2)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValues(x1, x2, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(OfxTime time,
                          int x1,
                          int x2)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValuesAtTime(time, x1, x2, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void
OfxInteger2DInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxInteger2DInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxInteger2DInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxInteger2DInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxInteger2DInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxInteger2DInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<int> displayMins(2);
    std::vector<int> displayMaxs(2);
    getProperties().getIntPropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    getProperties().getIntPropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxInteger2DInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();

    std::vector<int> mins(2);
    std::vector<int> maxs(2);
    _properties.getIntPropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getIntPropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

void
OfxInteger2DInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxInteger2DInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    int def[2];
    _properties.getIntPropertyN(kOfxParamPropDefault, def, 2);
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(def[0], def[1]);
}

void
OfxInteger2DInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
OfxInteger2DInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxInteger2DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::getKeyTime(int nth,
                                 OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::getKeyIndex(OfxTime time,
                                  int direction,
                                  int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock(), _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                               OfxTime offset,
                               const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range, _startIndex, 2 + _startIndex);
}

////////////////////////// OfxDouble3DInstance /////////////////////////////////////////////////

OfxDouble3DInstance::OfxDouble3DInstance(const OfxEffectInstancePtr& node,
                                         OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::Double3DInstance( descriptor, node->effectInstance() )
{
    const int ofxDims = 3;
    const OFX::Host::Property::Set &properties = getProperties();
    int knobDims = 3;
    int isMatrixType = properties.getIntProperty(kNatronOfxParamPropDoubleTypeMatrix3x3);
    std::string paramName = descriptor.getName();

    _startIndex = 0;
    if (isMatrixType == 1) {
        std::size_t found = paramName.find("_row1");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 9;
    } else if (isMatrixType == 2) {
        std::size_t found = paramName.find("_row2");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 9;
        _startIndex = 3;
    } else if (isMatrixType == 3) {
        std::size_t found = paramName.find("_row3");
        assert(found != std::string::npos);
        paramName = paramName.substr(0, found);
        knobDims = 9;
        _startIndex = 6;
    }


    KnobDoublePtr knob = node->getKnobByNameAndType<KnobDouble>(paramName);
    if (!knob) {
        knob = checkIfKnobExistsWithNameOrCreate<KnobDouble>(paramName, this, knobDims);
    }
    _knob = knob;
    setRange();
    setDisplayRange();

    const std::string & doubleType = getDoubleType();
    // only auto-merge dimensions and show a single slider if this is a scale
    knob->setCanAutoFoldDimensions(doubleType == kOfxParamDoubleTypeScale);

    std::vector<int> decimals(ofxDims);
    std::vector<double> def(ofxDims);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i = 0; i < ofxDims; ++i) {
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault, i);
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(_startIndex + i, dimensionName);
        knob->setIncrement(incr, _startIndex + i);
        knob->setDecimals(decimals[i], _startIndex + i);
    }

    knob->blockValueChanges();
    for (int i = 0; i <  ofxDims; ++i) {
        knob->setDefaultValue(def[i], _startIndex + i);
    }
    knob->unblockValueChanges();
}

OfxStatus
OfxDouble3DInstance::get(double & x1,
                         double & x2,
                         double & x3)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getValue(0 + _startIndex);
    x2 = knob->getValue(1 + _startIndex);
    x3 = knob->getValue(2 + _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::get(OfxTime time,
                         double & x1,
                         double & x2,
                         double & x3)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getValueAtTime(time, 0 + _startIndex);
    x2 = knob->getValueAtTime(time, 1 + _startIndex);
    x3 = knob->getValueAtTime(time, 2 + _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(double x1,
                         double x2,
                         double x3)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValues(x1, x2, x3, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(OfxTime time,
                         double x1,
                         double x2,
                         double x3)
{
    KnobDoublePtr knob = _knob.lock();

    knob->setValuesAtTime(time, x1, x2, x3, ViewSpec::current(), eValueChangedReasonPluginEdited, _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::derive(OfxTime time,
                            double & x1,
                            double & x2,
                            double & x3)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getDerivativeAtTime(time, ViewSpec::current(), 0 + _startIndex);
    x2 = knob->getDerivativeAtTime(time, ViewSpec::current(), 1 + _startIndex);
    x3 = knob->getDerivativeAtTime(time, ViewSpec::current(), 2 + _startIndex);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2,
                               double & x3)
{
    KnobDoublePtr knob = _knob.lock();

    x1 = knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 0 + _startIndex);
    x2 = knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 1 + _startIndex);
    x3 = knob->getIntegrateFromTimeToTime(time1, time2, ViewSpec::current(), 2 + _startIndex);

    return kOfxStatOK;
}

void
OfxDouble3DInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxDouble3DInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(_properties.getDoubleProperty(kOfxParamPropDefault, 0), _properties.getDoubleProperty(kOfxParamPropDefault, 1), _properties.getDoubleProperty(kOfxParamPropDefault, 2));
}

// callback which should set enabled state as appropriate
void
OfxDouble3DInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxDouble3DInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxDouble3DInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxDouble3DInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxDouble3DInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxDouble3DInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> displayMins(3);
    std::vector<double> displayMaxs(3);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxDouble3DInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<double> mins(3);
    std::vector<double> maxs(3);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

void
OfxDouble3DInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
OfxDouble3DInstance::getKnob() const
{
    return _knob.lock();
}

bool
OfxDouble3DInstance::isAnimated(int dimension) const
{
    KnobDoublePtr knob = _knob.lock();

    return knob->isAnimated( dimension, ViewSpec::current() );
}

bool
OfxDouble3DInstance::isAnimated() const
{
    KnobDoublePtr knob = _knob.lock();

    return knob->isAnimated( 0 + _startIndex, ViewSpec::current() ) || knob->isAnimated( 1 + _startIndex, ViewSpec::current() ) || knob->isAnimated( 2 + _startIndex, ViewSpec::current() );
}

OfxStatus
OfxDouble3DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock().get(), nKeys, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_knob.lock(), nth, time, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(_knob.lock(), time, direction, index, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_knob.lock(), time, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(_knob.lock(), _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                              OfxTime offset,
                              const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range, _startIndex, 3 + _startIndex);
}

////////////////////////// OfxInteger3DInstance /////////////////////////////////////////////////

OfxInteger3DInstance::OfxInteger3DInstance(const OfxEffectInstancePtr&node,
                                           OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::Integer3DInstance( descriptor, node->effectInstance() )
{
    const int dims = 3;
    const OFX::Host::Property::Set &properties = getProperties();
    KnobIntPtr knob = checkIfKnobExistsWithNameOrCreate<KnobInt>(descriptor.getName(), this, dims);

    _knob = knob;
    setRange();
    setDisplayRange();

    std::vector<int> increment(dims);
    std::vector<int> def(dims);

    for (int i = 0; i < dims; ++i) {
        int incr = properties.getIntProperty(kOfxParamPropIncrement, i);
        increment[i] = incr != 0 ?  incr : 1;
        def[i] = properties.getIntProperty(kOfxParamPropDefault, i);

        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(i, dimensionName);
    }

    knob->setIncrement(increment);
    knob->blockValueChanges();
    knob->setDefaultValue(def[0], 0);
    knob->setDefaultValue(def[1], 1);
    knob->setDefaultValue(def[2], 2);
    knob->unblockValueChanges();
}

OfxStatus
OfxInteger3DInstance::get(int & x1,
                          int & x2,
                          int & x3)
{
    KnobIntPtr knob = _knob.lock();

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
    KnobIntPtr knob = _knob.lock();

    x1 = knob->getValueAtTime(time, 0);
    x2 = knob->getValueAtTime(time, 1);
    x3 = knob->getValueAtTime(time, 2);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(int x1,
                          int x2,
                          int x3)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValues(x1, x2, x3, ViewSpec::current(), eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(OfxTime time,
                          int x1,
                          int x2,
                          int x3)
{
    KnobIntPtr knob = _knob.lock();

    knob->setValuesAtTime(time, x1, x2, x3, ViewSpec::current(), eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

void
OfxInteger3DInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}

void
OfxInteger3DInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0), _properties.getIntProperty(kOfxParamPropDefault, 1), _properties.getIntProperty(kOfxParamPropDefault, 2));
}

// callback which should set enabled state as appropriate
void
OfxInteger3DInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxInteger3DInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxInteger3DInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxInteger3DInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxInteger3DInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxInteger3DInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<int> displayMins(3);
    std::vector<int> displayMaxs(3);
    getProperties().getIntPropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    getProperties().getIntPropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxInteger3DInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    std::vector<int> mins(3);
    std::vector<int> maxs(3);
    _properties.getIntPropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getIntPropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setMinimumsAndMaximums(mins, maxs);
}

void
OfxInteger3DInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

KnobIPtr
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
    return OfxKeyFrame::deleteAllKeys( _knob.lock() );
}

OfxStatus
OfxInteger3DInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                               OfxTime offset,
                               const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxGroupInstance /////////////////////////////////////////////////

OfxGroupInstance::OfxGroupInstance(const OfxEffectInstancePtr& node,
                                   OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::GroupInstance( descriptor, node->effectInstance() )
    , _groupKnob()
{
    const OFX::Host::Property::Set &properties = getProperties();
    int isTab = properties.getIntProperty(kFnOfxParamPropGroupIsTab);
    KnobGroupPtr group = checkIfKnobExistsWithNameOrCreate<KnobGroup>(descriptor.getName(), this, 1);

    _groupKnob = group;
    int opened = properties.getIntProperty(kOfxParamPropGroupOpen);
    if (isTab) {
        group->setAsTab();
    }

    group->blockValueChanges();
    group->setDefaultValue(opened, 0);
    group->unblockValueChanges();
}

void
OfxGroupInstance::addKnob(KnobIPtr k)
{
    _groupKnob.lock()->addKnob(k);
}

KnobIPtr
OfxGroupInstance::getKnob() const
{
    return _groupKnob.lock();
}

void
OfxGroupInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setHintToolTip(getHint());
}

void
OfxGroupInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxGroupInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setSecret( getSecret() );
}


void
OfxGroupInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxGroupInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxGroupInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _groupKnob.lock()->setLabel( getParamLabel(this) );
}

void
OfxGroupInstance::setOpen()
{
    DYNAMIC_PROPERTY_CHECK();
    int opened = getProperties().getIntProperty(kOfxParamPropGroupOpen);
    _groupKnob.lock()->setValue((bool)opened);
}

////////////////////////// OfxPageInstance /////////////////////////////////////////////////


OfxPageInstance::OfxPageInstance(const OfxEffectInstancePtr& node,
                                 OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::PageInstance( descriptor, node->effectInstance() )
    , _pageKnob()
{
    EffectInstancePtr holder = getKnobHolder();

    assert(holder);

    _pageKnob = checkIfKnobExistsWithNameOrCreate<KnobPage>(descriptor.getName(), this, 1);
}

// callback which should set enabled state as appropriate
void
OfxPageInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxPageInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setAllDimensionsEnabled( getSecret() );
}


void
OfxPageInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxPageInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxPageInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setHintToolTip(getHint());
}

void
OfxPageInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setLabel( getParamLabel(this) );
}

KnobIPtr
OfxPageInstance::getKnob() const
{
    return _pageKnob.lock();
}

////////////////////////// OfxStringInstance /////////////////////////////////////////////////

struct OfxStringInstancePrivate
{
    KnobFileWPtr fileKnob;
    KnobOutputFileWPtr outputFileKnob;
    KnobStringWPtr stringKnob;
    KnobPathWPtr pathKnob;
    boost::shared_ptr<TLSHolder<OfxParamToKnob::OfxParamTLSData> > tlsData;

    OfxStringInstancePrivate()
        : fileKnob()
        , outputFileKnob()
        , stringKnob()
        , pathKnob()
        , tlsData( new TLSHolder<OfxParamToKnob::OfxParamTLSData>() )
    {
    }
};

OfxStringInstance::OfxStringInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::StringInstance( descriptor, node->effectInstance() )
    , _imp( new OfxStringInstancePrivate() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    std::string mode = properties.getStringProperty(kOfxParamPropStringMode);
    bool richText = mode == kOfxParamStringIsRichTextFormat;

    if (mode == kOfxParamStringIsFilePath) {
        int fileIsImage = ( ( node->isReader() ||
                              node->isWriter() ) &&
                            (getScriptName() == kOfxImageEffectFileParamName ||
                             getScriptName() == kOfxImageEffectProxyParamName) );
        int fileIsOutput = !properties.getIntProperty(kOfxParamPropStringFilePathExists);
        int filePathSupportsImageSequences = getCanAnimate();


        if (!fileIsOutput) {
            _imp->fileKnob = checkIfKnobExistsWithNameOrCreate<KnobFile>(descriptor.getName(), this, 1);
            if (fileIsImage) {
                _imp->fileKnob.lock()->setAsInputImage();
            }
            if (!filePathSupportsImageSequences) {
                _imp->fileKnob.lock()->setAnimationEnabled(false);
            }
        } else {
            _imp->outputFileKnob = checkIfKnobExistsWithNameOrCreate<KnobOutputFile>(descriptor.getName(), this, 1);
            if (fileIsImage) {
                _imp->outputFileKnob.lock()->setAsOutputImageFile();
            } else {
                _imp->outputFileKnob.lock()->turnOffSequences();
            }
            if (!filePathSupportsImageSequences) {
                _imp->outputFileKnob.lock()->setAnimationEnabled(false);
            }
        }
    } else if (mode == kOfxParamStringIsDirectoryPath) {
        _imp->pathKnob = checkIfKnobExistsWithNameOrCreate<KnobPath>(descriptor.getName(), this, 1);
        _imp->pathKnob.lock()->setMultiPath(false);
    } else if ( (mode == kOfxParamStringIsSingleLine) || (mode == kOfxParamStringIsLabel) || (mode == kOfxParamStringIsMultiLine) || richText ) {
        _imp->stringKnob = checkIfKnobExistsWithNameOrCreate<KnobString>(descriptor.getName(), this, 1);
        if (mode == kOfxParamStringIsLabel) {
            _imp->stringKnob.lock()->setAllDimensionsEnabled(false);
            _imp->stringKnob.lock()->setAsLabel();
        }
        if ( (mode == kOfxParamStringIsMultiLine) || richText ) {
            ///only QTextArea support rich text anyway
            _imp->stringKnob.lock()->setUsesRichText(richText);
            _imp->stringKnob.lock()->setAsMultiLine();
        }
    }
    std::string defaultVal = properties.getStringProperty(kOfxParamPropDefault);
    if ( !defaultVal.empty() ) {
        if ( _imp->fileKnob.lock() ) {
            projectEnvVar_setProxy(defaultVal);
            KnobFilePtr k = _imp->fileKnob.lock();
            k->blockValueChanges();
            k->setDefaultValue(defaultVal, 0);
            k->unblockValueChanges();
        } else if ( _imp->outputFileKnob.lock() ) {
            projectEnvVar_setProxy(defaultVal);
            KnobOutputFilePtr k = _imp->outputFileKnob.lock();
            k->blockValueChanges();
            k->setDefaultValue(defaultVal, 0);
            k->unblockValueChanges();
        } else if ( _imp->stringKnob.lock() ) {
            KnobStringPtr k = _imp->stringKnob.lock();
            k->blockValueChanges();
            k->setDefaultValue(defaultVal, 0);
            k->unblockValueChanges();
        } else if ( _imp->pathKnob.lock() ) {
            projectEnvVar_setProxy(defaultVal);
            KnobPathPtr k = _imp->pathKnob.lock();
            k->blockValueChanges();
            k->setDefaultValue(defaultVal, 0);
            k->unblockValueChanges();
        }
    }
}

OfxStringInstance::~OfxStringInstance()
{
}

void
OfxStringInstance::projectEnvVar_getProxy(std::string& str) const
{
    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    KnobHolder* holder = knob->getHolder();
    if (!holder) {
        return;
    }
    AppInstancePtr app = holder->getApp();
    if (!app) {
        return;
    }
    ProjectPtr proj = app->getProject();
    if (!proj) {
        return;
    }
    proj->canonicalizePath(str);
}

void
OfxStringInstance::projectEnvVar_setProxy(std::string& str) const
{
    getKnob()->getHolder()->getApp()->getProject()->simplifyPath(str);
}

OfxStatus
OfxStringInstance::get(std::string &str)
{
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    KnobOutputFilePtr outputFileKnob = _imp->outputFileKnob.lock();
    KnobStringPtr strknob = _imp->stringKnob.lock();
    KnobPathPtr pathKnob = _imp->pathKnob.lock();

    if (fileKnob) {
        str = fileKnob->getFileName( fileKnob->getCurrentTime(), fileKnob->getCurrentView() );
        projectEnvVar_getProxy(str);
    } else if (outputFileKnob) {
        str = outputFileKnob->generateFileNameAtTime( outputFileKnob->getCurrentTime(), outputFileKnob->getCurrentView() ).toStdString();
        projectEnvVar_getProxy(str);
    } else if (strknob) {
        str = strknob->getValue();
    } else if (pathKnob) {
        str = pathKnob->getValue();
        projectEnvVar_getProxy(str);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::get(OfxTime time,
                       std::string & str)
{
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    KnobOutputFilePtr outputFileKnob = _imp->outputFileKnob.lock();
    KnobStringPtr strknob = _imp->stringKnob.lock();
    KnobPathPtr pathKnob = _imp->pathKnob.lock();

    if (fileKnob) {
        str = fileKnob->getFileName( std::floor(time + 0.5), fileKnob->getCurrentView() );
        projectEnvVar_getProxy(str);
    } else if (outputFileKnob) {
        str = outputFileKnob->generateFileNameAtTime( std::floor(time + 0.5), outputFileKnob->getCurrentView() ).toStdString();
        projectEnvVar_getProxy(str);
    } else if (strknob) {
        str = strknob->getValueAtTime( std::floor(time + 0.5) );
    } else if (pathKnob) {
        str = pathKnob->getValueAtTime( std::floor(time + 0.5) );
        projectEnvVar_getProxy(str);
    }


    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(const char* str)
{
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    KnobOutputFilePtr outputFileKnob = _imp->outputFileKnob.lock();
    KnobStringPtr strknob = _imp->stringKnob.lock();
    KnobPathPtr pathKnob = _imp->pathKnob.lock();

    if (fileKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        fileKnob->setValueFromPlugin(s, ViewSpec::current(), 0);
    }
    if (outputFileKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        outputFileKnob->setValueFromPlugin(s, ViewSpec::current(), 0);
    }
    if (strknob) {
        strknob->setValueFromPlugin(str, ViewSpec::current(), 0);
    }
    if (pathKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        pathKnob->setValueFromPlugin(s, ViewSpec::current(), 0);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(OfxTime time,
                       const char* str)
{
    assert( KnobString::canAnimateStatic() );

    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    KnobOutputFilePtr outputFileKnob = _imp->outputFileKnob.lock();
    KnobStringPtr strknob = _imp->stringKnob.lock();
    KnobPathPtr pathKnob = _imp->pathKnob.lock();


    if (fileKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        fileKnob->setValueAtTimeFromPlugin(time, s, ViewSpec::current(), 0);
    }
    if (outputFileKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        outputFileKnob->setValueAtTimeFromPlugin(time, s, ViewSpec::current(), 0);
    }
    if (strknob) {
        strknob->setValueAtTimeFromPlugin(time, str, ViewSpec::current(), 0);
    }
    if (pathKnob) {
        std::string s(str);
        projectEnvVar_setProxy(s);
        pathKnob->setValueAtTimeFromPlugin(time, s, ViewSpec::current(), 0);
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::getV(va_list arg)
{
    const char **value = va_arg(arg, const char **);
    OfxStatus stat;
    std::string& tls = _imp->tlsData->getOrCreateTLSData()->str;

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
    std::string& tls = _imp->tlsData->getOrCreateTLSData()->str;

    stat = get( time, tls);
    *value = tls.c_str();

    return stat;
}

KnobIPtr
OfxStringInstance::getKnob() const
{
    if ( _imp->fileKnob.lock() ) {
        return _imp->fileKnob.lock();
    }
    if ( _imp->outputFileKnob.lock() ) {
        return _imp->outputFileKnob.lock();
    }
    if ( _imp->stringKnob.lock() ) {
        return _imp->stringKnob.lock();
    }
    if ( _imp->pathKnob.lock() ) {
        return _imp->pathKnob.lock();
    }

    return KnobIPtr();
}

// callback which should set enabled state as appropriate
void
OfxStringInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setAllDimensionsEnabled( getEnabled() );
    }
}

void
OfxStringInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setHintToolTip(getHint());
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setHintToolTip(getHint());
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setHintToolTip(getHint());
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setHintToolTip(getHint());
    }
}

void
OfxStringInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    const std::string& v = _properties.getStringProperty(kOfxParamPropDefault, 0);
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setDefaultValueWithoutApplying(v, 0);
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setDefaultValueWithoutApplying(v, 0);
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setDefaultValueWithoutApplying(v, 0);
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setDefaultValueWithoutApplying(v, 0);
    }
}

void
OfxStringInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setLabel( getParamLabel(this) );
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setLabel( getParamLabel(this) );
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setLabel( getParamLabel(this) );
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setLabel( getParamLabel(this) );
    }
}

// callback which should set secret state as appropriate
void
OfxStringInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setSecret( getSecret() );
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setSecret( getSecret() );
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setSecret( getSecret() );
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setSecret( getSecret() );
    }
}


void
OfxStringInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
}

void
OfxStringInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }

}

void
OfxStringInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    if ( _imp->fileKnob.lock() ) {
        _imp->fileKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if ( _imp->outputFileKnob.lock() ) {
        _imp->outputFileKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if ( _imp->stringKnob.lock() ) {
        _imp->stringKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
    if ( _imp->pathKnob.lock() ) {
        _imp->pathKnob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
    }
}

OfxStatus
OfxStringInstance::getNumKeys(unsigned int &nKeys) const
{
    KnobIPtr knob;

    if ( _imp->stringKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->stringKnob.lock() );
    } else if ( _imp->fileKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->fileKnob.lock() );
    } else {
        return nKeys = 0;
    }

    return OfxKeyFrame::getNumKeys(knob.get(), nKeys);
}

OfxStatus
OfxStringInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    KnobIPtr knob;

    if ( _imp->stringKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->stringKnob.lock() );
    } else if ( _imp->fileKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->fileKnob.lock() );
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
    KnobIPtr knob;

    if ( _imp->stringKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->stringKnob.lock() );
    } else if ( _imp->fileKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->fileKnob.lock() );
    } else {
        return kOfxStatFailed;
    }

    return OfxKeyFrame::getKeyIndex(knob, time, direction, index);
}

OfxStatus
OfxStringInstance::deleteKey(OfxTime time)
{
    KnobIPtr knob;

    if ( _imp->stringKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->stringKnob.lock() );
    } else if ( _imp->fileKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->fileKnob.lock() );
    } else {
        return kOfxStatErrBadIndex;
    }

    return OfxKeyFrame::deleteKey(knob, time);
}

OfxStatus
OfxStringInstance::deleteAllKeys()
{
    KnobIPtr knob;

    if ( _imp->stringKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->stringKnob.lock() );
    } else if ( _imp->fileKnob.lock() ) {
        knob = boost::dynamic_pointer_cast<KnobI>( _imp->fileKnob.lock() );
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

struct OfxCustomInstancePrivate
{
    KnobStringWPtr knob;
    OfxCustomInstance::customParamInterpolationV1Entry_t customParamInterpolationV1Entry;
    boost::shared_ptr<TLSHolder<OfxParamToKnob::OfxParamTLSData> > tlsData;

    OfxCustomInstancePrivate()
        : knob()
        , customParamInterpolationV1Entry(0)
        , tlsData( new TLSHolder<OfxParamToKnob::OfxParamTLSData>() )
    {
    }
};

OfxCustomInstance::OfxCustomInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::CustomInstance( descriptor, node->effectInstance() )
    , _imp( new OfxCustomInstancePrivate() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobStringPtr knob = checkIfKnobExistsWithNameOrCreate<KnobString>(descriptor.getName(), this, 1);

    _imp->knob = knob;

    knob->setAsCustom();
    knob->blockValueChanges();
    knob->setDefaultValue(properties.getStringProperty(kOfxParamPropDefault), 0);
    knob->unblockValueChanges();
    GCC_DIAG_PEDANTIC_OFF
    _imp->customParamInterpolationV1Entry = (customParamInterpolationV1Entry_t)properties.getPointerProperty(kOfxParamPropCustomInterpCallbackV1);
    GCC_DIAG_PEDANTIC_ON

    if (_imp->customParamInterpolationV1Entry) {
        knob->setCustomInterpolation( _imp->customParamInterpolationV1Entry, (void*)getHandle() );
    }
}

OfxCustomInstance::~OfxCustomInstance()
{
}

OfxStatus
OfxCustomInstance::get(std::string &str)
{
    KnobStringPtr knob = _imp->knob.lock();

    str = knob->getValue();

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::get(OfxTime time,
                       std::string & str)
{
    assert( KnobString::canAnimateStatic() );
    // it should call _customParamInterpolationV1Entry
    KnobStringPtr knob = _imp->knob.lock();
    str = knob->getValueAtTime(time);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(const char* str)
{
    KnobStringPtr knob = _imp->knob.lock();

    knob->setValueFromPlugin(str, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(OfxTime time,
                       const char* str)
{
    assert( KnobString::canAnimateStatic() );
    KnobStringPtr knob = _imp->knob.lock();
    knob->setValueAtTimeFromPlugin(time, str, ViewSpec::current(), 0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::getV(va_list arg)
{
    const char **value = va_arg(arg, const char **);
    std::string& s = _imp->tlsData->getOrCreateTLSData()->str;
    OfxStatus stat = get(s);

    *value = s.c_str();

    return stat;
}

OfxStatus
OfxCustomInstance::getV(OfxTime time,
                        va_list arg)
{
    const char **value = va_arg(arg, const char **);
    std::string& s = _imp->tlsData->getOrCreateTLSData()->str;
    OfxStatus stat = get( time, s);

    *value = s.c_str();

    return stat;
}

KnobIPtr
OfxCustomInstance::getKnob() const
{
    return _imp->knob.lock();
}

// callback which should set enabled state as appropriate
void
OfxCustomInstance::setEnabled()
{
    //DYNAMIC_PROPERTY_CHECK();
    //_imp->knob.lock()->setAllDimensionsEnabled( getEnabled() );
}

void
OfxCustomInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setHintToolTip(getHint());
}


void
OfxCustomInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setDefaultValueWithoutApplying(_properties.getStringProperty(kOfxParamPropDefault, 0), 0);
}

// callback which should set secret state as appropriate
void
OfxCustomInstance::setSecret()
{
    //Always make custom parameters secret as it is done on others OFX hosts. They can contain very long
    //string which may take time to the UI to draw.
    //DYNAMIC_PROPERTY_CHECK();
    //_imp->knob.lock()->setSecret( getSecret() );
}


void
OfxCustomInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxCustomInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxCustomInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setLabel( getParamLabel(this) );
}

void
OfxCustomInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    _imp->knob.lock()->setEvaluateOnChange( getEvaluateOnChange() );
}

OfxStatus
OfxCustomInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_imp->knob.lock().get(), nKeys);
}

OfxStatus
OfxCustomInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(_imp->knob.lock(), nth, time);
}

OfxStatus
OfxCustomInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(_imp->knob.lock(), time, direction, index);
}

OfxStatus
OfxCustomInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(_imp->knob.lock(), time);
}

OfxStatus
OfxCustomInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( _imp->knob.lock() );
}

OfxStatus
OfxCustomInstance::copyFrom(const OFX::Host::Param::Instance &instance,
                            OfxTime offset,
                            const OfxRangeD* range)
{
    const OfxParamToKnob & other = dynamic_cast<const OfxParamToKnob &>(instance);

    return OfxKeyFrame::copyFrom(other.getKnob(), getKnob(), offset, range);
}

////////////////////////// OfxParametricInstance /////////////////////////////////////////////////

OfxParametricInstance::OfxParametricInstance(const OfxEffectInstancePtr& node,
                                             OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::ParametricParam::ParametricInstance( descriptor, node->effectInstance() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    int parametricDimension = properties.getIntProperty(kOfxParamPropParametricDimension);
    KnobParametricPtr knob = checkIfKnobExistsWithNameOrCreate<KnobParametric>(descriptor.getName(), this, parametricDimension);
    _knob = knob;
    setRange();
    setDisplayRange();


    bool isPeriodic = (bool)properties.getIntProperty(kNatronOfxParamPropParametricIsPeriodic);
    knob->setPeriodic(isPeriodic);

    setLabel(); //set label on all curves

    std::vector<double> color(3 * parametricDimension);
    properties.getDoublePropertyN(kOfxParamPropParametricUIColour, &color[0], 3 * parametricDimension);

    
    for (int i = 0; i < parametricDimension; ++i) {
        knob->setCurveColor(i, color[i * 3], color[i * 3 + 1], color[i * 3 + 2]);
    }

    double range[2] = {0., 1.};
    getProperties().getDoublePropertyN(kOfxParamPropParametricRange, range, 2);
    if(range[1] > range[0]) {
        knob->setParametricRange(range[0], range[1]);
    }
    for (int i = 0; i < parametricDimension; ++i) {
        const std::string & curveName = getProperties().getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(i, curveName);
    }
}

OfxPluginEntryPoint*
OfxParametricInstance::getCustomOverlayInteractEntryPoint(const OFX::Host::Param::Instance* param) const
{
    Q_UNUSED(param);
    GCC_DIAG_PEDANTIC_OFF
    return (OfxPluginEntryPoint*)getProperties().getPointerProperty(kOfxParamPropParametricInteractBackground);
    GCC_DIAG_PEDANTIC_ON
}

void
OfxParametricInstance::onCurvesDefaultInitialized()
{
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultCurvesFromCurves();
}

OfxParametricInstance::~OfxParametricInstance()
{
}

KnobIPtr
OfxParametricInstance::getKnob() const
{
    return _knob.lock();
}

// callback which should set enabled state as appropriate
void
OfxParametricInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setAllDimensionsEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxParametricInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setSecret( getSecret() );
}


void
OfxParametricInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
}

void
OfxParametricInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));

}

void
OfxParametricInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setHintToolTip(getHint());
}


void
OfxParametricInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setEvaluateOnChange( getEvaluateOnChange() );
}

/// callback which should update label
void
OfxParametricInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setLabel( getParamLabel(this) );
}

void
OfxParametricInstance::setDisplayRange()
{
    DYNAMIC_PROPERTY_CHECK();
    // the display range is the Y display range in the parametric param
    // (as opposed to kOfxParamPropParametricRange, which is read-only on the instance and has no hook)

    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int dims = knob->getDimension();
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
}

void
OfxParametricInstance::setRange()
{
    DYNAMIC_PROPERTY_CHECK();
    // the range is the Y range in the parametric param
    // (as opposed to kOfxParamPropParametricRange, which is read-only on the instance and has no hook)

    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int dims = knob->getDimension();
    std::vector<double> mins(dims);
    std::vector<double> maxs(dims);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    knob->setMinimumsAndMaximums(mins, maxs);
}


OfxStatus
OfxParametricInstance::getValue(int curveIndex,
                                OfxTime /*time*/,
                                double parametricPosition,
                                double *returnValue)
{
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->getValue(curveIndex, parametricPosition, returnValue);

    if (stat == eStatusOK) {
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
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->getNControlPoints(curveIndex, returnValue);

    if (stat == eStatusOK) {
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
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->getNthControlPoint(curveIndex, nthCtl, key, value);

    if (stat == eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

// overridden from OFX::Host::ParametricParam::ParametricInstance::setNthControlPoint
OfxStatus
OfxParametricInstance::setNthControlPoint(int curveIndex,
                                          double /*time*/,
                                          int nthCtl,
                                          double key,
                                          double value,
                                          bool /*addAnimationKey*/)
{
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->setNthControlPoint(eValueChangedReasonPluginEdited, curveIndex, nthCtl, key, value);

    if (stat == eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

// overridden from OFX::Host::ParametricParam::ParametricInstance::addControlPoint
OfxStatus
OfxParametricInstance::addControlPoint(int curveIndex,
                                       double time,
                                       double key,
                                       double value,
                                       bool /* addAnimationKey*/)
{
    if ( (boost::math::isnan)(time) || // check for NaN
         (boost::math::isinf)(time) ||
         (boost::math::isnan)(key) || // check for NaN
         (boost::math::isinf)(key) ||
         (boost::math::isnan)(value) || // check for NaN
         (boost::math::isinf)(value) ) {
        return kOfxStatFailed;
    }

    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat;
    EffectInstance* effect = dynamic_cast<EffectInstance*>( knob->getHolder() );
    KeyframeTypeEnum interpolation = eKeyframeTypeSmooth; // a reasonable default
    // The initial curve for some plugins may be better with a specific interpolation. Unfortunately, the kOfxParametricSuiteV1 doesn't offer different interpolation methods
#ifdef DEBUG
#pragma message WARN("This is a hack, we should extend the parametric suite to add derivatives infos")
#endif
    if (effect) {
        if ( (effect->getPluginID() == PLUGINID_OFX_COLORCORRECT) || (effect->getPluginID() == PLUGINID_OFX_TIMEDISSOLVE) ) {
            interpolation = eKeyframeTypeHorizontal;
        } else if ( (effect->getPluginID() == PLUGINID_OFX_COLORLOOKUP) || (effect->getPluginID() == PLUGINID_OFX_RETIME) ) {
            interpolation = eKeyframeTypeCubic;
        }
    }
    stat = knob->addControlPoint(eValueChangedReasonPluginEdited, curveIndex, key, value, interpolation);

    if (stat == eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteControlPoint(int curveIndex,
                                          int nthCtl)
{
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->deleteControlPoint(eValueChangedReasonPluginEdited, curveIndex, nthCtl);

    if (stat == eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteAllControlPoints(int curveIndex)
{
    KnobParametricPtr knob = _knob.lock();
    if (!knob) {
        return kOfxStatFailed;
    }
    StatusEnum stat = knob->deleteAllControlPoints(eValueChangedReasonPluginEdited, curveIndex);

    if (stat == eStatusOK) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
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

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_OfxParamInstance.cpp"
