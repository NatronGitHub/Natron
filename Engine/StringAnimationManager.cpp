/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "StringAnimationManager.h"

#include <set>
#include <cmath>
#include <cassert>
#include <stdexcept>

#include <QtCore/QMutex>

#include "Engine/Knob.h"

NATRON_NAMESPACE_ENTER;


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct StringKeyFrame
{
    std::string value;
    double time;
};

struct StringKeyFrame_compare_time
{
    bool operator() (const StringKeyFrame & lhs,
                     const StringKeyFrame & rhs) const
    {
        return lhs.time < rhs.time;
    }
};

typedef std::set<StringKeyFrame, StringKeyFrame_compare_time> Keyframes;
typedef std::map<ViewIdx, Keyframes> PerViewKeyFrames;

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct StringAnimationManagerPrivate
{
    StringAnimationManager::customParamInterpolationV1Entry_t customInterpolation;
    void* ofxParamHandle;
    mutable QMutex keyframesMutex;
    PerViewKeyFrames keyframes;

    // Weak ptr because it is encapsulated into the AnimatingKnobStringHelper class (inheriting KnobI)
    KnobIConstWPtr knob;

    StringAnimationManagerPrivate(const KnobIConstPtr& knob)
        : customInterpolation(NULL)
        , ofxParamHandle(NULL)
        , keyframesMutex()
        , keyframes()
        , knob(knob)
    {
    }
};

StringAnimationManager::StringAnimationManager(const KnobIConstPtr& knob)
    : _imp( new StringAnimationManagerPrivate(knob) )
{
}

StringAnimationManager::~StringAnimationManager()
{
}

void
StringAnimationManager::splitView(ViewIdx view)
{
    QMutexLocker k(&_imp->keyframesMutex);
    const Keyframes& mainViewKeys = _imp->keyframes[ViewIdx(0)];
    Keyframes& thisViewKeys = _imp->keyframes[view];
    thisViewKeys = mainViewKeys;
}

void
StringAnimationManager::unSplitView(ViewIdx view)
{
    QMutexLocker k(&_imp->keyframesMutex);
    PerViewKeyFrames::iterator foundView = _imp->keyframes.find(view);
    if (foundView == _imp->keyframes.end()) {
        return;
    }
    _imp->keyframes.erase(foundView);
}

bool
StringAnimationManager::hasCustomInterp() const
{
    return _imp->customInterpolation != NULL;
}

void
StringAnimationManager::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                               void* ofxParamHandle)
{
    _imp->customInterpolation = func;
    _imp->ofxParamHandle = ofxParamHandle;
}

bool
StringAnimationManager::customInterpolation(double time,
                                            ViewIdx view,
                                            std::string* ret) const
{
    QMutexLocker l(&_imp->keyframesMutex);

    assert(_imp->customInterpolation);
    ///if there's a single keyframe, return it

    if ( _imp->keyframes.empty() ) {
        return false;
    }

    const PerViewKeyFrames::const_iterator foundView = _imp->keyframes.find(view);
    if (foundView == _imp->keyframes.end()) {
        return false;
    }

    if (foundView->second.size() == 1) {
        *ret = foundView->second.begin()->value;

        return true;
    }

    /// get the keyframes surrounding the time
    Keyframes::const_iterator upper = foundView->second.end();
    Keyframes::const_iterator lower = foundView->second.end();
    for (Keyframes::const_iterator it = foundView->second.begin(); it != foundView->second.end(); ++it) {
        if (it->time > time) {
            upper = it;
            break;
        } else if (it->time == time) {
            ///if there's a keyframe exactly at this time, return its value
            *ret = it->value;

            return true;
        }
    }

    if ( upper == foundView->second.end() ) {
        ///if the time is greater than the time of all keyframes return the last

        --upper;
        *ret = upper->value;

        return true;
    } else if ( upper == foundView->second.begin() ) {
        ///if the time is lesser than the time of all keyframes, return the first
        *ret = upper->value;

        return true;
    } else {
        ///general case, we're in-between 2 keyframes
        lower = upper;
        --lower;
    }

    OFX::Host::Property::PropSpec inArgsSpec[] = {
        { kOfxPropName,    OFX::Host::Property::eString, 1, true, "" },
        { kOfxPropTime,    OFX::Host::Property::eDouble, 1, true, "" },
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 2, true, ""},
        { kOfxParamPropInterpolationTime,    OFX::Host::Property::eDouble, 2, true, "" },
        { kOfxParamPropInterpolationAmount,    OFX::Host::Property::eDouble, 1, true, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set inArgs(inArgsSpec);
    KnobIConstPtr knob = _imp->knob.lock();
    assert(knob);
    if (knob) {
        inArgs.setStringProperty( kOfxPropName, knob->getName() );
    }
    inArgs.setDoubleProperty(kOfxPropTime, time);

    inArgs.setStringProperty(kOfxParamPropCustomValue, lower->value, 0);
    inArgs.setStringProperty(kOfxParamPropCustomValue, upper->value, 1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, lower->time, 0);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, upper->time, 1);
    inArgs.setDoubleProperty( kOfxParamPropInterpolationAmount, (time - lower->time) / (double)(upper->time - lower->time) );


    OFX::Host::Property::PropSpec outArgsSpec[] = {
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 1, false, ""},
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set outArgs(outArgsSpec);

    l.unlock();
    _imp->customInterpolation( _imp->ofxParamHandle, inArgs.getHandle(), outArgs.getHandle() );

    *ret = outArgs.getStringProperty(kOfxParamPropCustomValue, 0).c_str();

    return true;
} // customInterpolation

