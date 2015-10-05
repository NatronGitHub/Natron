/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <list>
#include <string>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/CacheEntry.h"


class Hash64;

namespace Natron {



class Image;
class FrameEntry
    : public CacheEntryHelper<U8,Natron::FrameKey,Natron::FrameParams>
{
public:
    FrameEntry(const FrameKey & key,
               const boost::shared_ptr<FrameParams> &  params,
               const Natron::CacheAPI* cache,
               Natron::StorageModeEnum storage,
               const std::string & path)
        : CacheEntryHelper<U8,FrameKey,FrameParams>(key,params,cache,storage,path)
        , _aborted(false)
        , _abortedMutex()
    {
    }

    ~FrameEntry()
    {
    }

    
    static boost::shared_ptr<FrameParams> makeParams(const RectI & rod,
                                                     int bitDepth,
                                                     int texW,
                                                     int texH,
                                                     const boost::shared_ptr<Natron::Image>& image) WARN_UNUSED_RETURN;
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

    void setAborted(bool aborted) {
        QMutexLocker k(&_abortedMutex);
        _aborted = aborted;
    }

    bool getAborted() const {
        QMutexLocker k(&_abortedMutex);
        return _aborted;
    }

    void getOriginalTiles(std::list<boost::shared_ptr<Natron::Image> >* ret) const
    {
        QReadLocker k(&_entryLock);
        _params->getOriginalTiles(ret);
    }

    void addOriginalTile(const boost::shared_ptr<Natron::Image>& image)
    {
        QWriteLocker k(&_entryLock);
        _params->addOriginalTile(image);
    }

private:

    ///The thread rendering the frame entry might have been aborted and the entry removed from the cache
    ///but another thread might successfully have found it in the cache. This flag is to notify it the frame
    ///is invalid.
    bool _aborted;
    mutable QMutex _abortedMutex;
};
}


#endif // NATRON_ENGINE_FRAMEENTRY_H
