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

#ifndef STRINGANIMATIONMANAGER_H
#define STRINGANIMATIONMANAGER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <map>
#include <memory>

#include "Global/GlobalDefines.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct StringAnimationManagerPrivate;

/**
 * @brief Animation of string parameters (AnimatingKnobStringHelper).
 *
 * A string keyframe is a keyframe of the knob's curve whose value is a reference to the string: equal strings
 * have the same keyframe value, different strings have different values. The reference does not depend on the
 * other keyframes of the curve, so adding, removing or moving a keyframe never changes the string of another
 * keyframe, and keyframes can be copied from a curve to another (undo/redo, copy/paste, clone, link) like the
 * keyframes of any other parameter type.
 *
 * Keyframe values are only meaningful within a process: they are never saved in projects, see
 * AnimatingKnobStringHelper::saveAnimation() and AnimatingKnobStringHelper::loadAnimation().
 *
 * This class also holds the custom interpolation function of OpenFX custom parameters.
 **/
class StringAnimationManager
{
public:
    StringAnimationManager(const KnobI* knob);

    ~StringAnimationManager();

    ///for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);

    bool hasCustomInterp() const;

    void setCustomInterpolation(customParamInterpolationV1Entry_t func, void* ofxParamHandle);

    /**
     * @brief Interpolate between the strings of two consecutive keyframes with the custom interpolation function.
     * Requires hasCustomInterp() and lowerTime < time < upperTime.
     **/
    void customInterpolation(double time,
                             double lowerTime,
                             const std::string& lowerValue,
                             double upperTime,
                             const std::string& upperValue,
                             std::string* ret) const;

    /**
     * @brief Returns the keyframe value referencing the string v. Thread-safe.
     **/
    static double stringToKeyFrameValue(const std::string & v);

    /**
     * @brief Returns in ret the string referenced by the keyframe value, or false if the value does not reference
     * a string. Thread-safe.
     **/
    static bool keyFrameValueToString(double value, std::string* ret);

private:

    std::unique_ptr<StringAnimationManagerPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // STRINGANIMATIONMANAGER_H
