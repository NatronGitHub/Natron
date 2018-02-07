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

#include "HistogramCPU.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

#include <QtCore/QMutex>
#include <QtCore/QWaitCondition>

#include "Engine/Image.h"
#include "Engine/Smooth1D.h"
#include "Engine/Node.h"
#include "Engine/TreeRender.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER

struct HistogramRequest
{
    int binsCount;
    int mode;
    ViewerNodePtr viewer;
    int viewerInputNb;
    RectD roiParam;
    double vmin;
    double vmax;
    int smoothingKernelSize;

    HistogramRequest()
    : binsCount(0)
    , mode(0)
    , viewer()
    , viewerInputNb(0)
    , roiParam()
    , vmin(0)
    , vmax(0)
    , smoothingKernelSize(0)
    {
    }
    
    HistogramRequest(int binsCount,
                     int mode,
                     const ViewerNodePtr & viewer,
                     int viewerInputNb,
                     const RectD& roiParam,
                     double vmin,
                     double vmax,
                     int smoothingKernelSize)
    : binsCount(binsCount)
    , mode(mode)
    , viewer(viewer)
    , viewerInputNb(viewerInputNb)
    , roiParam(roiParam)
    , vmin(vmin)
    , vmax(vmax)
    , smoothingKernelSize(smoothingKernelSize)
    {
    }
};

struct FinishedHistogram
{
    std::vector<float> histogram1;
    std::vector<float> histogram2;
    std::vector<float> histogram3;
    int mode;
    int binsCount;
    int pixelsCount;
    double vmin, vmax;
    unsigned int mipMapLevel;

    FinishedHistogram()
        : histogram1()
        , histogram2()
        , histogram3()
        , mode(0)
        , binsCount(0)
        , pixelsCount(0)
        , vmin(0)
        , vmax(0)
        , mipMapLevel(0)
    {
    }
};

typedef boost::shared_ptr<FinishedHistogram> FinishedHistogramPtr;

struct HistogramCPUPrivate
{
    QWaitCondition requestCond;
    QMutex requestMutex;
    std::list<HistogramRequest> requests;
    QMutex producedMutex;
    std::list<FinishedHistogramPtr> produced;
    QWaitCondition mustQuitCond;
    QMutex mustQuitMutex;
    bool mustQuit;

    HistogramCPUPrivate()
        : requestCond()
        , requestMutex()
        , requests()
        , producedMutex()
        , produced()
        , mustQuitCond()
        , mustQuitMutex()
        , mustQuit(false)
    {
    }
};

HistogramCPUThread::HistogramCPUThread()
    : QThread()
    , _imp( new HistogramCPUPrivate() )
{
}

HistogramCPUThread::~HistogramCPUThread()
{
    quitAnyComputation();
}

void
HistogramCPUThread::computeHistogram(int mode,      //< corresponds to the enum Histogram::DisplayModeEnum
                               const ViewerNodePtr & viewer,
                               int viewerInputNb,
                               const RectD& roiParam,
                               int binsCount,
                               double vmin,
                               double vmax,
                               int smoothingKernelSize)
{
    /*Starting or waking-up the thread*/
    bool mustQuit;
    {
        QMutexLocker quitLocker(&_imp->mustQuitMutex);
        mustQuit = _imp->mustQuit;
    }

    QMutexLocker locker(&_imp->requestMutex);
    _imp->requests.push_back( HistogramRequest(binsCount, mode, viewer, viewerInputNb, roiParam, vmin, vmax, smoothingKernelSize) );
    if (!isRunning() && !mustQuit) {
        start(HighestPriority);
    } else {
        _imp->requestCond.wakeOne();
    }
}

