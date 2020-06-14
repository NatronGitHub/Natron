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
#include "Engine/KeyFrameInterpolator.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/PointOverlayInteract.h"
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
getOfxKeyFrames(const KnobIPtr& knob,
                OfxKeyFramesSet &keyframes,
                int startDim = -1,
                int endDim = -1)
{
    if ( !knob || !knob->canAnimate() ) {
        return;
    }
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getNDimensions();
    }
    assert(startDim < endDim && startDim >= 0);
    if ( knob->canAnimate() ) {
        std::list<ViewIdx> views = knob->getViewsList();
        for (int i = startDim; i < endDim; ++i) {
            for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {

                // Some plug-ins (any of our GeneratorPlugin derivatives) use getNumKeys to figure out
                // if the knob is animated.
                // If the knob has an expression, it may be using e.g a random based function and does not necessarily
                // have a fixed number of keyframes. In that case we return 2 fake keyframes so the plug-in thinks
                // there's an animation.
                // If we don't do this, a simple constant node with a random() as expression of the color knob will
                // make the node return true in the isIdentity action of the GeneratorPlugin (in openfx-supportext)
                // hence the color will not vary over time.
                std::string expr = knob->getExpression(DimIdx(i), *it);
                if (!expr.empty()) {
                    keyframes.insert(0);
                    keyframes.insert(1);
                } else {
                    CurvePtr curve = knob->getAnimationCurve(*it, DimIdx(i));
                    if (curve) {
                        KeyFrameSet dimKeys = curve->getKeyFrames_mt_safe();
                        for (KeyFrameSet::iterator it2 = dimKeys.begin(); it2 != dimKeys.end(); ++it2) {
                            keyframes.insert( it2->getTime() );
                        }
                    }
                }
            }
        }
    }

} // getOfxKeyFrames

///anonymous namespace to handle keyframes communication support for Ofx plugins
/// in a generalized manner
namespace OfxKeyFrame {
static
OfxStatus
getNumKeys(const KnobIPtr& knob,
           unsigned int &nKeys,
           int startDim = -1,
           int endDim = -1)
{
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
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
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    if (nth < 0) {
        return kOfxStatErrBadIndex;
    }
    OfxKeyFramesSet keyframes;
    getOfxKeyFrames(knob, keyframes, startDim, endDim);
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
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    OfxKeyFramesSet keyframes;

    getOfxKeyFrames(knob, keyframes, startDim, endDim);
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
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getNDimensions();
    }
    assert(startDim < endDim && startDim >= 0);
    ScopedChanges_RAII changes(knob.get());
    for (int i = startDim; i < endDim; ++i) {
        knob->deleteValueAtTime(TimeValue(time), ViewSetSpec::all(), DimIdx(i), eValueChangedReasonPluginEdited);
    }

    return kOfxStatOK;
}

static
OfxStatus
deleteAllKeys(const KnobIPtr& knob,
              int startDim = -1,
              int endDim = -1)
{
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    if (startDim == -1) {
        startDim = 0;
    }
    if (endDim == -1) {
        endDim = knob->getNDimensions();
    }
    assert(startDim < endDim && startDim >= 0);

    knob->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonPluginEdited);

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
    if (!from) {
        return kOfxStatErrBadHandle;
    }
    if (!to) {
        return kOfxStatErrBadHandle;
    }
    ///copy only if type is the same
    if ( from->typeName() == to->typeName() ) {
        ScopedChanges_RAII changes(to.get());
        for (int i = startDim; i < endDim; ++i) {
            to->copyKnob(from, ViewSetSpec::all(), DimIdx(i), ViewSetSpec::all(), DimIdx(i), range, offset);
        }
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

    NodePtr node = effect->getNode();
    assert(node);
    std::string pluginID = node->getPluginID();
    /*
       For readers and writers
     */
    if ( ReadNode::isBundledReader(pluginID) || WriteNode::isBundledWriter(pluginID) ) {
        NodePtr iocontainer = node->getIOContainer();
        assert(iocontainer);

        return iocontainer->getEffectInstance();
    }

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
    QObject::connect( handler, SIGNAL(displayMinMaxChanged(DimSpec)), this, SLOT(onDisplayMinMaxChanged(DimSpec)) );
    QObject::connect( handler, SIGNAL(minMaxChanged(DimSpec)), this, SLOT(onMinMaxChanged(DimSpec)) );
    QObject::connect( handler, SIGNAL(helpChanged()), this, SLOT(onHintTooltipChanged()) );
    QObject::connect( handler, SIGNAL(inViewerContextLabelChanged()), this, SLOT(onInViewportLabelChanged()) );
    QObject::connect( handler, SIGNAL(viewerContextSecretChanged()), this, SLOT(onInViewportSecretChanged()) );

    QObject::connect( this, SIGNAL(mustRefreshAnimationLevelLater()), this, SLOT(onMustRefreshAutoKeyingPropsLaterReceived()), Qt::QueuedConnection );
}

void
OfxParamToKnob::refreshAutoKeyingPropsLater()
{
    ++_nRefreshAutoKeyingRequests;
    Q_EMIT mustRefreshAnimationLevelLater();

}

void
OfxParamToKnob::onMustRefreshAutoKeyingPropsLaterReceived()
{
    if (!_nRefreshAutoKeyingRequests) {
        return;
    }
    _nRefreshAutoKeyingRequests = 0;
    refreshAutoKeyingPropsNow();
}

void
OfxParamToKnob::refreshAutoKeyingPropsNow()
{
    
    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);
    KnobIPtr knob = getKnob();
    assert(knob);

    AnimationLevelEnum level =  eAnimationLevelNone;
    int nDims = knob->getNDimensions();
    std::list<ViewIdx> views = knob->getViewsList();
    TimeValue time = knob->getCurrentRenderTime();
    for (int i = 0; i < nDims; ++i) {
        for (std::list<ViewIdx>::const_iterator it = views.begin(); it != views.end(); ++it) {
            AnimationLevelEnum thisLevel = getKnob()->getAnimationLevel(DimIdx(i), time, *it);
            if (thisLevel == eAnimationLevelOnKeyframe) {
                level = eAnimationLevelOnKeyframe;
                break;
            } else if (thisLevel == eAnimationLevelInterpolatedValue && level == eAnimationLevelNone) {
                level = eAnimationLevelInterpolatedValue;
            }
        }
    }


    param->getProperties().setIntProperty(kOfxParamPropIsAnimating, level != eAnimationLevelNone);
    param->getProperties().setIntProperty(kOfxParamPropIsAutoKeying, level == eAnimationLevelInterpolatedValue);
}

void
OfxParamToKnob::onMustRefreshGuiTriggered(ViewSetSpec,DimSpec,ValueChangedReasonEnum)
{
    refreshAutoKeyingPropsLater();
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
    KnobChoicePtr isChoice = toKnobChoice(knob);
    if (!isChoice) {
        return;
    }
    std::vector<ChoiceOption> entries = isChoice->getEntries();


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
    assert(nProps == param->getProperties().getDimension(kOfxParamPropChoiceLabelOption));

    KnobChoicePtr knob = toKnobChoice(getKnob());
    assert(knob);
    std::vector<ChoiceOption> entries = knob->getEntries();

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
    param->getProperties().setIntProperty( kOfxParamPropEnabled, (int)knob->isEnabled() );
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
OfxParamToKnob::onDisplayMinMaxChanged(DimSpec dimension)
{
    DYNAMIC_PROPERTY_CHECK();

    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
    KnobIntBasePtr isInt = toKnobIntBase(knob);
    if (!isDouble && !isInt) {
        return;
    }
    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);

    int nDims = knob->getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        if (dimension.isAll() || i == dimension) {

            if ( hasDoubleMinMaxProps() && isDouble) {
                double min = isDouble->getMinimum(DimIdx(i));
                double max = isDouble->getMaximum(DimIdx(i));
                param->getProperties().setDoubleProperty(kOfxParamPropDisplayMin, min, i);
                param->getProperties().setDoubleProperty(kOfxParamPropDisplayMax, max, i);
            } else if (isInt) {
                int min = isInt->getMinimum(DimIdx(i));
                int max = isInt->getMaximum(DimIdx(i));
                param->getProperties().setIntProperty(kOfxParamPropDisplayMin, min, i);
                param->getProperties().setIntProperty(kOfxParamPropDisplayMax, max, i);
            }
        }
    }


}

