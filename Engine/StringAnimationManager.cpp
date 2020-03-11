/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include <ofxParam.h>
#include <ofxhPropertySuite.h> // PropSpec

#include "Engine/Knob.h"

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

struct StringKeyFrame
{
    std::string value;
    TimeValue time;
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

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct StringAnimationManagerPrivate
{
    StringAnimationManager::customParamInterpolationV1Entry_t customInterpolation;
    void* ofxParamHandle;
    std::string knobName;
    
    mutable QMutex keyframesMutex;
    Keyframes keyframes;


    StringAnimationManagerPrivate()
        : customInterpolation(NULL)
        , ofxParamHandle(NULL)
        , knobName()
        , keyframesMutex()
        , keyframes()
    {
    }

    StringAnimationManagerPrivate(const StringAnimationManagerPrivate& other)
    : customInterpolation(other.customInterpolation)
    , ofxParamHandle(other.ofxParamHandle)
    , knobName(other.knobName)
    , keyframesMutex()
    , keyframes(other.keyframes)
    {

    }
};

StringAnimationManager::StringAnimationManager()
    : _imp( new StringAnimationManagerPrivate() )
{
}

StringAnimationManager::~StringAnimationManager()
{
}

StringAnimationManager::StringAnimationManager(const StringAnimationManager& other)
: _imp(new StringAnimationManagerPrivate(*other._imp))
{

}


bool
StringAnimationManager::hasCustomInterp() const
{
    return _imp->customInterpolation != NULL;
}

void
StringAnimationManager::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                               void* ofxParamHandle,
                                               const std::string& knobName)
{
    _imp->customInterpolation = func;
    _imp->ofxParamHandle = ofxParamHandle;
    _imp->knobName = knobName;
}

bool
StringAnimationManager::customInterpolation(TimeValue time,
                                            std::string* ret) const
{
    QMutexLocker l(&_imp->keyframesMutex);

    assert(_imp->customInterpolation);
    ///if there's a single keyframe, return it

    if ( _imp->keyframes.empty() ) {
        return false;
    }
    
    if (_imp->keyframes.size() == 1) {
        *ret = _imp->keyframes.begin()->value;
        return true;
    }

    /// get the keyframes surrounding the time
    Keyframes::const_iterator upper = _imp->keyframes.end();
    Keyframes::const_iterator lower = _imp->keyframes.end();
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (it->time > time) {
            upper = it;
            break;
        } else if (it->time == time) {
            ///if there's a keyframe exactly at this time, return its value
            *ret = it->value;

            return true;
        }
    }

    if ( upper == _imp->keyframes.end() ) {
        ///if the time is greater than the time of all keyframes return the last

        --upper;
        *ret = upper->value;

        return true;
    } else if ( upper == _imp->keyframes.begin() ) {
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
    inArgs.setStringProperty( kOfxPropName, _imp->knobName );
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

ValueChangedReturnCodeEnum
StringAnimationManager::insertKeyFrame(TimeValue time,
                                       const std::string & v,
                                       double* index)
{
    StringKeyFrame k;

    k.time = time;
    k.value = v;

    ValueChangedReturnCodeEnum retCode = eValueChangedReturnCodeNothingChanged;

    QMutexLocker l(&_imp->keyframesMutex);
    std::pair<Keyframes::iterator, bool> ret = _imp->keyframes.insert(k);
    if (ret.second) {
        retCode = eValueChangedReturnCodeKeyframeAdded;
    } else {
        if (ret.first->value != v) {
            retCode = eValueChangedReturnCodeKeyframeModified;
        }
        _imp->keyframes.erase(ret.first);
        ret = _imp->keyframes.insert(k);
        assert(ret.second);
    }
    *index = std::distance(_imp->keyframes.begin(), ret.first);
    return retCode;
}

void
StringAnimationManager::removeKeyFrame(TimeValue time)
{
    QMutexLocker l(&_imp->keyframesMutex);
 
    for (Keyframes::iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (it->time == time) {
            _imp->keyframes.erase(it);

            return;
        }
    }
}


void
StringAnimationManager::removeKeyframes(const std::list<double>& keysRemoved)
{
    for (std::list<double>::const_iterator it = keysRemoved.begin(); it != keysRemoved.end(); ++it) {
        removeKeyFrame(TimeValue(*it));
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
                                                    std::string* returnValue) const
{
    int index = std::floor(interpolated + 0.5);
    QMutexLocker l(&_imp->keyframesMutex);

 
    if ( _imp->keyframes.empty() ) {
        return;
    }

    ///if the index is not in the range, just return the last
    if ( index >= (int)_imp->keyframes.size() ) {
        Keyframes::const_iterator it = _imp->keyframes.end();
        --it;
        *returnValue = it->value;

        return;
    }

    int i = 0;
    for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it) {
        if (i == index) {
            *returnValue = it->value;

            return;
        }
        ++i;
    }
}


bool
StringAnimationManager::clone(const StringAnimationManager & other,
                              SequenceTime offset,
                              const RangeD* range)
{
    QMutexLocker l(&_imp->keyframesMutex);
    QMutexLocker l2(&other._imp->keyframesMutex);
    bool hasChanged = false;


    if ( _imp->keyframes.size() != other._imp->keyframes.size() ) {
        hasChanged = true;
    }
    if (!hasChanged) {
        Keyframes::const_iterator oit = other._imp->keyframes.begin();
        for (Keyframes::const_iterator it = _imp->keyframes.begin(); it != _imp->keyframes.end(); ++it, ++oit) {
            if ( (it->time != oit->time) || (it->value != oit->value) ) {
                hasChanged = true;
                break;
            }
        }
    }
    if (hasChanged) {
        _imp->keyframes.clear();
        for (Keyframes::const_iterator it = other._imp->keyframes.begin(); it != other._imp->keyframes.end(); ++it) {
            TimeValue time = it->time;
            if ( range && ( (time < range->min) || (time > range->max) ) ) {
                // We ignore a keyframe, then consider the curve has changed
                hasChanged = true;
                continue;
            }
            StringKeyFrame k;
            k.time = TimeValue(time + offset);
            k.value = it->value;
            _imp->keyframes.insert(k);

        }

    }

    return hasChanged;
}


void
StringAnimationManager::load(const std::map<double, std::string > & keyframes)
{
    QMutexLocker l(&_imp->keyframesMutex);

    _imp->keyframes.clear();
    for (std::map<double, std::string>::const_iterator it = keyframes.begin(); it != keyframes.end(); ++it) {
        
        StringKeyFrame k;
        k.time = TimeValue(it->first);
        k.value = it->second;
        std::pair<Keyframes::iterator, bool> ret = _imp->keyframes.insert(k);
        assert(ret.second);
        Q_UNUSED(ret);
        
        
    }
}

void
StringAnimationManager::save(std::map<double, std::string>* keyframes) const
{
    QMutexLocker l(&_imp->keyframesMutex);
    
    for (Keyframes::const_iterator it2 = _imp->keyframes.begin(); it2 != _imp->keyframes.end(); ++it2) {
        std::pair<std::map<double, std::string>::iterator, bool> success = keyframes->insert( std::make_pair(it2->time, it2->value) );
        assert(success.second);
        Q_UNUSED(success);
    }
}

NATRON_NAMESPACE_EXIT
