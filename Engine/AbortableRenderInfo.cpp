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

#include <QAtomicInt>

#include "AbortableRenderInfo.h"

NATRON_NAMESPACE_ENTER;

struct AbortableRenderInfoPrivate
{
    bool canAbort;
    QAtomicInt aborted;
    U64 age;

    AbortableRenderInfoPrivate(bool canAbort,
                               U64 age)
    : canAbort(canAbort)
    , aborted()
    , age(age)
    {
        aborted.fetchAndStoreAcquire(0);
    }
};

AbortableRenderInfo::AbortableRenderInfo()
: _imp(new AbortableRenderInfoPrivate(true, 0))
{

}

AbortableRenderInfo::AbortableRenderInfo(bool canAbort,
                                         U64 age)
: _imp(new AbortableRenderInfoPrivate(canAbort, age))
{
}

AbortableRenderInfo::~AbortableRenderInfo()
{

}

U64
AbortableRenderInfo::getRenderAge() const
{
    return _imp->age;
}

bool
AbortableRenderInfo::canAbort() const
{
    return _imp->canAbort;
}

bool
AbortableRenderInfo::isAborted() const
{
    return (int)_imp->aborted > 0;
}

void
AbortableRenderInfo::setAborted()
{
    _imp->aborted = 1;
}

NATRON_NAMESPACE_EXIT;