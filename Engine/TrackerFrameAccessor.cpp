/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
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
#include <libmv/autotrack/region.h>
#include <libmv/logging/logging.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

#include <QtCore/QDebug>

#include "Engine/AppInstance.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/EffectInstance.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/MultiThread.h"
#include "Engine/Image.h"
#include "Engine/TreeRender.h"
#include "Engine/Node.h"

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

struct FrameAccessorCacheKey
{
    int frame;
    TrackerFrameAccessor::GetImageTypeEnum type;
    int mipMapLevel;
    mv::FrameAccessor::InputMode mode;
};

struct CacheKey_compare_less
{
    bool operator() (const FrameAccessorCacheKey & lhs,
                     const FrameAccessorCacheKey & rhs) const
    {
        if (lhs.type < rhs.type) {
            return true;
        } else if (lhs.type > rhs.type) {
            return false;
        }

        if (lhs.frame < rhs.frame) {
            return true;
        } else if (lhs.frame > rhs.frame) {
            return false;
        }

        if (lhs.mipMapLevel < rhs.mipMapLevel) {
            return true;
        } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
            return false;
        }

        if ( (int)lhs.mode < (int)rhs.mode ) {
            return true;
        } else if ( (int)lhs.mode > (int)rhs.mode ) {
            return false;
        }
        return false;



    }
};

typedef boost::shared_ptr<MvFloatImage> MvFloatImagePtr;

struct FrameAccessorCacheEntry
{
    MvFloatImagePtr image;

    // If null, this is the full image
    RectI bounds;
    unsigned int referenceCount;
};


typedef std::multimap<FrameAccessorCacheKey, FrameAccessorCacheEntry, CacheKey_compare_less > FrameAccessorCache;


class ConvertToLibMVImageProcessorBase : public ImageMultiThreadProcessorBase
{
protected:

    Image::CPUData _source;
    MvFloatImage* _destination;
    RectI _destinationBounds;
    bool _enabledChannels[3];
    bool _takeDstFromAlpha;

public:

    ConvertToLibMVImageProcessorBase(const EffectInstancePtr& renderClone)
    : ImageMultiThreadProcessorBase(renderClone)
    {

    }

    virtual ~ConvertToLibMVImageProcessorBase()
    {
    }

    void setValues(const Image::CPUData& source,
                   MvFloatImage* destination,
                   const RectI& destinationBounds,
                   bool enabledChannels[3],
                   bool takeDstFromAlpha)
    {
        _source = source;
        _destination = destination;
        _destinationBounds = destinationBounds;
        for (int i = 0; i < 3; ++i) {
            _enabledChannels[i] = enabledChannels[i];
        }
        _takeDstFromAlpha = takeDstFromAlpha;
    }
};

template <int srcNComps, typename PIX, int maxValue>
class ConvertToLibMVImageProcessor : public ConvertToLibMVImageProcessorBase
{

public:

    ConvertToLibMVImageProcessor(const EffectInstancePtr& renderClone)
    : ConvertToLibMVImageProcessorBase(renderClone)
    {

    }

    virtual ~ConvertToLibMVImageProcessor()
    {
    }



private:

    virtual ActionRetCodeEnum multiThreadProcessImages(const RectI& renderWindow) OVERRIDE FINAL
    {
        if (_enabledChannels[0]) {
            if (_enabledChannels[1]) {
                if (_enabledChannels[2]) {
                    return processInternal<true, true, true>(renderWindow);
                } else {
                    return processInternal<true, true, false>(renderWindow);
                }
            } else {
                if (_enabledChannels[2]) {
                    return processInternal<true, false, true>(renderWindow);
                } else {
                    return processInternal<true, false, false>(renderWindow);
                }
            }
        } else {
            if (_enabledChannels[1]) {
                if (_enabledChannels[2]) {
                    return processInternal<false, true, true>(renderWindow);
                } else {
                    return processInternal<false, true, false>(renderWindow);
                }
            } else {
                if (_enabledChannels[2]) {
                    return processInternal<false, false, true>(renderWindow);
                } else {
                    return processInternal<false, false, false>(renderWindow);
                }
            }
        }

    }