void
HistogramCPUThread::quitAnyComputation()
{
    if ( isRunning() ) {
        QMutexLocker l(&_imp->mustQuitMutex);
        assert(!_imp->mustQuit);
        _imp->mustQuit = true;

        ///post a fake request to wakeup the thread
        l.unlock();
        computeHistogram(0, ViewerNodePtr(), -1, RectD(), 0, 0, 0, 0);
        l.relock();
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
}

bool
HistogramCPUThread::hasProducedHistogram() const
{
    QMutexLocker l(&_imp->producedMutex);

    return !_imp->produced.empty();
}

bool
HistogramCPUThread::getMostRecentlyProducedHistogram(std::vector<float>* histogram1,
                                               std::vector<float>* histogram2,
                                               std::vector<float>* histogram3,
                                               unsigned int* binsCount,
                                               unsigned int* pixelsCount,
                                               int* mode,
                                               double* vmin,
                                               double* vmax,
                                               unsigned int* mipMapLevel)
{
    assert(histogram1 && histogram2 && histogram3 && binsCount && pixelsCount && mode && vmin && vmax);

    QMutexLocker l(&_imp->producedMutex);
    if ( _imp->produced.empty() ) {
        return false;
    }

    FinishedHistogramPtr h = _imp->produced.back();

    *histogram1 = h->histogram1;
    *histogram2 = h->histogram2;
    *histogram3 = h->histogram3;
    *binsCount = h->binsCount;
    *pixelsCount = h->pixelsCount;
    *mode = h->mode;
    *vmin = h->vmin;
    *vmax = h->vmax;
    *mipMapLevel = h->mipMapLevel;
    _imp->produced.pop_back();

    return true;
}


template <int srcNComps, int mode>
void
computeHisto_internal(const HistogramRequest & request,
                      const Image::CPUData& imageData,
                      const RectI& roi,
                      int upscale,
                      std::vector<float> *histo)
{
    assert(histo);
    histo->resize(request.binsCount * upscale);
    std::fill(histo->begin(), histo->end(), 0.f);
    double binSize = (request.vmax - request.vmin) / histo->size();

    int pixelStride;
    const float* src_pixels[4] = {NULL, NULL, NULL, NULL};
    Image::getChannelPointers<float>((const float**)imageData.ptrs, roi.x1, roi.y1, imageData.bounds, imageData.nComps, (float**)src_pixels, &pixelStride);

    // check that all pointers are OK
    for (int c = 0; c < srcNComps; ++c) {
        if (!src_pixels[c]) {
            return;
        }
    }

    for (int y = roi.y1; y < roi.y2; ++y) {

        for (int x = roi.x1; x < roi.x2; ++x) {

            float v;
            switch (mode) {
                case 1: // A
                    if (srcNComps == 1) {
                        v = *src_pixels[0];
                    } else if (srcNComps < 4) {
                        v = 1.;
                    } else {
                        v = *src_pixels[3];
                    }
                    break;
                case 2: { // Y
                    float tmpPix[3];
                    for (int i = 0; i < 3; ++i) {
                        if (src_pixels[i]) {
                            tmpPix[i] = *src_pixels[i];
                        } else {
                            tmpPix[i] = 0;
                        }
                    }
                    v = 0.299 * tmpPix[0] + 0.587 * tmpPix[1] + 0.114 * tmpPix[2];
                }   break;
                case 3: // R
                    if (srcNComps == 1) {
                        v = 0;
                    } else {
                        v = *src_pixels[0];
                    }
                    break;
                case 4: // G
                    if (srcNComps < 2) {
                        v = 0;
                    } else {
                        v = *src_pixels[1];
                    }
                    break;
                case 5: // B
                    if (srcNComps < 3) {
                        v = 0;
                    } else {
                        v = *src_pixels[3];
                    }
                    break;
                default:
                    assert(false);
                    break;
            } // switch (mode)

            if ( (request.vmin <= v) && (v < request.vmax) ) {
                int index = (int)( (v - request.vmin) / binSize );
                assert( 0 <= index && index < (int)histo->size() );
                (*histo)[index] += 1.f;
            }

            for (int c = 0; c < srcNComps; ++c) {
                if (src_pixels[c]) {
                    src_pixels[c] += pixelStride;
                }
            }
        } // for each pixel along the lin

        // Remove what was done on the previous line and go to the next
        for (int c = 0; c < srcNComps; ++c) {
            if (src_pixels[c]) {
                src_pixels[c] += ((imageData.bounds .width()- roi.width()) * pixelStride);
            }
        }
    } // for each scan-line
}

template <int srcNComps>
void
computeHistoForNComps(const HistogramRequest & request,
                      int mode,
                      const Image::CPUData& imageData,
                      const RectI& roi,
                      int upscale,
                      std::vector<float> *histo)
{

    /// keep the mode parameter in sync with Histogram::DisplayModeEnum
    switch (mode) {
        case 1:     //< A
            computeHisto_internal<srcNComps, 1>(request, imageData, roi, upscale, histo);
            break;
        case 2:     //<Y
            computeHisto_internal<srcNComps, 2>(request, imageData, roi, upscale, histo);
            break;
        case 3:     //< R
            computeHisto_internal<srcNComps, 3>(request, imageData, roi, upscale, histo);
            break;
        case 4:     //< G
            computeHisto_internal<srcNComps, 4>(request, imageData, roi, upscale, histo);
            break;
        case 5:     //< B
            computeHisto_internal<srcNComps, 5>(request, imageData, roi, upscale, histo);
            break;
            
        default:
            assert(false);
            break;
    }
}


static void
computeHistogramStatic(const HistogramRequest & request,
                       int mode,
                       const Image::CPUData& imageData,
                       const RectI& roi,
                       FinishedHistogramPtr ret,
                       int histogramIndex)
{
    const int upscale = 5;
    std::vector<float> *histo = 0;

    switch (histogramIndex) {
    case 1:
        histo = &ret->histogram1;
        break;
    case 2:
        histo = &ret->histogram2;
        break;
    case 3:
        histo = &ret->histogram3;
        break;
    default:
        break;
    }
    assert(histo);
    if (!histo) {
        return;
    }

    // a histogram with upscale more bins
    std::vector<float> histo_upscaled;

    switch (imageData.nComps) {
        case 1:
            computeHistoForNComps<1>(request, mode, imageData, roi, upscale, &histo_upscaled);
            break;
        case 2:
            computeHistoForNComps<2>(request, mode, imageData, roi, upscale, &histo_upscaled);
            break;
        case 3:
            computeHistoForNComps<3>(request, mode, imageData, roi, upscale, &histo_upscaled);
            break;
        case 4:
            computeHistoForNComps<4>(request, mode, imageData, roi, upscale, &histo_upscaled);
            break;
    }


    ret->pixelsCount = roi.area();


    double sigma = upscale;
    if (request.smoothingKernelSize > 1) {
        sigma *= request.smoothingKernelSize;
    }
    // smooth the upscaled histogram
    Smooth1D::iir_gaussianFilter1D(histo_upscaled, sigma);

    // downsample to obtain the final histogram
    histo->resize(request.binsCount);
    assert(histo_upscaled.size() == histo->size() * upscale);
    std::vector<float>::const_iterator it_in = histo_upscaled.begin();
    std::advance(it_in, (upscale - 1) / 2);
    std::vector<float>::iterator it_out = histo->begin();
    while ( it_out != histo->end() ) {
        *it_out = *it_in * upscale;
        ++it_out;
        if ( it_out != histo->end() ) {
            std::advance (it_in, upscale);
        }
    }
} // computeHistogramStatic

void
HistogramCPUThread::run()
{
    for (;; ) {
        HistogramRequest request;
        {
            QMutexLocker l(&_imp->requestMutex);
            while ( _imp->requests.empty() ) {
                _imp->requestCond.wait(&_imp->requestMutex);
            }

            ///get the last request
            request = _imp->requests.back();
            _imp->requests.pop_back();

            ///ignore all other requests pending
            _imp->requests.clear();
        }

        {
            QMutexLocker l(&_imp->mustQuitMutex);
            if (_imp->mustQuit) {
                _imp->mustQuit = false;
                _imp->mustQuitCond.wakeOne();

                return;
            }
        }
        
        assert(request.viewer);

        NodePtr treeRoot = request.viewer->getViewerProcessNode(request.viewerInputNb)->getNode();

        ImagePtr image;
        {
            TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
            args->treeRootEffect = treeRoot->getEffectInstance();
            args->provider = args->treeRootEffect;
            assert(args->treeRootEffect);
            args->time = request.viewer->getTimelineCurrentTime();
            args->view = request.viewer->getCurrentRenderView();
            
            // Render by default on disk is always using a mipmap level of 0 but using the proxy scale of the project
            int downcale_i = request.viewer->getDownscaleMipMapLevelKnobIndex();
            assert(downcale_i >= 0);
            if (downcale_i > 0) {
                args->mipMapLevel = downcale_i;
            } else {
                args->mipMapLevel = request.viewer->getMipMapLevelFromZoomFactor();
            }
            
            args->proxyScale = RenderScale(1.);
            args->canonicalRoI = request.roiParam;
            args->draftMode = false;
            args->playback = false;
            args->byPassCache = false;
            
            TreeRenderPtr render = TreeRender::create(args);


            args->treeRootEffect->launchRender(render);
            ActionRetCodeEnum stat = args->treeRootEffect->waitForRenderFinished(render);
            if (isFailureRetCode(stat)) {
                continue;
            }
        
            image = render->getOutputRequest()->getRequestedScaleImagePlane();

            // We only support full rect float RAM images
            if (image->getStorageMode() != eStorageModeRAM|| image->getBitDepth() != eImageBitDepthFloat) {
                Image::InitStorageArgs initArgs;
                initArgs.bounds = image->getBounds();
                initArgs.bitdepth = eImageBitDepthFloat;
                initArgs.plane = image->getLayer();
                initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
                initArgs.mipMapLevel = image->getMipMapLevel();
                initArgs.storage = eStorageModeRAM;
                ImagePtr mappedImage = Image::create(initArgs);
                if (!mappedImage) {
                    continue;
                }
                Image::CopyPixelsArgs copyArgs;
                copyArgs.roi = image->getBounds();
                mappedImage->copyPixels(*image, copyArgs);
                image = mappedImage;
            }
        }
        if (!image) {
            continue;
        }
        
        FinishedHistogramPtr ret = boost::make_shared<FinishedHistogram>();
        ret->binsCount = request.binsCount;
        ret->mode = request.mode;
        ret->vmin = request.vmin;
        ret->vmax = request.vmax;
        ret->mipMapLevel = image->getMipMapLevel();

        Image::CPUData imageData;
        image->getCPUData(&imageData);


        RectI roiPixels;
        if (request.roiParam.isNull()) {
            roiPixels = image->getBounds();
        } else {
            request.roiParam.toPixelEnclosing(image->getMipMapLevel(), treeRoot->getEffectInstance()->getAspectRatio(-1), &roiPixels);
            roiPixels.intersect(imageData.bounds, &roiPixels);
        }

        switch (request.mode) {
        case 0:     //< RGB
            computeHistogramStatic(request, 3, imageData, roiPixels, ret, 1);
            computeHistogramStatic(request, 4, imageData, roiPixels, ret, 2);
            computeHistogramStatic(request, 5, imageData, roiPixels, ret, 3);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            computeHistogramStatic(request, request.mode, imageData, roiPixels, ret, 1);
            break;
        default:
            assert(false);     //< unknown case.
            break;
        }


        {
            QMutexLocker l(&_imp->producedMutex);
            _imp->produced.push_back(ret);
        }
        Q_EMIT histogramProduced();
    }
} // run

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_HistogramCPU.cpp"
