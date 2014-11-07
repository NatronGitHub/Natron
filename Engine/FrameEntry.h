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
#include <string>
#include <boost/shared_ptr.hpp>

#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"
#include "Engine/FrameKey.h"
#include "Engine/FrameParams.h"
#include "Engine/CacheEntry.h"


class Hash64;

namespace Natron {




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

    static FrameKey makeKey(SequenceTime time,
                            U64 treeVersion,
                            double gain,
                            int lut,
                            int bitDepth,
                            int channels,
                            int view,
                            const TextureRect & textureRect,
                            const RenderScale & scale,
                            const std::string & inputName) WARN_UNUSED_RETURN;
    static boost::shared_ptr<FrameParams> makeParams(const RectI & rod,
                                                     int bitDepth,
                                                     int texW,
                                                     int texH) WARN_UNUSED_RETURN;
    const U8* data() const WARN_UNUSED_RETURN
    {
        return _data.readable();
    }

    U8* data() WARN_UNUSED_RETURN
    {
        return _data.writable();
    }

    void setAborted(bool aborted) {
        QMutexLocker k(&_abortedMutex);
        _aborted = aborted;
    }

    bool getAborted() const {
        QMutexLocker k(&_abortedMutex);
        return _aborted;
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
