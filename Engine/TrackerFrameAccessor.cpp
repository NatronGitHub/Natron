/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerFrameAccessor.h"

#include <boost/utility.hpp>

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#include <libmv/image/array_nd.h>
#include <libmv/autotrack/region.h>
#include <libmv/logging/logging.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

#include <QtCore/QDebug>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/TrackerContext.h"

NATRON_NAMESPACE_ENTER

namespace  {
struct FrameAccessorCacheKey
{
    int frame;
    int mipMapLevel;
    mv::FrameAccessor::InputMode mode;
};

struct CacheKey_compare_less
{
    bool operator() (const FrameAccessorCacheKey & lhs,
                     const FrameAccessorCacheKey & rhs) const
    {
        if (lhs.frame < rhs.frame) {
            return true;
        } else if (lhs.frame > rhs.frame) {
            return false;
        } else {
            if (lhs.mipMapLevel < rhs.mipMapLevel) {
                return true;
            } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
                return false;
            } else {
                if ( (int)lhs.mode < (int)rhs.mode ) {
                    return true;
                } else if ( (int)lhs.mode > (int)rhs.mode ) {
                    return false;
                } else {
                    return false;
                }
            }
        }
    }
};


class MvFloatImage
    : public libmv::Array3D<float>
{
public:

    MvFloatImage()
        : libmv::Array3D<float>()
    {
    }

    MvFloatImage(int height,
                 int width)
        : libmv::Array3D<float>(height, width)
    {
    }

    MvFloatImage(float* data,
                 int height,
                 int width)
        : libmv::Array3D<float>(data, height, width)
    {
    }

    virtual ~MvFloatImage()
    {
    }
};


struct FrameAccessorCacheEntry
{
    boost::shared_ptr<MvFloatImage> image;

    // If null, this is the full image
    RectI bounds;
    unsigned int referenceCount;
};

typedef std::multimap<FrameAccessorCacheKey, FrameAccessorCacheEntry, CacheKey_compare_less > FrameAccessorCache;


template <bool doR, bool doG, bool doB>
void
natronImageToLibMvFloatImageForChannels(const Image* source,
                                        const RectI& roi,
                                        MvFloatImage& mvImg)
{
    //mvImg is expected to have its bounds equal to roi

    Image::ReadAccess racc(source);
    unsigned int compsCount = source->getComponentsCount();

    assert(compsCount == 3);
    unsigned int srcRowElements = source->getRowElements();

    assert( source->getBounds().contains(roi) );
    const float* src_pixels = (const float*)racc.pixelAt(roi.x1, roi.y1);
    assert(src_pixels);
    float* dst_pixels = mvImg.Data();
    assert(dst_pixels);
    //LibMV images have their origin in the top left hand corner

    // It's important to rescale the resultappropriately so that e.g. if only
    // blue is selected, it's not zeroed out.
    float scale = (doR ? 0.2126f : 0.0f) +
                  (doG ? 0.7152f : 0.0f) +
                  (doB ? 0.0722f : 0.0f);
    int h = roi.height();
    int w = roi.width();
    for ( int y = 0; y < h; ++y,
          src_pixels += (srcRowElements - compsCount * w) ) {
        for (int x = 0; x < w; ++x,
             src_pixels += compsCount,
             ++dst_pixels) {
            /// Apply luminance conversion while we copy the image
            /// This code is taken from DisableChannelsTransform::run in libmv/autotrack/autotrack.cc
            *dst_pixels = ( 0.2126f * (doR ? src_pixels[0] : 0.0f) +
                            0.7152f * (doG ? src_pixels[1] : 0.0f) +
                            0.0722f * (doB ? src_pixels[2] : 0.0f) ) / scale;
        }
    }
}