    template <bool doR, bool doG, bool doB>
    ActionRetCodeEnum processInternal(const RectI& roi)
    {

        // LibMV images have their origin in the top left hand corner

        float* dst_pixels = (float*)Image::pixelAtStatic(roi.x1, roi.y1, _destinationBounds, 1, sizeof(float), (unsigned char*)_destination->Data());
        assert(dst_pixels);


        // It's important to rescale the result appropriately so that e.g. if only
        // blue is selected, it's not zeroed out.
        const float scale = (doR ? 0.2126f : 0.0f) + (doG ? 0.7152f : 0.0f) + (doB ? 0.0722f : 0.0f);

        for ( int y = roi.y1; y < roi.y2; ++y) {

            for (int x = roi.x1; x < roi.x2; ++x) {

                int srcPixelsStride;
                const PIX* src_pixels[4] = {NULL, NULL, NULL, NULL};
                Image::getChannelPointers<PIX, srcNComps>((const PIX**)_source.ptrs, x, y, _source.bounds, (PIX**)src_pixels, &srcPixelsStride);

                if (!src_pixels[0]) {
                    *dst_pixels = 0;
                } else {
                    if (srcNComps == 1) {
                        *dst_pixels = Image::convertPixelDepth<PIX, float>(*src_pixels[0]);
                    } else if (srcNComps == 4 && _takeDstFromAlpha) {
                        *dst_pixels = Image::convertPixelDepth<PIX, float>(*src_pixels[3]);
                    } else {
                        float tmpPix[4] = {0.f, 0.f, 0.f, 0.f};
                        for (int c = 0; c < srcNComps; ++c) {
                            tmpPix[c] = doR ? Image::convertPixelDepth<PIX, float>(*src_pixels[c]) : 0.f;
                        }
                        /// Apply luminance conversion while we copy the image
                        /// This code is taken from DisableChannelsTransform::run in libmv/autotrack/autotrack.cc
                        *dst_pixels = (0.2126f * tmpPix[0] + 0.7152f * tmpPix[1] + 0.0722f * tmpPix[2]) / scale;
                    }
                }

                ++dst_pixels;
                
            } // for each pixels
            
        } // for each scanline
        return eActionStatusOK;
    }
};

ActionRetCodeEnum
natronImageToLibMvFloatImageForDepth(bool enabledChannels[3],
                                     const Image& sourceImage,
                                     const RectI& roi,
                                     MvFloatImage& mvImg,
                                     bool takeDstFromAlpha,
                                     const boost::shared_ptr<ConvertToLibMVImageProcessorBase>& proc)
{
    Image::CPUData data;
    sourceImage.getCPUData(&data);

    proc->setValues(data, &mvImg, roi, enabledChannels, takeDstFromAlpha);
    proc->setRenderWindow(roi);

    return proc->launchThreadsBlocking();

} // natronImageToLibMvFloatImageForDepth

template <int srcNComps>
ActionRetCodeEnum
natronImageToLibMvFloatImageForNComps(bool enabledChannels[3],
                                      const Image& source,
                                      const RectI& roi,
                                      bool takeDstFromAlpha,
                                      MvFloatImage& mvImg)
{
    EffectInstancePtr renderClone;
    boost::shared_ptr<ConvertToLibMVImageProcessorBase> proc;
    switch (source.getBitDepth()) {
        case eImageBitDepthByte:
            proc.reset(new ConvertToLibMVImageProcessor<srcNComps, unsigned char, 255>(renderClone));
            break;
        case eImageBitDepthShort:
            proc.reset(new ConvertToLibMVImageProcessor<srcNComps, unsigned short, 65535>(renderClone));
            break;
        case eImageBitDepthFloat:
            proc.reset(new ConvertToLibMVImageProcessor<srcNComps, float, 1>(renderClone));
            break;
        default:
            return eActionStatusFailed;
    }

    return natronImageToLibMvFloatImageForDepth(enabledChannels, source, roi, mvImg, takeDstFromAlpha, proc);

}


