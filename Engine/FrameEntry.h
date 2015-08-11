//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_FRAMEENTRY_H_
#define NATRON_ENGINE_FRAMEENTRY_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

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
        QMutexLocker k(&_entryLock);
        _params->getOriginalTiles(ret);
    }

    void addOriginalTile(const boost::shared_ptr<Natron::Image>& image)
    {
        QMutexLocker k(&_entryLock);
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


#endif // NATRON_ENGINE_FRAMEENTRY_H_