void
OfxParamToKnob::onMinMaxChanged(DimSpec dimension)
{
    DYNAMIC_PROPERTY_CHECK();

    KnobIPtr knob = getKnob();
    if (!knob) {
        return;
    }
    KnobDoubleBasePtr isDouble = toKnobDoubleBase(knob);
    KnobIntBasePtr isInt = toKnobIntBase(knob);
    if (!isDouble && !isInt) {
        return;
    }
    OFX::Host::Param::Instance* param = getOfxParam();
    assert(param);

    int nDims = knob->getNDimensions();
    for (int i = 0; i < nDims; ++i) {
        if (dimension.isAll() || i == dimension) {

            if ( hasDoubleMinMaxProps() && isDouble) {
                double min = isDouble->getMinimum(DimIdx(i));
                double max = isDouble->getMaximum(DimIdx(i));
                param->getProperties().setDoubleProperty(kOfxParamPropMin, min, i);
                param->getProperties().setDoubleProperty(kOfxParamPropMax, max, i);
            } else if (isInt) {
                int min = isInt->getMinimum(DimIdx(i));
                int max = isInt->getMaximum(DimIdx(i));
                param->getProperties().setIntProperty(kOfxParamPropMin, min, i);
                param->getProperties().setIntProperty(kOfxParamPropMax, max, i);
            }
        }
    }
}

template <typename K>
static boost::shared_ptr<K> resolveRenderKnob(const KnobIPtr& originalKnob)
{
    return originalKnob->getCloneForHolder<K>(appPTR->getOFXCurrentEffect_TLS());
}

////////////////////////// OfxPushButtonInstance /////////////////////////////////////////////////

OfxPushButtonInstance::OfxPushButtonInstance(const OfxEffectInstancePtr& node,
                                             OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::PushbuttonInstance( descriptor, node->effectInstance() )
{
    _knob = getOrCreateKnob<KnobButton>(descriptor.getName(), this, 1);
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
    knob->setEnabled( getEnabled() );
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
    KnobIntPtr k = getOrCreateKnob<KnobInt>(descriptor.getName(), this, 1);

    _knob = k;
    setRange();
    setDisplayRange();

    int def = properties.getIntProperty(kOfxParamPropDefault);

    k->setIncrement(1); // kOfxParamPropIncrement only exists for Double
    k->setDefaultValue(def);
    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, 0);
    k->setDimensionName(DimIdx(0), dimensionName);
}


OfxStatus
OfxIntegerInstance::get(int & v)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValue(DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::get(OfxTime time,
                        int & v)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(int v)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    knob->setValue(v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

    return kOfxStatOK;
}

OfxStatus
OfxIntegerInstance::set(OfxTime time,
                        int v)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    knob->setValueAtTime(TimeValue(time), v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

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
    knob->setEnabled( getEnabled() );
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
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0));
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobInt>(_knob.lock()), nKeys);
}

OfxStatus
OfxIntegerInstance::getKeyTime(int nth,
                               OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobInt>(_knob.lock()), nth, time);
}

OfxStatus
OfxIntegerInstance::getKeyIndex(OfxTime time,
                                int direction,
                                int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobInt>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxIntegerInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobInt>(_knob.lock()), time);
}

OfxStatus
OfxIntegerInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobInt>(_knob.lock()) );
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
    knob->setDisplayRange(displayMin, displayMax);
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
    knob->setRange(mini, maxi);
}

////////////////////////// OfxDoubleInstance /////////////////////////////////////////////////


OfxDoubleInstance::OfxDoubleInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::DoubleInstance( descriptor, node->effectInstance() )

{
    const OFX::Host::Property::Set &properties = getProperties();
    const std::string & coordSystem = getDefaultCoordinateSystem();
    KnobDoublePtr dblKnob = getOrCreateKnob<KnobDouble>(descriptor.getName(), this, 1);

    _knob = dblKnob;
    setRange();
    setDisplayRange();

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedX) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXAbsolute) ) {
        dblKnob->setValueIsNormalized(DimIdx(0), eValueIsNormalizedX);
    } else if ( (doubleType == kOfxParamDoubleTypeNormalisedY) ||
                ( doubleType == kOfxParamDoubleTypeNormalisedYAbsolute) ) {
        dblKnob->setValueIsNormalized(DimIdx(0), eValueIsNormalizedY);
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
        dblKnob->setValueIsNormalized(DimIdx(0), eValueIsNormalizedX);
    } else if ( (doubleType == kOfxParamDoubleTypeNormalisedY) ||
                ( doubleType == kOfxParamDoubleTypeNormalisedYAbsolute) ) {
        dblKnob->setValueIsNormalized(DimIdx(0), eValueIsNormalizedY);
    }
    dblKnob->setDefaultValuesAreNormalized(coordSystem == kOfxParamCoordinatesNormalised ||
                                           doubleType == kOfxParamDoubleTypeNormalisedX ||
                                           doubleType == kOfxParamDoubleTypeNormalisedXAbsolute ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedY ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedYAbsolute ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedXY ||
                                           doubleType ==  kOfxParamDoubleTypeNormalisedXYAbsolute);
    dblKnob->setDefaultValue(def);
    std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, 0);
    dblKnob->setDimensionName(DimIdx(0), dimensionName);
}