NATRON_NAMESPACE_ANONYMOUS_EXIT


ActionRetCodeEnum
TrackerFrameAccessor::natronImageToLibMvFloatImage(bool enabledChannels[3],
                                                   const Image& source,
                                                   const RectI& roi,
                                                   bool takeDstFromAlpha,
                                                   MvFloatImage& mvImg)
{
    assert(source.getStorageMode() == eStorageModeRAM);
    switch (source.getComponentsCount()) {
        case 1:
            return natronImageToLibMvFloatImageForNComps<1>(enabledChannels, source, roi, takeDstFromAlpha, mvImg);
        case 2:
            return natronImageToLibMvFloatImageForNComps<2>(enabledChannels, source, roi, takeDstFromAlpha, mvImg);
        case 3:
            return natronImageToLibMvFloatImageForNComps<3>(enabledChannels, source, roi, takeDstFromAlpha, mvImg);
        case 4:
            return natronImageToLibMvFloatImageForNComps<4>(enabledChannels, source, roi, takeDstFromAlpha, mvImg);
        default:
            return eActionStatusFailed;
    }
}




struct TrackerFrameAccessorPrivate
{
    NodePtr sourceImageProvider, maskImageProvider;
    ImagePlaneDesc maskImagePlane;
    int maskPlaneIndex;
    mutable QMutex cacheMutex;
    FrameAccessorCache cache;
    bool enabledChannels[3];
    int formatHeight;

    TrackerFrameAccessorPrivate(const NodePtr& sourceImageProvider,
                                const NodePtr& maskImageProvider,
                                const ImagePlaneDesc& maskImagePlane,
                                int maskPlaneIndex,
                                bool enabledChannels[3],
                                int formatHeight)
        : sourceImageProvider(sourceImageProvider)
        , maskImageProvider(maskImageProvider)
        , maskImagePlane(maskImagePlane)
        , maskPlaneIndex(maskPlaneIndex)
        , cacheMutex()
        , cache()
        , enabledChannels()
        , formatHeight(formatHeight)
    {
        memcpy(this->enabledChannels, enabledChannels, sizeof(bool) * 3);
    }
};

TrackerFrameAccessor::TrackerFrameAccessor(const NodePtr& sourceImageProvider,
                                           const NodePtr& maskImageProvider,
                                           const ImagePlaneDesc& maskImagePlane,
                                           int maskPlaneIndex,
                                           bool enabledChannels[3],
                                           int formatHeight)
    : mv::FrameAccessor()
    , _imp( new TrackerFrameAccessorPrivate(sourceImageProvider, maskImageProvider, maskImagePlane, maskPlaneIndex, enabledChannels, formatHeight) )
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TrackerFrameAccessor::MakeSharedEnabler: public TrackerFrameAccessor
{
    MakeSharedEnabler(const NodePtr& sourceImageProvider,
                      const NodePtr& maskImageProvider,
                      const ImagePlaneDesc& maskImagePlane,
                      int maskPlaneIndex,
                      bool enabledChannels[3],
                      int formatHeight) : TrackerFrameAccessor(sourceImageProvider, maskImageProvider, maskImagePlane, maskPlaneIndex, enabledChannels, formatHeight) {
    }
};