void
StringAnimationManager::insertKeyFrame(double time,
                                       ViewIdx view,
                                       const std::string & v,
                                       double* index)
{
    StringKeyFrame k;

    k.time = time;
    k.value = v;
    QMutexLocker l(&_imp->keyframesMutex);
    Keyframes& keys = _imp->keyframes[view];

    std::pair<Keyframes::iterator, bool> ret = keys.insert(k);
    if (!ret.second) {
        keys.erase(ret.first);
        ret = keys.insert(k);
        assert(ret.second);
    }
    *index = std::distance(keys.begin(), ret.first);
}

void
StringAnimationManager::removeKeyFrame(double time, ViewIdx view)
{
    QMutexLocker l(&_imp->keyframesMutex);
    const PerViewKeyFrames::iterator foundView = _imp->keyframes.find(view);
    if (foundView == _imp->keyframes.end()) {
        return;
    }
    for (Keyframes::iterator it = foundView->second.begin(); it != foundView->second.end(); ++it) {
        if (it->time == time) {
            foundView->second.erase(it);

            return;
        }
    }
}


void
StringAnimationManager::removeKeyframes(const std::list<double>& keysRemoved, ViewIdx view)
{
    for (std::list<double>::const_iterator it = keysRemoved.begin(); it != keysRemoved.end(); ++it) {
        removeKeyFrame(*it, view);
    }
}


void
StringAnimationManager::clearKeyFrames()
{
    QMutexLocker l(&_imp->keyframesMutex);

    _imp->keyframes.clear();
}

void
StringAnimationManager::stringFromInterpolatedIndex(double interpolated,
                                                    ViewIdx view,
                                                    std::string* returnValue) const
{
    int index = std::floor(interpolated + 0.5);
    QMutexLocker l(&_imp->keyframesMutex);

    const PerViewKeyFrames::iterator foundView = _imp->keyframes.find(view);
    if (foundView == _imp->keyframes.end()) {
        return;
    }

    if ( foundView->second.empty() ) {
        return;
    }

    ///if the index is not in the range, just return the last
    if ( index >= (int)_imp->keyframes.size() ) {
        Keyframes::const_iterator it = foundView->second.end();
        --it;
        *returnValue = it->value;

        return;
    }

    int i = 0;
    for (Keyframes::const_iterator it = foundView->second.begin(); it != foundView->second.end(); ++it) {
        if (i == index) {
            *returnValue = it->value;

            return;
        }
        ++i;
    }
}


bool
StringAnimationManager::clone(const StringAnimationManager & other,
                              ViewIdx view,
                              ViewIdx otherView,
                              SequenceTime offset,
                              const RangeD* range)
{
    QMutexLocker l(&_imp->keyframesMutex);
    QMutexLocker l2(&other._imp->keyframesMutex);
    bool hasChanged = false;

    const PerViewKeyFrames::iterator foundOtherView = other._imp->keyframes.find(otherView);
    if (foundOtherView == other._imp->keyframes.end()) {
        return false;
    }

    Keyframes& keys = _imp->keyframes[view];

    if ( keys.size() != foundOtherView->second.size() ) {
        hasChanged = true;
    }
    if (!hasChanged) {
        Keyframes::const_iterator oit = foundOtherView->second.begin();
        for (Keyframes::const_iterator it = keys.begin(); it != keys.end(); ++it, ++oit) {
            if ( (it->time != oit->time) || (it->value != oit->value) ) {
                hasChanged = true;
                break;
            }
        }
    }
    if (hasChanged) {
        keys.clear();
        for (Keyframes::const_iterator it = foundOtherView->second.begin(); it != foundOtherView->second.end(); ++it) {
            double time = it->time;
            if ( range && ( (time < range->min) || (time > range->max) ) ) {
                // We ignore a keyframe, then consider the curve has changed
                hasChanged = true;
                continue;
            }
            StringKeyFrame k;
            k.time = time + offset;
            k.value = it->value;
            keys.insert(k);

        }

        keys = foundOtherView->second;
    }

    return hasChanged;
}


void
StringAnimationManager::load(const std::map<ViewIdx,std::map<double, std::string> > & keyframes)
{
    QMutexLocker l(&_imp->keyframesMutex);

    _imp->keyframes.clear();
    for (std::map<ViewIdx,std::map<double, std::string> >::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {

        Keyframes& keys = _imp->keyframes[it->first];
        for (std::map<double, std::string>::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            StringKeyFrame k;
            k.time = it2->first;
            k.value = it2->second;
            std::pair<Keyframes::iterator, bool> ret = keys.insert(k);
            assert(ret.second);
            Q_UNUSED(ret);
        }

    }
}

void
StringAnimationManager::save(std::map<ViewIdx,std::map<double, std::string> >* keyframes) const
{
    QMutexLocker l(&_imp->keyframesMutex);

    for (PerViewKeyFrames::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {

        std::map<double, std::string>& keys = (*keyframes)[it->first];

        for (Keyframes::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            std::pair<std::map<int, std::string>::iterator, bool> success = keys.insert( std::make_pair(it2->time, it2->value) );
            assert(success.second);
            Q_UNUSED(success);
        }

    }
}

NATRON_NAMESPACE_EXIT;
