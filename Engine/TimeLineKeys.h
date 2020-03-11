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

#ifndef TIMELINEKEYS_H
#define TIMELINEKEYS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct TimeLineKey
{
    int frame;
    bool isUserKey;

    explicit TimeLineKey(int frame,
                         bool isUserKey = false)
        : frame(frame)
        , isUserKey(isUserKey)
    {
    }

    explicit TimeLineKey()
        : frame(0)
        , isUserKey(false)
    {
    }
};

struct TimeLineKey_compare
{
    bool operator() (const TimeLineKey& lhs,
                     const TimeLineKey& rhs) const
    {
        return lhs.frame < rhs.frame;
    }
};

typedef std::set<TimeLineKey, TimeLineKey_compare> TimeLineKeysSet;

inline void
insertTimelineKey(const TimeLineKey& key, TimeLineKeysSet* keysSet)
{
    std::pair<TimeLineKeysSet::iterator, bool> ret = keysSet->insert(key);
    if (!ret.second && key.isUserKey) {
        // If this is a user key, erase the existing key and ensure we see the user key
        keysSet->erase(ret.first);
        keysSet->insert(key);
    }
}


NATRON_NAMESPACE_EXIT

#endif // TIMELINEKEYS_H