OfxStatus
OfxDoubleInstance::get(double & v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValue(DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::get(OfxTime time,
                       double & v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(double v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    knob->setValue(v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::set(OfxTime time,
                       double v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    knob->setValueAtTime(TimeValue(time), v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::derive(OfxTime time,
                          double & v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    v = knob->getDerivativeAtTime( TimeValue(time), knob->getCurrentRenderView(), DimIdx(0) );

    return kOfxStatOK;
}

OfxStatus
OfxDoubleInstance::integrate(OfxTime time1,
                             OfxTime time2,
                             double & v)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    v = knob->getIntegrateFromTimeToTime( TimeValue(time1), TimeValue(time2), knob->getCurrentRenderView(), DimIdx(0) );

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
    knob->setEnabled( getEnabled() );
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
    knob->setDefaultValueWithoutApplying(_properties.getDoubleProperty(kOfxParamPropDefault, 0));
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
    knob->setDisplayRange(displayMin, displayMax);
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
    knob->setRange(mini, maxi);
}

KnobIPtr
OfxDoubleInstance::getKnob() const
{
    return _knob.lock();
}



OfxStatus
OfxDoubleInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(_knob.lock(), nKeys);
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
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobDouble>(_knob.lock()), time);
}

OfxStatus
OfxDoubleInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobDouble>(_knob.lock()) );
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
    KnobBoolPtr b = getOrCreateKnob<KnobBool>(descriptor.getName(), this, 1);

    _knob = b;
    int def = properties.getIntProperty(kOfxParamPropDefault);
    b->setDefaultValue( (bool)def);
}

OfxStatus
OfxBooleanInstance::get(bool & b)
{
    KnobBoolPtr knob = resolveRenderKnob<KnobBool>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    b = knob->getValue(DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::get(OfxTime time,
                        bool & b)
{
    assert( KnobBool::canAnimateStatic() );
    KnobBoolPtr knob = resolveRenderKnob<KnobBool>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    b = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(bool b)
{
    KnobBoolPtr knob = resolveRenderKnob<KnobBool>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    knob->setValue(b, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

    return kOfxStatOK;
}

OfxStatus
OfxBooleanInstance::set(OfxTime time,
                        bool b)
{
    assert( KnobBool::canAnimateStatic() );
    KnobBoolPtr knob = resolveRenderKnob<KnobBool>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    knob->setValueAtTime(TimeValue(time), b, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);

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
    knob->setEnabled( getEnabled() );
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
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0));
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobBool>(_knob.lock()), nKeys);
}

OfxStatus
OfxBooleanInstance::getKeyTime(int nth,
                               OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobBool>(_knob.lock()), nth, time);
}

OfxStatus
OfxBooleanInstance::getKeyIndex(OfxTime time,
                                int direction,
                                int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobBool>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxBooleanInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobBool>(_knob.lock()), time);
}

OfxStatus
OfxBooleanInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobBool>(_knob.lock()) );
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
    KnobChoicePtr choice = getOrCreateKnob<KnobChoice>(descriptor.getName(), this, 1);

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
    choice->setDefaultValue(def);
    bool cascading = properties.getIntProperty(kNatronOfxParamPropChoiceCascading) != 0;
    choice->setCascading(cascading);
    QObject::connect( choice.get(), SIGNAL(populated()), this, SLOT(onChoiceMenuPopulated()) );
    QObject::connect( choice.get(), SIGNAL(entryAppended()), this, SLOT(onChoiceMenuEntryAppended()) );
    QObject::connect( choice.get(), SIGNAL(entriesReset()), this, SLOT(onChoiceMenuReset()) );
}

OfxStatus
OfxChoiceInstance::get(int & v)
{
    KnobChoicePtr knob = resolveRenderKnob<KnobChoice>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValue(DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::get(OfxTime time,
                       int & v)
{
    assert( KnobChoice::canAnimateStatic() );
    KnobChoicePtr knob = resolveRenderKnob<KnobChoice>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    v = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxChoiceInstance::set(int v)
{
    KnobChoicePtr knob = resolveRenderKnob<KnobChoice>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    int nEntries = knob->getNumEntries();
    if ( (0 <= v) && ( v < nEntries ) ) {
        knob->setValue(v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);


        return kOfxStatOK;
    } else {
        return kOfxStatErrBadIndex;
    }
}

OfxStatus
OfxChoiceInstance::set(OfxTime time,
                       int v)
{
    KnobChoicePtr knob = resolveRenderKnob<KnobChoice>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    int nEntries = knob->getNumEntries();
    if ( (0 <= v) && ( v < nEntries ) ) {
        knob->setValueAtTime(TimeValue(time), v, knob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);


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
    knob->setDefaultValueWithoutApplying(_properties.getIntProperty(kOfxParamPropDefault, 0));
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
    knob->setEnabled( getEnabled() );
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
    if (!knob) {
        return;
    }
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobChoice>(_knob.lock()), nKeys);
}

OfxStatus
OfxChoiceInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobChoice>(_knob.lock()), nth, time);
}

OfxStatus
OfxChoiceInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobChoice>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxChoiceInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobChoice>(_knob.lock()), time);
}

OfxStatus
OfxChoiceInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobChoice>(_knob.lock()) );
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
    KnobColorPtr knob = getOrCreateKnob<KnobColor>(descriptor.getName(), this, 4);

    _knob = knob;



    setRange();
    setDisplayRange();

    const int dims = 4;
    std::vector<double> def(dims);

    properties.getDoublePropertyN(kOfxParamPropDefault, &def[0], dims);
    knob->setDefaultValues(def, DimIdx(0));


    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);

        knob->setDimensionName(DimIdx(i), dimensionName);
    }

    // [FD] I think all color parameters should be folded automatically:
    // Grade and ColorCorrect are some of the most used nodes,
    // and they have "A" channel uncked by default, so we don't care if colors
    // are folded.
    // The only case where this could matter are the "Draw" nodes (Rectangle, Constant, etc.),
    // but these are rarely used compared to "Color" nodes.
    knob->setCanAutoFoldDimensions(true);
}

OfxStatus
OfxRGBAInstance::get(double & r,
                     double & g,
                     double & b,
                     double & a)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getValue(DimIdx(0), view);
    g = knob->getValue(DimIdx(1), view);
    b = knob->getValue(DimIdx(2), view);
    a = knob->getValue(DimIdx(3), view);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::get(OfxTime time,
                     double &r,
                     double & g,
                     double & b,
                     double & a)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }

    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getValueAtTime( TimeValue(time), DimIdx(0), view );
    g = knob->getValueAtTime( TimeValue(time), DimIdx(1), view );
    b = knob->getValueAtTime( TimeValue(time), DimIdx(2), view );
    a = knob->getValueAtTime( TimeValue(time), DimIdx(3), view );

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(double r,
                     double g,
                     double b,
                     double a)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(4);
    vals[0] = r;
    vals[1] = g;
    vals[2] = b;
    vals[3] = a;
    knob->setValueAcrossDimensions(vals, DimIdx(0), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::set(OfxTime time,
                     double r,
                     double g,
                     double b,
                     double a)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(4);
    vals[0] = r;
    vals[1] = g;
    vals[2] = b;
    vals[3] = a;
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(0), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBAInstance::derive(OfxTime time,
                        double &r,
                        double & g,
                        double & b,
                        double & a)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(0));
    g = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(1));
    b = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(2));
    a = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(3));

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
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(0));
    g = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(1));
    b = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(2));
    a = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(3));

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
    knob->setEnabled( getEnabled() );
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
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::vector<double> color(4);
    _properties.getDoublePropertyN(kOfxParamPropDefault, &color[0], 4);

    knob->setDefaultValuesWithoutApplying(color, DimIdx(0));
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
    knob->setDisplayRangeAcrossDimensions(displayMins, displayMaxs);
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
    knob->setRangeAcrossDimensions(mins, maxs);
}

KnobIPtr
OfxRGBAInstance::getKnob() const
{
    return _knob.lock();
}


OfxStatus
OfxRGBAInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobColor>(_knob.lock()), nKeys);
}

OfxStatus
OfxRGBAInstance::getKeyTime(int nth,
                            OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobColor>(_knob.lock()), nth, time);
}

OfxStatus
OfxRGBAInstance::getKeyIndex(OfxTime time,
                             int direction,
                             int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobColor>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxRGBAInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobColor>(_knob.lock()), time);
}

OfxStatus
OfxRGBAInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobColor>(_knob.lock()) );
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
    KnobColorPtr color  = getOrCreateKnob<KnobColor>(descriptor.getName(), this, 3);

    _knob = color;

    const int dims = 3;

    std::vector<double> defValues(dims);
    properties.getDoublePropertyN(kOfxParamPropDefault, &defValues[0], dims);

    color->setDefaultValues(defValues, DimIdx(0));

    setRange();
    setDisplayRange();


    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    for (int i = 0; i < dims; ++i) {
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        color->setDimensionName(DimIdx(i), dimensionName);
    }

    // [FD] I think all color parameters should be folded automatically:
    // Grade and ColorCorrect are some of the most used nodes,
    // and they have "A" channel uncked by default, so we don't care if colors
    // are folded.
    // The only case where this could matter are the "Draw" nodes (Rectangle, Constant, etc.),
    // but these are rarely used compared to "Color" nodes.
    color->setCanAutoFoldDimensions(true);
}