static void
natronImageToLibMvFloatImage(bool enabledChannels[3],
                             const Image* source,
                             const RectI& roi,
                             MvFloatImage& mvImg)
{
    if (enabledChannels[0]) {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true, true, true>(source, roi, mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true, true, false>(source, roi, mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true, false, true>(source, roi, mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true, false, false>(source, roi, mvImg);
            }
        }
    } else {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false, true, true>(source, roi, mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false, true, false>(source, roi, mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false, false, true>(source, roi, mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false, false, false>(source, roi, mvImg);
            }
        }
    }
}
} // anon namespace


struct TrackerFrameAccessorPrivate
{
    const TrackerContext* context;
    boost::shared_ptr<Node> trackerInput;
    mutable QMutex cacheMutex;
    FrameAccessorCache cache;
    bool enabledChannels[3];
    int formatHeight;

    TrackerFrameAccessorPrivate(const TrackerContext* context,
                                bool enabledChannels[3],
                                int formatHeight)
        : context(context)
        , trackerInput()
        , cacheMutex()
        , cache()
        , enabledChannels()
        , formatHeight(formatHeight)
    {
        trackerInput = context->getNode()->getInput(0);
        assert(trackerInput);
        for (int i = 0; i < 3; ++i) {
            this->enabledChannels[i] = enabledChannels[i];
        }
    }
};

TrackerFrameAccessor::TrackerFrameAccessor(const TrackerContext* context,
                                           bool enabledChannels[3],
                                           int formatHeight)
    : mv::FrameAccessor()
    , _imp( new TrackerFrameAccessorPrivate(context, enabledChannels, formatHeight) )
{
}

TrackerFrameAccessor::~TrackerFrameAccessor()
{
}

void
TrackerFrameAccessor::getEnabledChannels(bool* r,
                                         bool* g,
                                         bool* b) const
{
    *r = _imp->enabledChannels[0];
    *g = _imp->enabledChannels[1];
    *b = _imp->enabledChannels[2];
}

double
TrackerFrameAccessor::invertYCoordinate(double yIn,
                                        double formatHeight)
{
    return formatHeight - 1 - yIn;
}

void
TrackerFrameAccessor::convertLibMVRegionToRectI(const mv::Region& region,
                                                int /*formatHeight*/,
                                                RectI* roi)
{
    roi->x1 = region.min(0);
    roi->x2 = region.max(0);
    roi->y1 = region.min(1);
    //roi->y1 = invertYCoordinate(region.max(1), formatHeight);
    roi->y2 = region.max(1);
    //roi->y2 = invertYCoordinate(region.min(1), formatHeight);
}

/*
 * @brief This is called by LibMV to retrieve an image either for reference or as search frame.
 */
mv::FrameAccessor::Key
TrackerFrameAccessor::GetImage(int /*clip*/,
                               int frame,
                               mv::FrameAccessor::InputMode input_mode,
                               int downscale,            // Downscale by 2^downscale.
                               const mv::Region* region,     // Get full image if NULL.
                               const mv::FrameAccessor::Transform* /*transform*/, // May be NULL.
                               mv::FloatImage** destination)
{
    // Since libmv only uses MONO images for now we have only optimized for this case, remove and handle properly
    // other case(s) when they get integrated into libmv.
    assert(input_mode == mv::FrameAccessor::MONO);


    FrameAccessorCacheKey key;
    key.frame = frame;
    key.mipMapLevel = downscale;
    key.mode = input_mode;

    /*
       Check if a frame exists in the cache with matching key and bounds enclosing the given region
     */
    RectI roi;
    if (region) {
        convertLibMVRegionToRectI(*region, _imp->formatHeight, &roi);

        QMutexLocker k(&_imp->cacheMutex);
        std::pair<FrameAccessorCache::iterator, FrameAccessorCache::iterator> range = _imp->cache.equal_range(key);
        for (FrameAccessorCache::iterator it = range.first; it != range.second; ++it) {
            if ( (roi.x1 >= it->second.bounds.x1) && (roi.x2 <= it->second.bounds.x2) &&
                 ( roi.y1 >= it->second.bounds.y1) && ( roi.y2 <= it->second.bounds.y2) ) {
#ifdef TRACE_LIB_MV
                qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Found cached image at frame" << frame << "with RoI x1="
                         << region->min(0) << "y1=" << region->max(1) << "x2=" << region->max(0) << "y2=" << region->min(1);
#endif
                // LibMV is kinda dumb on this we must necessarily copy the data either via CopyFrom or the
                // assignment constructor:
                // EDIT: fixed libmv
                *destination = it->second.image.get();
                //destination->CopyFrom<float>(*it->second.image);
                ++it->second.referenceCount;

                return (mv::FrameAccessor::Key)it->second.image.get();
            }
        }
    }

    EffectInstPtr effect;
    if (_imp->trackerInput) {
        effect = _imp->trackerInput->getEffectInstance();
    }
    if (!effect) {
        return (mv::FrameAccessor::Key)0;
    }

    // Not in accessor cache, call renderRoI
    RenderScale scale;
    scale.y = scale.x = Image::getScaleFromMipMapLevel( (unsigned int)downscale );


    RectD precomputedRoD;
    if (!region) {
        bool isProjectFormat;
        StatusEnum stat = effect->getRegionOfDefinition_public(_imp->trackerInput->getHashValue(), frame, scale, ViewIdx(0), &precomputedRoD, &isProjectFormat);
        if (stat == eStatusFailed) {
            return (mv::FrameAccessor::Key)0;
        }
        double par = effect->getAspectRatio(-1);
        precomputedRoD.toPixelEnclosing( (unsigned int)downscale, par, &roi );
    }

    std::list<ImagePlaneDesc> components;
    components.push_back( ImagePlaneDesc::getRGBComponents() );

    NodePtr node = _imp->context->getNode();
    const bool isRenderUserInteraction = true;
    const bool isSequentialRender = false;
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
    if (isAbortable) {
        isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, node->getEffectInstance() );
    }
    ParallelRenderArgsSetter frameRenderArgs( frame,
                                              ViewIdx(0), //<  view 0 (left)
                                              isRenderUserInteraction, //<isRenderUserInteraction
                                              isSequentialRender, //isSequential
                                              abortInfo, //abort info
                                              node, //  requester
                                              0, //texture index
                                              node->getApp()->getTimeLine().get(), //Timeline
                                              NodePtr(), // rotoPaintNode
                                              true, //isAnalysis
                                              false, //draftMode
                                              boost::shared_ptr<RenderStats>() ); // Stats
    EffectInstance::RenderRoIArgs args( frame,
                                        scale,
                                        downscale,
                                        ViewIdx(0),
                                        false,
                                        roi,
                                        precomputedRoD,
                                        components,
                                        eImageBitDepthFloat,
                                        true,
                                        _imp->context->getNode()->getEffectInstance().get(),
                                        eStorageModeRAM /*returnOpenGLTex*/,
                                        frame);
    std::map<ImagePlaneDesc, ImagePtr> planes;
    EffectInstance::RenderRoIRetCode stat = effect->renderRoI(args, &planes);
    if ( (stat != EffectInstance::eRenderRoIRetCodeOk) || planes.empty() ) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Failed to call renderRoI on input at frame" << frame << "with RoI x1="
                 << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2;
#endif

        return (mv::FrameAccessor::Key)0;
    }

    assert( !planes.empty() );
    const ImagePtr& sourceImage = planes.begin()->second;
    RectI sourceBounds = sourceImage->getBounds();
    RectI intersectedRoI;
    if ( !roi.intersect(sourceBounds, &intersectedRoI) ) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "RoI does not intersect the source image bounds (RoI x1="
                 << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif

        return (mv::FrameAccessor::Key)0;
    }

