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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct StringAnimationManagerPrivate;

///not thread-safe
class StringAnimationManager
{
public:
    StringAnimationManager();

    StringAnimationManager(const StringAnimationManager& other);

    ~StringAnimationManager();

    ///for integration of openfx custom params
    typedef OfxStatus (*customParamInterpolationV1Entry_t)(const void*            handleRaw,
                                                           OfxPropertySetHandle inArgsRaw,
                                                           OfxPropertySetHandle outArgsRaw);

    bool hasCustomInterp() const;

    void setCustomInterpolation(customParamInterpolationV1Entry_t func, void* ofxParamHandle, const std::string& knobName);

    bool customInterpolation(TimeValue time, std::string* ret) const;

    ValueChangedReturnCodeEnum insertKeyFrame(TimeValue time, const std::string & v, double* index);

    void removeKeyFrame(TimeValue time);

    void removeKeyframes(const std::list<double>& keysRemoved);

    void clearKeyFrames();

    void stringFromInterpolatedIndex(double interpolated, std::string* returnValue) const;

    bool clone(const StringAnimationManager & other, SequenceTime offset, const RangeD* range);

    void load(const std::map<double, std::string> & keyframes);

    void save(std::map<double, std::string>* keyframes) const;

private:

    boost::scoped_ptr<StringAnimationManagerPrivate> _imp;
};

NATRON_NAMESPACE_EXIT

#endif // STRINGANIMATIONMANAGER_H