TrackerFrameAccessorPtr
TrackerFrameAccessor::create(const NodePtr& sourceImageProvider,
                                      const NodePtr& maskImageProvider,
                                      const ImagePlaneDesc& maskImagePlane,
                                      int maskPlaneIndex,
                                      bool enabledChannels[3],
                                      int formatHeight)
{
    return boost::make_shared<TrackerFrameAccessor::MakeSharedEnabler>(sourceImageProvider, maskImageProvider, maskImagePlane, maskPlaneIndex, enabledChannels, formatHeight);
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

struct ArgsWithRender
{
    mv::FrameAccessor::GetImageArgs* args;
    RectI roi;
    FrameAccessorCacheKey key;
    TreeRenderPtr render;
};

void
TrackerFrameAccessor::GetImageInternal(std::list<mv::FrameAccessor::GetImageArgs>& imageRequests)
{

    std::vector<ArgsWithRender> allRenders(imageRequests.size());

    {
        std::size_t i = 0;
        for (std::list<mv::FrameAccessor::GetImageArgs>::iterator it = imageRequests.begin(); it != imageRequests.end(); ++it, ++i) {
            mv::FrameAccessor::GetImageArgs& args = *it;

            allRenders[i].args = &args;


            args.destination = 0;
            args.destinationKey = 0;

            // Since libmv only uses MONO images for now we have only optimized for this case, remove and handle properly
            // other case(s) when they get integrated into libmv.
            assert(args.input_mode == mv::FrameAccessor::MONO);


            allRenders[i].key.frame = args.frame;
            allRenders[i].key.mipMapLevel = args.downscale;
            allRenders[i].key.mode = mv::FrameAccessor::MONO;
            allRenders[i].key.type = args.sourceType;

            /*
             Check if a frame exists in the cache with matching key and bounds enclosing the given region
             */

            if (args.region) {
                convertLibMVRegionToRectI(*args.region, _imp->formatHeight, &allRenders[i].roi);

                QMutexLocker k(&_imp->cacheMutex);
                std::pair<FrameAccessorCache::iterator, FrameAccessorCache::iterator> range = _imp->cache.equal_range(allRenders[i].key);
                for (FrameAccessorCache::iterator it = range.first; it != range.second; ++it) {
                    if ( (allRenders[i].roi.x1 >= it->second.bounds.x1) && (allRenders[i].roi.x2 <= it->second.bounds.x2) &&
                        ( allRenders[i].roi.y1 >= it->second.bounds.y1) && ( allRenders[i].roi.y2 <= it->second.bounds.y2) ) {
#ifdef TRACE_LIB_MV
                        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Found cached image at frame" << frame << "with RoI x1="
                        << region->min(0) << "y1=" << region->max(1) << "x2=" << region->max(0) << "y2=" << region->min(1);
#endif
                        // LibMV is kinda dumb on this we must necessarily copy the data either via CopyFrom or the
                        // assignment constructor:
                        // EDIT: fixed libmv
                        args.destination = it->second.image.get();
                        //destination->CopyFrom<float>(*it->second.image);
                        ++it->second.referenceCount;

                        args.destinationKey = (mv::FrameAccessor::Key)it->second.image.get();
                        break;
                    }
                }
            }

            NodePtr sourceNode;
            switch (args.sourceType) {
                case eGetImageTypeSource:
                    sourceNode = _imp->sourceImageProvider;
                    break;
                case eGetImageTypeMask:
                    sourceNode = _imp->maskImageProvider;
                    break;
            }

            if (!sourceNode) {
                continue;
            }

            // Not in accessor cache, call renderRoI
            RenderScale scale;
            scale.y = scale.x = Image::getScaleFromMipMapLevel( (unsigned int)args.downscale );


            // Convert roi to canonical coordinates
            RectD roiCanonical;
            allRenders[i].roi.toCanonical_noClipping(0, 1., &roiCanonical);


            TreeRender::CtorArgsPtr renderArgs(new TreeRender::CtorArgs);
            {
                renderArgs->treeRootEffect = sourceNode->getEffectInstance();
                renderArgs->provider = renderArgs->treeRootEffect;
                renderArgs->time = TimeValue(args.frame);
                if (args.sourceType == eGetImageTypeMask) {
                    renderArgs->plane = _imp->maskImagePlane;
                }
                renderArgs->mipMapLevel = args.downscale;            
                renderArgs->canonicalRoI = roiCanonical;
            }
            
            TreeRenderPtr render = TreeRender::create(renderArgs);
            
            renderArgs->treeRootEffect->launchRender(render);
            allRenders[i].render = render;

        } // for each request
    } // i

    // Wait for all renders
    for (std::size_t i = 0; i < allRenders.size(); ++i) {

        ArgsWithRender& args = allRenders[i];
        if (!args.render) {
            // no render: nothing to do
            continue;
        }

        const RectI& roi = args.roi;

        ActionRetCodeEnum stat = args.render->getOriginalTreeRoot()->waitForRenderFinished(args.render);
        
        if (isFailureRetCode(stat)) {
            continue;
        } else {

#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Failed to call renderRoI on input at frame" << args.args->frame << "with RoI x1="
            << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2;
#endif
        }

        FrameViewRequestPtr outputRequest = args.render->getOutputRequest();

        ImagePtr sourceImage = outputRequest->getRequestedScaleImagePlane();
        const RectI& sourceBounds = sourceImage->getBounds();
        RectI intersectedRoI;
        if ( !roi.intersect(sourceBounds, &intersectedRoI) ) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "RoI does not intersect the source image bounds (RoI x1="
            << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif

            continue;
        }

#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "renderRoi (frame" << frame << ") OK  (BOUNDS= x1="
        << sourceBounds.x1 << "y1=" << sourceBounds.y1 << "x2=" << sourceBounds.x2 << "y2=" << sourceBounds.y2 << ") (ROI = " << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif

        // Make sure the Natron image rendered is RGBA full rect and on CPU, we don't support other formats to convert to libmv
        if (sourceImage->getStorageMode() != eStorageModeRAM) {
            Image::InitStorageArgs initArgs;
            initArgs.bounds = intersectedRoI;
            initArgs.plane = sourceImage->getLayer();
            initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
            initArgs.storage = eStorageModeRAM;
            initArgs.bitdepth = sourceImage->getBitDepth();
            ImagePtr tmpImage = Image::create(initArgs);
            if (!tmpImage) {
                continue;
            }
            Image::CopyPixelsArgs cpyArgs;
            cpyArgs.roi = intersectedRoI;
            tmpImage->copyPixels(*sourceImage, cpyArgs);
            sourceImage = tmpImage;
            
        }



        FrameAccessorCacheEntry entry;
        entry.image.reset( new MvFloatImage( roi.height(), roi.width() ) );
        entry.bounds = roi;
        entry.referenceCount = 1;

        stat = natronImageToLibMvFloatImage(_imp->enabledChannels,
                                     *sourceImage,
                                     roi,
                                     args.args->sourceType == eGetImageTypeMask/*takeDstFromAlpha*/,
                                     *entry.image);

        if (isFailureRetCode(stat)) {
            continue;
        }
        // we ignore the transform parameter and do it in natronImageToLibMvFloatImage instead

        args.args->destination = entry.image.get();
        args.args->destinationKey = (mv::FrameAccessor::Key)entry.image.get();
        //destination->CopyFrom<float>(*entry.image);

        //insert into the cache
        {
            QMutexLocker k(&_imp->cacheMutex);
            _imp->cache.insert( std::make_pair(args.key, entry) );
        }
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Rendered frame" << frame << "with RoI x1="
        << intersectedRoI.x1 << "y1=" << intersectedRoI.y1 << "x2=" << intersectedRoI.x2 << "y2=" << intersectedRoI.y2;
#endif

    } // for each render

} // GetImageInternal

/*
 * @brief This is called by LibMV to retrieve an image either for reference or as search frame.
 */
void
TrackerFrameAccessor::GetImage(std::list<mv::FrameAccessor::GetImageArgs>& imageRequests)
{
    GetImageInternal(imageRequests);
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