OfxStatus
OfxRGBInstance::get(double & r,
                    double & g,
                    double & b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getValue(DimIdx(0), view);
    g = knob->getValue(DimIdx(1), view);
    b = knob->getValue(DimIdx(2), view);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::get(OfxTime time,
                    double & r,
                    double & g,
                    double & b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);
    g = knob->getValueAtTime(TimeValue(time), DimIdx(1), view);
    b = knob->getValueAtTime(TimeValue(time), DimIdx(2), view);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(double r,
                    double g,
                    double b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(3);
    vals[0] = r;
    vals[1] = g;
    vals[2] = b;
    knob->setValueAcrossDimensions(vals, DimIdx(0), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::set(OfxTime time,
                    double r,
                    double g,
                    double b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(3);
    vals[0] = r;
    vals[1] = g;
    vals[2] = b;
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(0), view, eValueChangedReasonPluginEdited);
    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::derive(OfxTime time,
                       double & r,
                       double & g,
                       double & b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(0));
    g = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(1));
    b = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(2));

    return kOfxStatOK;
}

OfxStatus
OfxRGBInstance::integrate(OfxTime time1,
                          OfxTime time2,
                          double &r,
                          double & g,
                          double & b)
{
    KnobColorPtr knob = resolveRenderKnob<KnobColor>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    r = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(0));
    g = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(1));
    b = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(2));

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
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::vector<double> color(3);
    _properties.getDoublePropertyN(kOfxParamPropDefault, &color[0], 3);

    knob->setDefaultValuesWithoutApplying(color);
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
    knob->setEnabled( getEnabled() );
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
    knob->setDisplayRangeAcrossDimensions(displayMins, displayMaxs);
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
    knob->setRangeAcrossDimensions(mins, maxs);
}

KnobIPtr
OfxRGBInstance::getKnob() const
{
    return _knob.lock();
}

OfxStatus
OfxRGBInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobColor>(_knob.lock()), nKeys);
}

OfxStatus
OfxRGBInstance::getKeyTime(int nth,
                           OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobColor>(_knob.lock()), nth, time);
}

OfxStatus
OfxRGBInstance::getKeyIndex(OfxTime time,
                            int direction,
                            int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobColor>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxRGBInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobColor>(_knob.lock()), time);
}

OfxStatus
OfxRGBInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobColor>(_knob.lock()) );
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
        dblKnob = getOrCreateKnob<KnobDouble>(paramName, this, knobDims);
    }
    _knob = dblKnob;
    setRange();
    setDisplayRange();

    const std::string & doubleType = getDoubleType();
    if ( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ) {
        dblKnob->setValueIsNormalized(DimIdx(0), eValueIsNormalizedX);
        dblKnob->setValueIsNormalized(DimIdx(1), eValueIsNormalizedY);
    }
    // disable slider if the type is an absolute position
    if ( (doubleType == kOfxParamDoubleTypeXYAbsolute) ||
         ( doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ||
         ( isRectangleType > 0) ) {
        dblKnob->disableSlider();
    }

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
        dblKnob->setDimensionName(DimIdx(i + _startIndex), dimensionName);

        dblKnob->setIncrement(incr, DimIdx(_startIndex + i));
        dblKnob->setDecimals(decimals[i], DimIdx(_startIndex + i));
    }

    // Only create native overlays if there is no interact or kOfxParamPropUseHostOverlayHandle is set
    // see https://github.com/MrKepzie/Natron/issues/932
    // only create automatic overlay for kOfxParamDoubleTypeXYAbsolute and kOfxParamDoubleTypeNormalisedXYAbsolute
    if ( ( !node->effectInstance()->getOverlayInteractMainEntry() && !isRectangleType &&
           ( ( getDoubleType() == kOfxParamDoubleTypeXYAbsolute) ||
             ( getDoubleType() == kOfxParamDoubleTypeNormalisedXYAbsolute) ) ) ||
         ( properties.getIntProperty(kOfxParamPropUseHostOverlayHandle) == 1) ) {
        // Natron 2 version:
        dblKnob->setHasHostOverlayHandle(true);
        // Natron 3 version:
        /*
        boost::shared_ptr<PointOverlayInteract> interact(new PointOverlayInteract());
        std::map<std::string,std::string> knobs;
        knobs["position"] = dblKnob->getName();
        node->registerOverlay(eOverlayViewportTypeViewer, interact, knobs);
         */
    }

    // Position knobs should not have their dimensions folded by default (e.g: The Translate parameter of a Transform node is not
    // expected to be folded by default, but the Size of a Blur node is.)
    // Only auto-merge dimensions and show a single slider if this is a scale or a size.
    dblKnob->setCanAutoFoldDimensions( (doubleType == kOfxParamDoubleTypeScale) ||
                                       (doubleType == kOfxParamDoubleTypeXY) ||
                                       (doubleType == kOfxParamDoubleTypeNormalisedXY) );

    dblKnob->setSpatial( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
                         (doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ||
                         (doubleType == kOfxParamDoubleTypeXY) ||
                         (doubleType == kOfxParamDoubleTypeXYAbsolute) );
    if ( (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
         (doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) ) {
        dblKnob->setValueIsNormalized(DimIdx(0 + _startIndex), eValueIsNormalizedX);
        dblKnob->setValueIsNormalized(DimIdx(1 + _startIndex), eValueIsNormalizedY);
    }
    dblKnob->setDefaultValuesAreNormalized( (coordSystem == kOfxParamCoordinatesNormalised) ||
                                            (doubleType == kOfxParamDoubleTypeNormalisedXY) ||
                                            (doubleType == kOfxParamDoubleTypeNormalisedXYAbsolute) );
    dblKnob->setDefaultValues(def, DimIdx(_startIndex));
}

OfxStatus
OfxDouble2DInstance::get(double & x1,
                         double & x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValue(DimIdx(_startIndex), view);
    x2 = knob->getValue(DimIdx(_startIndex + 1), view);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::get(OfxTime time,
                         double & x1,
                         double & x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValueAtTime(TimeValue(time), DimIdx(_startIndex), view);
    x2 = knob->getValueAtTime(TimeValue(time), DimIdx(_startIndex + 1), view);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(double x1,
                         double x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    std::vector<double> vals(2);
    vals[0] = x1;
    vals[1] = x2;
    ViewIdx view = knob->getCurrentRenderView();
    knob->setValueAcrossDimensions(vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::set(OfxTime time,
                         double x1,
                         double x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    std::vector<double> vals(2);
    vals[0] = x1;
    vals[1] = x2;
    ViewIdx view = knob->getCurrentRenderView();
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::derive(OfxTime time,
                            double &x1,
                            double & x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(_startIndex));
    x2 = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(1 + _startIndex));

    return kOfxStatOK;
}

OfxStatus
OfxDouble2DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(_startIndex));
    x2 = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(1 + _startIndex));

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
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::vector<double> color(2);
    _properties.getDoublePropertyN(kOfxParamPropDefault, &color[0], 2);

    knob->setDefaultValuesWithoutApplying(color);
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
    knob->setEnabled( getEnabled() );
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
    for (int i = 0; i < 2; ++i) {
        knob->setDisplayRange(displayMins[i], displayMaxs[i], DimIdx(i + _startIndex));
    }
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
    for (int i = 0; i < 2; ++i) {
        knob->setRange(mins[i], maxs[i], DimIdx(i + _startIndex));
    }
}

KnobIPtr
OfxDouble2DInstance::getKnob() const
{
    return _knob.lock();
}


OfxStatus
OfxDouble2DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobDouble>(_knob.lock()), nKeys, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobDouble>(_knob.lock()), nth, time, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobDouble>(_knob.lock()), time, direction, index, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobDouble>(_knob.lock()), time, _startIndex, _startIndex + 2);
}

