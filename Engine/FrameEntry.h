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

#ifndef NATRON_ENGINE_FRAMEENTRY_H
#define NATRON_ENGINE_FRAMEENTRY_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <string>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
#include <QtCore/QObject>
#include <QtCore/QMutex>
CLANG_DIAG_ON(deprecated)

#include "Global/GlobalDefines.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/CacheEntry.h"
#include "Engine/EngineFwd.h"


NATRON_NAMESPACE_ENTER

class FrameEntry
    : public CacheEntryHelper<U8, FrameKey, FrameParams>
{
public:
    FrameEntry(const FrameKey & key,
               const FrameParamsPtr &  params,
               const CacheAPI* cache)
        : CacheEntryHelper<U8, FrameKey, FrameParams>(key, params, cache)
    {
    }

    ~FrameEntry()
    {
    }

    const U8* data() const WARN_UNUSED_RETURN
    {
        return _data.readable();
    }

    U8* data() WARN_UNUSED_RETURN
    {
        return _data.writable();
    }

    const U8* pixelAt(int x, int y ) const WARN_UNUSED_RETURN;

    void copy(const FrameEntry& other);


    ImagePtr getInternalImage() const
    {
        QReadLocker k(&_entryLock);

        return _params->getInternalImage();
    }

    void setInternalImage(const ImagePtr& image)
    {
        QWriteLocker k(&_entryLock);

        _params->setInternalImage(image);
    }



};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_FRAMEENTRY_H