#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "renderRoi (frame" << frame << ") OK  (BOUNDS= x1="
             << sourceBounds.x1 << "y1=" << sourceBounds.y1 << "x2=" << sourceBounds.x2 << "y2=" << sourceBounds.y2 << ") (ROI = " << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif

    /*
       Copy the Natron image to the LivMV float image
     */
    FrameAccessorCacheEntry entry;
    entry.image = boost::make_shared<MvFloatImage>( intersectedRoI.height(), intersectedRoI.width() );
    entry.bounds = intersectedRoI;
    entry.referenceCount = 1;
    natronImageToLibMvFloatImage(_imp->enabledChannels,
                                 sourceImage.get(),
                                 intersectedRoI,
                                 *entry.image);
    // we ignore the transform parameter and do it in natronImageToLibMvFloatImage instead

    *destination = entry.image.get();
    //destination->CopyFrom<float>(*entry.image);

    //insert into the cache
    {
        QMutexLocker k(&_imp->cacheMutex);
        _imp->cache.insert( std::make_pair(key, entry) );
    }
#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Rendered frame" << frame << "with RoI x1="
             << intersectedRoI.x1 << "y1=" << intersectedRoI.y1 << "x2=" << intersectedRoI.x2 << "y2=" << intersectedRoI.y2;
#endif

    return (mv::FrameAccessor::Key)entry.image.get();
} // TrackerFrameAccessor::GetImage


void
TrackerFrameAccessor::ReleaseImage(Key key)
{
    MvFloatImage* imgKey = (MvFloatImage*)key;
    QMutexLocker k(&_imp->cacheMutex);

    for (FrameAccessorCache::iterator it = _imp->cache.begin(); it != _imp->cache.end(); ++it) {
        if (it->second.image.get() == imgKey) {
            --it->second.referenceCount;
            if (!it->second.referenceCount) {
                _imp->cache.erase(it);

                return;
            }
        }
    }
}

/*
 * @brief This is called by LibMV to retrieve an the mask, which is always defined in the reference frame.
 */
// Get mask image for the given track.
//
// Implementation of this method should sample mask associated with the track
// within given region to the given destination.
//
// Result is supposed to be a single channel image.
//
// If region is NULL, it it assumed to be full-frame.
mv::FrameAccessor::Key
TrackerFrameAccessor::GetMaskForTrack(int clip,
                                      int frame,
                                      int track,
                                      const mv::Region* region,
                                      mv::FloatImage* destination)
{
    Q_UNUSED(clip);
    Q_UNUSED(frame);
    Q_UNUSED(track);
    Q_UNUSED(region);
    Q_UNUSED(destination);

    // no mask yet

    // This feature was lost in libmv/blander when the autotrack API changed, and was
    // re-introduced by this commit:
    // https://developer.blender.org/rBb0015686e2e48a384a0b2a03a75f6daaad7271c0
    //

#pragma message WARN("TODO: implement TrackerFrameAccessor::GetMaskForTrack")

    return NULL;
}

// Release a specified mask.
//
// Non-caching implementation may free used memory immediately.
void
TrackerFrameAccessor::ReleaseMask(mv::FrameAccessor::Key key)
{
    Q_UNUSED(key);

    // no mask yet
}

// Not used in LibMV
bool
TrackerFrameAccessor::GetClipDimensions(int /*clip*/,
                                        int* /*width*/,
                                        int* /*height*/)
{
    return false;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
TrackerFrameAccessor::NumClips()
{
    return 1;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
TrackerFrameAccessor::NumFrames(int /*clip*/)
{
    return 0;
}

NATRON_NAMESPACE_EXIT