OfxStatus
OfxDouble2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(resolveRenderKnob<KnobDouble>(_knob.lock()), _startIndex, _startIndex + 2);
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
        iKnob = getOrCreateKnob<KnobInt>(paramName, this, knobDims);
    }
    _knob = iKnob;
    setRange();
    setDisplayRange();

    if (isRectangleType) {
        iKnob->setAsRectangle();
    }

    std::vector<int> increment(ofxDims);
    std::vector<int> def(ofxDims);


    properties.getIntPropertyN(kOfxParamPropDefault, &def[0], ofxDims);
    for (int i = 0; i < ofxDims; ++i) {
        increment[i] = 1; // kOfxParamPropIncrement only exists for Double
        std::string dimensionName = properties.getStringProperty(kOfxParamPropDimensionLabel, i);
        iKnob->setDimensionName(DimIdx(i + _startIndex), dimensionName);
        iKnob->setIncrement(increment[i], DimIdx(_startIndex + i));

    }

    iKnob->setDefaultValues(def, DimIdx(_startIndex));

    // int knobs never auto-fold
    // We could have a kOfxParamPropDoubleType property on ints to specify if it's a scale or size
    iKnob->setCanAutoFoldDimensions(false /*doubleType == kOfxParamDoubleTypeScale*/);
}

OfxStatus
OfxInteger2DInstance::get(int & x1,
                          int & x2)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValue(DimIdx(_startIndex), view);
    x2 = knob->getValue(DimIdx(_startIndex + 1), view);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::get(OfxTime time,
                          int & x1,
                          int & x2)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValueAtTime(TimeValue(time), DimIdx(_startIndex), view);
    x2 = knob->getValueAtTime(TimeValue(time), DimIdx(_startIndex + 1), view);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(int x1,
                          int x2)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<int> vals(2);
    vals[0] = x1;
    vals[1] = x2;
    knob->setValueAcrossDimensions(vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxInteger2DInstance::set(OfxTime time,
                          int x1,
                          int x2)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<int> vals(2);
    vals[0] = x1;
    vals[1] = x2;
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);

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
    knob->setEnabled( getEnabled() );
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
    for (int i = 0; i < 2; ++i) {
        knob->setDisplayRange(displayMins[i], displayMaxs[i], DimIdx(i + _startIndex));
    }
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
    for (int i = 0; i < 2; ++i) {
        knob->setRange(mins[i], maxs[i], DimIdx(i + _startIndex));
    }
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
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    std::vector<int> values(2);
    _properties.getIntPropertyN(kOfxParamPropDefault, &values[0], 2);

    knob->setDefaultValuesWithoutApplying(values);
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobInt>(_knob.lock()), nKeys, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::getKeyTime(int nth,
                                 OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobInt>(_knob.lock()), nth, time, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::getKeyIndex(OfxTime time,
                                  int direction,
                                  int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobInt>(_knob.lock()), time, direction, index, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobInt>(_knob.lock()), time, _startIndex, 2 + _startIndex);
}

OfxStatus
OfxInteger2DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(resolveRenderKnob<KnobInt>(_knob.lock()), _startIndex, 2 + _startIndex);
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
        knob = getOrCreateKnob<KnobDouble>(paramName, this, knobDims);
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
        knob->setDimensionName(DimIdx(_startIndex + i), dimensionName);
        knob->setIncrement(incr, DimIdx(_startIndex + i));
        knob->setDecimals(decimals[i], DimIdx(_startIndex + i));
    }

    knob->setDefaultValues(def, DimIdx(_startIndex));
}

OfxStatus
OfxDouble3DInstance::get(double & x1,
                         double & x2,
                         double & x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValue(DimIdx(0 + _startIndex), view);
    x2 = knob->getValue(DimIdx(1 + _startIndex), view);
    x3 = knob->getValue(DimIdx(2 + _startIndex), view);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::get(OfxTime time,
                         double & x1,
                         double & x2,
                         double & x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValueAtTime(TimeValue(time), DimIdx(0 + _startIndex), view);
    x2 = knob->getValueAtTime(TimeValue(time), DimIdx(1 + _startIndex), view);
    x3 = knob->getValueAtTime(TimeValue(time), DimIdx(2 + _startIndex), view);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(double x1,
                         double x2,
                         double x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(3);
    vals[0] = x1;
    vals[1] = x2;
    vals[2] = x3;
    knob->setValueAcrossDimensions(vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::set(OfxTime time,
                         double x1,
                         double x2,
                         double x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<double> vals(3);
    vals[0] = x1;
    vals[1] = x2;
    vals[2] = x3;
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(_startIndex), view, eValueChangedReasonPluginEdited);


    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::derive(OfxTime time,
                            double & x1,
                            double & x2,
                            double & x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(0 + _startIndex));
    x2 = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(1 + _startIndex));
    x3 = knob->getDerivativeAtTime(TimeValue(time), view, DimIdx(2 + _startIndex));

    return kOfxStatOK;
}

OfxStatus
OfxDouble3DInstance::integrate(OfxTime time1,
                               OfxTime time2,
                               double &x1,
                               double & x2,
                               double & x3)
{
    KnobDoublePtr knob = resolveRenderKnob<KnobDouble>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(0 + _startIndex));
    x2 = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(1 + _startIndex));
    x3 = knob->getIntegrateFromTimeToTime(TimeValue(time1), TimeValue(time2), view, DimIdx(2 + _startIndex));

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
    std::vector<double> values(3);
    for (int i = 0; i < 3; ++i) {
        values[i] = _properties.getDoubleProperty(kOfxParamPropDefault, i);
    }
    KnobDoublePtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(values);
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
    knob->setEnabled( getEnabled() );
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
    for (int i = 0; i < 3; ++i) {
        knob->setDisplayRange(displayMins[i], displayMaxs[i], DimIdx(i + _startIndex));
    }
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
    for (int i = 0; i < 3; ++i) {
        knob->setRange(mins[i], maxs[i], DimIdx(i + _startIndex));
    }
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

OfxStatus
OfxDouble3DInstance::getNumKeys(unsigned int &nKeys) const
{
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobDouble>(_knob.lock()), nKeys, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::getKeyTime(int nth,
                                OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobDouble>(_knob.lock()), nth, time, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::getKeyIndex(OfxTime time,
                                 int direction,
                                 int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobDouble>(_knob.lock()), time, direction, index, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobDouble>(_knob.lock()), time, _startIndex, 3 + _startIndex);
}

OfxStatus
OfxDouble3DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(resolveRenderKnob<KnobDouble>(_knob.lock()), _startIndex, 3 + _startIndex);
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
    KnobIntPtr knob = getOrCreateKnob<KnobInt>(descriptor.getName(), this, dims);

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
        knob->setDimensionName(DimIdx(i), dimensionName);
    }

    knob->setIncrement(increment);

    knob->setDefaultValues(def, DimIdx(0));

    // int knobs never auto-fold
    // We could have a kOfxParamPropDoubleType property on ints to specify if it's a scale or size
    knob->setCanAutoFoldDimensions(false /*doubleType == kOfxParamDoubleTypeScale*/);
}

