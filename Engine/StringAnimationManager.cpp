/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#include "StringAnimationManager.h"

#include <map>
#include <vector>
#include <cmath>
#include <cassert>
#include <stdexcept>

#include <QReadWriteLock>

#include <ofxParam.h>
#include <ofxhPropertySuite.h> // PropSpec

#include "Engine/Knob.h"

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

// The strings referenced by string keyframes. It is shared by all knobs, so that a keyframe value has the same
// meaning in every curve: keyframes can be copied between knobs without converting their values.
// Strings are never removed, because a keyframe referencing a string may come back at any time (undo/redo,
// copy/paste). This is bounded by the number of distinct strings set on keyframes during the session.
class StringKeyFrameTable
{
public:
    static StringKeyFrameTable& instance()
    {
        static StringKeyFrameTable table; // thread-safe initialization (C++11)

        return table;
    }

    double valueFromString(const std::string & v)
    {
        {
            QReadLocker l(&_lock);
            std::map<std::string, int>::const_iterator found = _ids.find(v);
            if ( found != _ids.end() ) {
                return found->second;
            }
        }
        QWriteLocker l(&_lock);
        // another thread may have added it in-between
        std::pair<std::map<std::string, int>::iterator, bool> ret = _ids.insert( std::make_pair( v, (int)_strings.size() ) );
        if (ret.second) {
            _strings.push_back(v);
        }

        return ret.first->second;
    }

    bool stringFromValue(double value,
                         std::string* ret) const
    {
        // The value of a string keyframe is an integer
        double id = std::floor(value + 0.5);
        QReadLocker l(&_lock);

        if ( !(id >= 0) || ( id >= (double)_strings.size() ) ) { // also catches NaN
            return false;
        }
        *ret = _strings[(std::size_t)id];

        return true;
    }

private:
    StringKeyFrameTable()
        : _lock()
        , _ids()
        , _strings()
    {
    }

    mutable QReadWriteLock _lock;
    std::map<std::string, int> _ids;
    std::vector<std::string> _strings;
};

NATRON_NAMESPACE_ANONYMOUS_EXIT


struct StringAnimationManagerPrivate
{
    StringAnimationManager::customParamInterpolationV1Entry_t customInterpolation;
    void* ofxParamSetHandle;
    const KnobI* knob;

    StringAnimationManagerPrivate(const KnobI* knob)
        : customInterpolation(NULL)
        , ofxParamSetHandle(NULL)
        , knob(knob)
    {
    }
};

StringAnimationManager::StringAnimationManager(const KnobI* knob)
    : _imp( new StringAnimationManagerPrivate(knob) )
{
}

StringAnimationManager::~StringAnimationManager()
{
}

bool
StringAnimationManager::hasCustomInterp() const
{
    return _imp->customInterpolation != NULL;
}

void
StringAnimationManager::setCustomInterpolation(customParamInterpolationV1Entry_t func,
                                               void* ofxParamSetHandle)
{
    _imp->customInterpolation = func;
    _imp->ofxParamSetHandle = ofxParamSetHandle;
}

bool
StringAnimationManager::customInterpolation(double time,
                                            double lowerTime,
                                            const std::string& lowerValue,
                                            double upperTime,
                                            const std::string& upperValue,
                                            std::string* ret) const
{
    assert(_imp->customInterpolation);
    assert(lowerTime < time && time < upperTime);

    OFX::Host::Property::PropSpec inArgsSpec[] = {
        { kOfxPropName,    OFX::Host::Property::eString, 1, true, "" },
        { kOfxPropTime,    OFX::Host::Property::eDouble, 1, true, "" },
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 2, true, ""},
        { kOfxParamPropInterpolationTime,    OFX::Host::Property::eDouble, 2, true, "" },
        { kOfxParamPropInterpolationAmount,    OFX::Host::Property::eDouble, 1, true, "" },
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set inArgs(inArgsSpec);
    inArgs.setStringProperty( kOfxPropName, _imp->knob->getName() );
    inArgs.setDoubleProperty(kOfxPropTime, time);

    inArgs.setStringProperty(kOfxParamPropCustomValue, lowerValue, 0);
    inArgs.setStringProperty(kOfxParamPropCustomValue, upperValue, 1);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, lowerTime, 0);
    inArgs.setDoubleProperty(kOfxParamPropInterpolationTime, upperTime, 1);
    inArgs.setDoubleProperty( kOfxParamPropInterpolationAmount, (time - lowerTime) / (double)(upperTime - lowerTime) );


    OFX::Host::Property::PropSpec outArgsSpec[] = {
        { kOfxParamPropCustomValue,    OFX::Host::Property::eString, 1, false, ""},
        OFX::Host::Property::propSpecEnd
    };
    OFX::Host::Property::Set outArgs(outArgsSpec);

    OfxStatus stat = _imp->customInterpolation( _imp->ofxParamSetHandle, inArgs.getHandle(), outArgs.getHandle() );
    if (stat != kOfxStatOK) {
        // outArgs holds its default value, "", which is not a value of the parameter
        return false;
    }

    *ret = outArgs.getStringProperty(kOfxParamPropCustomValue, 0).c_str();

    return true;
} // customInterpolation

double
StringAnimationManager::stringToKeyFrameValue(const std::string & v)
{
    return StringKeyFrameTable::instance().valueFromString(v);
}

bool
StringAnimationManager::keyFrameValueToString(double value,
                                              std::string* ret)
{
    return StringKeyFrameTable::instance().stringFromValue(value, ret);
}

NATRON_NAMESPACE_EXIT
