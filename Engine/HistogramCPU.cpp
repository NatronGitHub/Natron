/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif
#include "Engine/Image.h"
#include "Engine/Smooth1D.h"

NATRON_NAMESPACE_ENTER

struct HistogramRequest
{
    int binsCount;
    int mode;
    ImagePtr image;
    RectI rect;
    double vmin;
    double vmax;
    int smoothingKernelSize;

    HistogramRequest()
        : binsCount(0)
        , mode(0)
        , image()
        , rect()
        , vmin(0)
        , vmax(0)
        , smoothingKernelSize(0)
    {
    }

    HistogramRequest(int binsCount,
                     int mode,
                     const ImagePtr & image,
                     const RectI & rect,
                     double vmin,
                     double vmax,
                     int smoothingKernelSize)
        : binsCount(binsCount)
        , mode(mode)
        , image(image)
        , rect(rect)
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

HistogramCPU::HistogramCPU()
    : QThread()
    , _imp( new HistogramCPUPrivate() )
{
}

HistogramCPU::~HistogramCPU()
{
    quitAnyComputation();
}

void
HistogramCPU::computeHistogram(int mode,      //< corresponds to the enum Histogram::DisplayModeEnum
                               const ImagePtr & image,
                               const RectI & rect,
                               int binsCount,
                               double vmin,
                               double vmax,
                               int smoothingKernelSize)
{
    /*Starting or waking-up the thread*/
    QMutexLocker quitLocker(&_imp->mustQuitMutex);
    QMutexLocker locker(&_imp->requestMutex);

    _imp->requests.push_back( HistogramRequest(binsCount, mode, image, rect, vmin, vmax, smoothingKernelSize) );
    if (!isRunning() && !_imp->mustQuit) {
        quitLocker.unlock();
        start(HighestPriority);
    } else {
        quitLocker.unlock();
        _imp->requestCond.wakeOne();
    }
}

void
HistogramCPU::quitAnyComputation()
{
    if ( isRunning() ) {
        QMutexLocker l(&_imp->mustQuitMutex);
        assert(!_imp->mustQuit);
        _imp->mustQuit = true;

        ///post a fake request to wakeup the thread
        l.unlock();
        computeHistogram(0, ImagePtr(), RectI(), 0, 0, 0, 0);
        l.relock();
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
    }
}

bool
HistogramCPU::hasProducedHistogram() const
{
    QMutexLocker l(&_imp->producedMutex);

    return !_imp->produced.empty();
}

bool
HistogramCPU::getMostRecentlyProducedHistogram(std::vector<float>* histogram1,
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

///putting these in an anonymous namespace will yield this error on gcc 4.2:
///"function has not external linkage"
struct pix_red
{
    static float val(const float *pix)
    {
        return pix[0];
    }
};

struct pix_green
{
    static float val(const float *pix)
    {
        return pix[1];
    }
};

struct pix_blue
{
    static float val(const float *pix)
    {
        return pix[2];
    }
};

struct pix_alpha
{
    static float val(const float *pix)
    {
        return pix[3];
    }
};

struct pix_lum
{
    static float val(const float *pix)
    {
        return 0.299 * pix[0] + 0.587 * pix[1] + 0.114 * pix[2];
    }
};


template <float pix_func(const float*)>
void
computeHisto(const HistogramRequest & request,
             int upscale,
             std::vector<float> *histo)
{
    assert(histo);
    histo->resize(request.binsCount * upscale);
    std::fill(histo->begin(), histo->end(), 0.f);
    double binSize = (request.vmax - request.vmin) / histo->size();

    ///Images come from the viewer which is in float.
    assert(request.image->getBitDepth() == eImageBitDepthFloat);

    Image::ReadAccess acc = request.image->getReadRights();

    for (int y = request.rect.bottom(); y < request.rect.top(); ++y) {
        for (int x = request.rect.left(); x < request.rect.right(); ++x) {
            const float *pix = (const float*)acc.pixelAt(x, y);
            float v = pix_func(pix);
            if ( (request.vmin <= v) && (v < request.vmax) ) {
                int index = (int)( (v - request.vmin) / binSize );
                assert( 0 <= index && index < (int)histo->size() );
                (*histo)[index] += 1.f;
            }
        }
    }
}



static void
computeHistogramStatic(const HistogramRequest & request,
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

    /// keep the mode parameter in sync with Histogram::DisplayModeEnum

    int mode = request.mode;

    ///if the mode is RGB, adjust the mode to either R,G or B depending on the histogram index
    if (mode == 0) {
        mode = histogramIndex + 2;
    }

    ret->pixelsCount = request.rect.area();
    // a histogram with upscale more bins
    std::vector<float> histo_upscaled;
    switch (mode) {
    case 1:     //< A
        computeHisto<&pix_alpha::val>(request, upscale, &histo_upscaled);
        break;
    case 2:     //<Y
        computeHisto<&pix_lum::val>(request, upscale, &histo_upscaled);
        break;
    case 3:     //< R
        computeHisto<&pix_red::val>(request, upscale, &histo_upscaled);
        break;
    case 4:     //< G
        computeHisto<&pix_green::val>(request, upscale, &histo_upscaled);
        break;
    case 5:     //< B
        computeHisto<&pix_blue::val>(request, upscale, &histo_upscaled);
        break;

    default:
        assert(false);
        break;
    }
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
HistogramCPU::run()
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
#endif
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
        FinishedHistogramPtr ret = boost::make_shared<FinishedHistogram>();
        ret->binsCount = request.binsCount;
        ret->mode = request.mode;
        ret->vmin = request.vmin;
        ret->vmax = request.vmax;
        ret->mipMapLevel = request.image->getMipMapLevel();


        switch (request.mode) {
        case 0:     //< RGB
            computeHistogramStatic(request, ret, 1);
            computeHistogramStatic(request, ret, 2);
            computeHistogramStatic(request, ret, 3);
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            computeHistogramStatic(request, ret, 1);
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