OfxStatus
OfxInteger3DInstance::get(int & x1,
                          int & x2,
                          int & x3)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValue(DimIdx(0), view);
    x2 = knob->getValue(DimIdx(1), view);
    x3 = knob->getValue(DimIdx(2), view);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::get(OfxTime time,
                          int & x1,
                          int & x2,
                          int & x3)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    x1 = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);
    x2 = knob->getValueAtTime(TimeValue(time), DimIdx(1), view);
    x3 = knob->getValueAtTime(TimeValue(time), DimIdx(2), view);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(int x1,
                          int x2,
                          int x3)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<int> vals(3);
    vals[0] = x1;
    vals[1] = x2;
    vals[2] = x3;
    knob->setValueAcrossDimensions(vals, DimIdx(0), view, eValueChangedReasonPluginEdited);

    return kOfxStatOK;
}

OfxStatus
OfxInteger3DInstance::set(OfxTime time,
                          int x1,
                          int x2,
                          int x3)
{
    KnobIntPtr knob = resolveRenderKnob<KnobInt>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    std::vector<int> vals(3);
    vals[0] = x1;
    vals[1] = x2;
    vals[2] = x3;
    knob->setValueAtTimeAcrossDimensions(TimeValue(time), vals, DimIdx(0), view, eValueChangedReasonPluginEdited);

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
    std::vector<int> values(3);
    for (int i = 0; i < 3; ++i) {
        values[i] = _properties.getIntProperty(kOfxParamPropDefault, i);
    }
    KnobIntPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    knob->setDefaultValuesWithoutApplying(values);
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
    knob->setEnabled( getEnabled() );
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
    knob->setDisplayRangeAcrossDimensions(displayMins, displayMaxs);
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
    knob->setRangeAcrossDimensions(mins, maxs);
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobInt>(_knob.lock()), nKeys);
}

OfxStatus
OfxInteger3DInstance::getKeyTime(int nth,
                                 OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobInt>(_knob.lock()), nth, time);
}

OfxStatus
OfxInteger3DInstance::getKeyIndex(OfxTime time,
                                  int direction,
                                  int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobInt>(_knob.lock()), time, direction, index);
}

OfxStatus
OfxInteger3DInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobInt>(_knob.lock()), time);
}

OfxStatus
OfxInteger3DInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys(resolveRenderKnob<KnobInt>(_knob.lock()));
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
    KnobGroupPtr group = getOrCreateKnob<KnobGroup>(descriptor.getName(), this, 1);

    _groupKnob = group;
    int opened = properties.getIntProperty(kOfxParamPropGroupOpen);
    if (isTab) {
        group->setAsTab();
    }

    group->setDefaultValue(opened);
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
    _groupKnob.lock()->setEnabled( getEnabled() );
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
    _groupKnob.lock()->setValue((bool)opened, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
}

////////////////////////// OfxPageInstance /////////////////////////////////////////////////


OfxPageInstance::OfxPageInstance(const OfxEffectInstancePtr& node,
                                 OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::PageInstance( descriptor, node->effectInstance() )
    , _pageKnob()
{
    _pageKnob = getOrCreateKnob<KnobPage>(descriptor.getName(), this, 1);
}

// callback which should set enabled state as appropriate
void
OfxPageInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setEnabled( getEnabled() );
}

// callback which should set secret state as appropriate
void
OfxPageInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    _pageKnob.lock()->setEnabled( getSecret() );
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
    KnobStringWPtr stringKnob;
    KnobPathWPtr pathKnob;
    boost::shared_ptr<TLSHolder<OfxParamToKnob::OfxParamTLSData> > tlsData;

    OfxStringInstancePrivate()
        : fileKnob()
        , stringKnob()
        , pathKnob()
        , tlsData()
    {
        tlsData = boost::make_shared<TLSHolder<OfxParamToKnob::OfxParamTLSData> >();
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
    KnobFilePtr fileKnob;
    KnobPathPtr pathKnob;
    KnobStringPtr stringKnob;

    if (mode == kOfxParamStringIsFilePath) {
        int fileIsImage = ( ( node->isReader() ||
                              node->isWriter() ) &&
                            (getScriptName() == kOfxImageEffectFileParamName ||
                             getScriptName() == kOfxImageEffectProxyParamName) );
        int fileIsOutput = !properties.getIntProperty(kOfxParamPropStringFilePathExists);

        fileKnob = getOrCreateKnob<KnobFile>(descriptor.getName(), this, 1);
        _imp->fileKnob = fileKnob;

        if (!fileIsOutput) {
            fileKnob->setDialogType(fileIsImage ? KnobFile::eKnobFileDialogTypeOpenFileSequences
                                : KnobFile::eKnobFileDialogTypeOpenFile);

        } else {
            fileKnob->setDialogType(fileIsImage ? KnobFile::eKnobFileDialogTypeSaveFileSequences
                                : KnobFile::eKnobFileDialogTypeSaveFile);
        }
        if (!fileIsImage) {
            fileKnob->setAnimationEnabled(false);
        } else {
            std::vector<std::string> filters;
            appPTR->getSupportedReaderFileFormats(&filters);
            fileKnob->setDialogFilters(filters);
        }
    } else if (mode == kOfxParamStringIsDirectoryPath) {
        pathKnob = getOrCreateKnob<KnobPath>(descriptor.getName(), this, 1);
        _imp->pathKnob = pathKnob;
        pathKnob->setMultiPath(false);
    } else if ( (mode == kOfxParamStringIsSingleLine) || (mode == kOfxParamStringIsLabel) || (mode == kOfxParamStringIsMultiLine) || richText ) {
        stringKnob = getOrCreateKnob<KnobString>(descriptor.getName(), this, 1);
        _imp->stringKnob = stringKnob;
        if (mode == kOfxParamStringIsLabel) {

            stringKnob->setEnabled(false);
            stringKnob->setAsLabel();
         }
        if ( (mode == kOfxParamStringIsMultiLine) || richText ) {
            ///only QTextArea support rich text anyway
            stringKnob->setUsesRichText(richText);
            stringKnob->setAsMultiLine();
        }
    }
    std::string defaultVal = properties.getStringProperty(kOfxParamPropDefault);
    if ( !defaultVal.empty() ) {
        if (fileKnob) {
            KnobFilePtr k = _imp->fileKnob.lock();
            k->setDefaultValue(defaultVal, DimIdx(0));
        } else if (stringKnob) {
            KnobStringPtr k = _imp->stringKnob.lock();
            k->setDefaultValue(defaultVal, DimIdx(0));
        } else if (pathKnob) {
            KnobPathPtr k = _imp->pathKnob.lock();
            k->setDefaultValue(defaultVal, DimIdx(0));
        }
    }
}

OfxStringInstance::~OfxStringInstance()
{
}


OfxStatus
OfxStringInstance::get(std::string &str)
{
    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }

    assert(fileKnob || strknob || pathKnob);
    if (fileKnob) {
        str = fileKnob->getValue();
    } else if (strknob) {
        str = strknob->getValue();
    } else if (pathKnob) {
        str = pathKnob->getValue();
    } else {
        return kOfxStatErrBadHandle;
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::get(OfxTime time,
                       std::string & str)
{
    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }

    assert(fileKnob || strknob || pathKnob);
    if (fileKnob) {
        str = fileKnob->getValueAtTime(TimeValue(time), DimIdx(0), fileKnob->getCurrentRenderView());
    } else if (strknob) {
        str = strknob->getValueAtTime(TimeValue(time), DimIdx(0), strknob->getCurrentRenderView());
    } else if (pathKnob) {
        str = pathKnob->getValueAtTime(TimeValue(time), DimIdx(0), pathKnob->getCurrentRenderView());
    } else {
        return kOfxStatErrBadHandle;
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(const char* str)
{
    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (fileKnob) {
        std::string s(str);
        fileKnob->setValue(s, fileKnob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else if (strknob) {
        strknob->setValue(str, strknob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else if (pathKnob) {
        std::string s(str);
        pathKnob->setValue(s, pathKnob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else {
        return kOfxStatErrBadHandle;
    }

    return kOfxStatOK;
}

OfxStatus
OfxStringInstance::set(OfxTime time,
                       const char* str)
{
    assert( KnobString::canAnimateStatic() );

    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (fileKnob) {
        std::string s(str);
        fileKnob->setValueAtTime(TimeValue(time), s, fileKnob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else if (strknob) {
        strknob->setValueAtTime(TimeValue(time), str, strknob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else if (pathKnob) {
        std::string s(str);
        pathKnob->setValueAtTime(TimeValue(time), s, pathKnob->getCurrentRenderView(), DimIdx(0), eValueChangedReasonPluginEdited, 0);
    } else {
        return kOfxStatErrBadHandle;
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
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        return fileKnob;
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        return stringKnob;
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        return pathKnob;
    }

    return KnobIPtr();
}

// callback which should set enabled state as appropriate
void
OfxStringInstance::setEnabled()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setEnabled( getEnabled() );
        return;
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setEnabled( getEnabled() );
        return;
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setEnabled( getEnabled() );
        return;
    }
}

void
OfxStringInstance::setHint()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setHintToolTip(getHint());
        return;
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setHintToolTip(getHint());
        return;
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setHintToolTip(getHint());
        return;
    }
}

void
OfxStringInstance::setDefault()
{
    DYNAMIC_PROPERTY_CHECK();
    const std::string& v = _properties.getStringProperty(kOfxParamPropDefault, 0);

    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setDefaultValueWithoutApplying(v);
        return;
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setDefaultValueWithoutApplying(v);
        return;
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setDefaultValueWithoutApplying(v);
        return;
    }
}

void
OfxStringInstance::setLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setLabel( getParamLabel(this) );
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setLabel( getParamLabel(this) );
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setLabel( getParamLabel(this) );
    }
}

// callback which should set secret state as appropriate
void
OfxStringInstance::setSecret()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setSecret( getSecret() );
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setSecret( getSecret() );
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setSecret( getSecret() );
    }
}


void
OfxStringInstance::setInViewportSecret()
{
    DYNAMIC_PROPERTY_CHECK();

    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setInViewerContextSecret( getProperties().getIntProperty(kNatronOfxParamPropInViewerContextSecret) );
    }
}

void
OfxStringInstance::setInViewportLabel()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setInViewerContextLabel(QString::fromUtf8(getProperties().getStringProperty(kNatronOfxParamPropInViewerContextLabel).c_str()));
    }

}

void
OfxStringInstance::setEvaluateOnChange()
{
    DYNAMIC_PROPERTY_CHECK();
    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob->setEvaluateOnChange( getEvaluateOnChange() );
    }
    KnobStringPtr stringKnob = _imp->stringKnob.lock();
    if (stringKnob) {
        stringKnob->setEvaluateOnChange( getEvaluateOnChange() );
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob->setEvaluateOnChange( getEvaluateOnChange() );
    }
}

OfxStatus
OfxStringInstance::getNumKeys(unsigned int &nKeys) const
{
    KnobIPtr knob;

    KnobFilePtr fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    KnobStringPtr strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    KnobPathPtr pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (strknob) {
        knob = boost::dynamic_pointer_cast<KnobI>(strknob);
    } else if (fileKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(fileKnob);
    } else if (pathKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(pathKnob);
    } else {
        return kOfxStatErrBadHandle;
    }

    return OfxKeyFrame::getNumKeys(knob, nKeys);
}

OfxStatus
OfxStringInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    KnobIPtr knob;

    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (strknob) {
        knob = boost::dynamic_pointer_cast<KnobI>(strknob);
    } else if (fileKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(fileKnob);
    } else if (pathKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(pathKnob);
    } else {
        return kOfxStatErrBadHandle;
    }

    return OfxKeyFrame::getKeyTime(knob, nth, time);
}

OfxStatus
OfxStringInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    KnobIPtr knob;

    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (strknob) {
        knob = boost::dynamic_pointer_cast<KnobI>(strknob);
    } else if (fileKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(fileKnob);
    } else if (pathKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(pathKnob);
    } else {
        return kOfxStatErrBadHandle;
    }

    return OfxKeyFrame::getKeyIndex(knob, time, direction, index);
}

OfxStatus
OfxStringInstance::deleteKey(OfxTime time)
{
    KnobIPtr knob;

    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (strknob) {
        knob = boost::dynamic_pointer_cast<KnobI>(strknob);
    } else if (fileKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(fileKnob);
    } else if (pathKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(pathKnob);
    } else {
        return kOfxStatErrBadHandle;
    }

    return OfxKeyFrame::deleteKey(knob, time);
}

OfxStatus
OfxStringInstance::deleteAllKeys()
{
    KnobIPtr knob;

    KnobFilePtr fileKnob;
    KnobStringPtr strknob;
    KnobPathPtr pathKnob;

    fileKnob = _imp->fileKnob.lock();
    if (fileKnob) {
        fileKnob = resolveRenderKnob<KnobFile>(fileKnob);
    }
    strknob = _imp->stringKnob.lock();
    if (strknob) {
        strknob = resolveRenderKnob<KnobString>(strknob);
    }
    pathKnob = _imp->pathKnob.lock();
    if (pathKnob) {
        pathKnob = resolveRenderKnob<KnobPath>(pathKnob);
    }
    assert(fileKnob || strknob || pathKnob);
    if (strknob) {
        knob = boost::dynamic_pointer_cast<KnobI>(strknob);
    } else if (fileKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(fileKnob);
    } else if (pathKnob) {
        knob = boost::dynamic_pointer_cast<KnobI>(pathKnob);
    } else {
        return kOfxStatErrBadHandle;
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
        , tlsData()
    {
        tlsData = boost::make_shared<TLSHolder<OfxParamToKnob::OfxParamTLSData> >();
    }
};

OfxCustomInstance::OfxCustomInstance(const OfxEffectInstancePtr& node,
                                     OFX::Host::Param::Descriptor & descriptor)
    : OfxParamToKnob(node)
    , OFX::Host::Param::CustomInstance( descriptor, node->effectInstance() )
    , _imp( new OfxCustomInstancePrivate() )
{
    const OFX::Host::Property::Set &properties = getProperties();
    KnobStringPtr knob = getOrCreateKnob<KnobString>(descriptor.getName(), this, 1);

    _imp->knob = knob;

    knob->setAsCustom();
    knob->setDefaultValue(properties.getStringProperty(kOfxParamPropDefault));
    GCC_DIAG_PEDANTIC_OFF
    _imp->customParamInterpolationV1Entry = (customParamInterpolationV1Entry_t)properties.getPointerProperty(kOfxParamPropCustomInterpCallbackV1);
    GCC_DIAG_PEDANTIC_ON

    if (_imp->customParamInterpolationV1Entry) {
        knob->setAnimationEnabled(true);
        CurvePtr interpolator = knob->getAnimationCurve(ViewIdx(0), DimIdx(0));

        boost::shared_ptr<OfxCustomStringKeyFrameInterpolator> interp(new OfxCustomStringKeyFrameInterpolator(_imp->customParamInterpolationV1Entry, (void*)getHandle(), getName()));
        interpolator->setInterpolator(interp);
    }
}

OfxCustomInstance::~OfxCustomInstance()
{
}

OfxStatus
OfxCustomInstance::get(std::string &str)
{
    KnobStringPtr knob = resolveRenderKnob<KnobString>(_imp->knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    str = knob->getValue(DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::get(OfxTime time,
                       std::string & str)
{
    assert( KnobString::canAnimateStatic() );
    // it should call _customParamInterpolationV1Entry
    KnobStringPtr knob = resolveRenderKnob<KnobString>(_imp->knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    str = knob->getValueAtTime(TimeValue(time), DimIdx(0), view);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(const char* str)
{
    KnobStringPtr knob = resolveRenderKnob<KnobString>(_imp->knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    knob->setValue(str, view, DimIdx(0), eValueChangedReasonPluginEdited, 0);

    return kOfxStatOK;
}

OfxStatus
OfxCustomInstance::set(OfxTime time,
                       const char* str)
{
    assert( KnobString::canAnimateStatic() );
    KnobStringPtr knob = resolveRenderKnob<KnobString>(_imp->knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    knob->setValueAtTime(TimeValue(time), str, view, DimIdx(0), eValueChangedReasonPluginEdited, 0);

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
    // Custom params are always disabled
    //_imp->knob.lock()->setEnabled( getEnabled() );
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
    _imp->knob.lock()->setDefaultValueWithoutApplying(_properties.getStringProperty(kOfxParamPropDefault, 0));
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
    return OfxKeyFrame::getNumKeys(resolveRenderKnob<KnobString>(_imp->knob.lock()), nKeys);
}

OfxStatus
OfxCustomInstance::getKeyTime(int nth,
                              OfxTime & time) const
{
    return OfxKeyFrame::getKeyTime(resolveRenderKnob<KnobString>(_imp->knob.lock()), nth, time);
}

OfxStatus
OfxCustomInstance::getKeyIndex(OfxTime time,
                               int direction,
                               int & index) const
{
    return OfxKeyFrame::getKeyIndex(resolveRenderKnob<KnobString>(_imp->knob.lock()), time, direction, index);
}

OfxStatus
OfxCustomInstance::deleteKey(OfxTime time)
{
    return OfxKeyFrame::deleteKey(resolveRenderKnob<KnobString>(_imp->knob.lock()), time);
}

OfxStatus
OfxCustomInstance::deleteAllKeys()
{
    return OfxKeyFrame::deleteAllKeys( resolveRenderKnob<KnobString>(_imp->knob.lock()) );
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
    KnobParametricPtr knob = getOrCreateKnob<KnobParametric>(descriptor.getName(), this, parametricDimension);


    _knob = knob;
    setRange();
    setDisplayRange();


    bool isPeriodic = (bool)properties.getIntProperty(kNatronOfxParamPropParametricIsPeriodic);
    knob->setPeriodic(isPeriodic);

    setLabel(); //set label on all curves

    std::vector<double> color(3 * parametricDimension);
    properties.getDoublePropertyN(kOfxParamPropParametricUIColour, &color[0], 3 * parametricDimension);

    
    for (int i = 0; i < parametricDimension; ++i) {
        knob->setCurveColor(DimIdx(i), color[i * 3], color[i * 3 + 1], color[i * 3 + 2]);
    }

    double range[2] = {0., 1.};
    getProperties().getDoublePropertyN(kOfxParamPropParametricRange, range, 2);
    if (!knob) {
        return;
    }
    if(range[1] > range[0]) {
        knob->setParametricRange(range[0], range[1]);
    }
    for (int i = 0; i < parametricDimension; ++i) {
        const std::string & curveName = getProperties().getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(DimIdx(i), curveName);
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
    knob->setEnabled( getEnabled() );
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
    for (int i = 0; i < knob->getNDimensions(); ++i) {
        const std::string & curveName = getProperties().getStringProperty(kOfxParamPropDimensionLabel, i);
        knob->setDimensionName(DimIdx(i), curveName);
    }
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
    int dims = knob->getNDimensions();
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    _properties.getDoublePropertyN(kOfxParamPropDisplayMin, &displayMins[0], displayMins.size());
    _properties.getDoublePropertyN(kOfxParamPropDisplayMax, &displayMaxs[0], displayMaxs.size());
    knob->setDisplayRangeAcrossDimensions(displayMins, displayMaxs);
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
    int dims = knob->getNDimensions();
    std::vector<double> mins(dims);
    std::vector<double> maxs(dims);
    _properties.getDoublePropertyN(kOfxParamPropMin, &mins[0], mins.size());
    _properties.getDoublePropertyN(kOfxParamPropMax, &maxs[0], maxs.size());
    knob->setRangeAcrossDimensions(mins, maxs);
}


OfxStatus
OfxParametricInstance::getValue(int curveIndex,
                                OfxTime /*time*/,
                                double parametricPosition,
                                double *returnValue)
{
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->evaluateCurve(DimIdx(curveIndex), view, parametricPosition, returnValue);

    if (!isFailureRetCode(stat)) {
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
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->getNControlPoints(DimIdx(curveIndex), view, returnValue);

    if (!isFailureRetCode(stat)) {
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
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->getNthControlPoint(DimIdx(curveIndex), view, nthCtl, key, value);

    if (!isFailureRetCode(stat)) {
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
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->setNthControlPoint(eValueChangedReasonPluginEdited, DimIdx(curveIndex), view, nthCtl, key, value);

    if (!isFailureRetCode(stat)) {
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

    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ActionRetCodeEnum stat;
    EffectInstancePtr effect = toEffectInstance( knob->getHolder() );
    KeyframeTypeEnum interpolation = eKeyframeTypeSmooth; // a reasonable default
    // The initial curve for some plugins may be better with a specific interpolation. Unfortunately, the kOfxParametricSuiteV1 doesn't offer different interpolation methods
#ifdef DEBUG
#pragma message WARN("This is a hack, we should finish up the ParametricParameter suite V2")
#endif
    if (effect) {
        NodePtr node = effect->getNode();
        if ( (node->getPluginID() == PLUGINID_OFX_COLORCORRECT) || (node->getPluginID() == PLUGINID_OFX_TIMEDISSOLVE) ) {
            interpolation = eKeyframeTypeHorizontal;
        } else if ( (node->getPluginID() == PLUGINID_OFX_COLORLOOKUP) || (node->getPluginID() == PLUGINID_OFX_RETIME) ) {
            interpolation = eKeyframeTypeCubic;
        }
    }
    stat = knob->addControlPoint(eValueChangedReasonPluginEdited, DimIdx(curveIndex), key, value, interpolation);

    if (!isFailureRetCode(stat)) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteControlPoint(int curveIndex,
                                          int nthCtl)
{
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->deleteControlPoint(eValueChangedReasonPluginEdited, DimIdx(curveIndex), view, nthCtl);

    if (!isFailureRetCode(stat)) {
        return kOfxStatOK;
    } else {
        return kOfxStatFailed;
    }
}

OfxStatus
OfxParametricInstance::deleteAllControlPoints(int curveIndex)
{
    KnobParametricPtr knob = resolveRenderKnob<KnobParametric>(_knob.lock());
    assert(knob);
    if (!knob) {
        return kOfxStatErrBadHandle;
    }
    ViewIdx view = knob->getCurrentRenderView();
    ActionRetCodeEnum stat = knob->deleteAllControlPoints(eValueChangedReasonPluginEdited, DimIdx(curveIndex), view);

    if (!isFailureRetCode(stat)) {
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
