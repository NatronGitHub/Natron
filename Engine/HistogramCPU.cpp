    //  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "HistogramCPU.h"

#include <algorithm>
#include <QMutex>
#include <QWaitCondition>

#include "Engine/Image.h"


struct HistogramRequest {
    int binsCount;
    int mode;
    boost::shared_ptr<Natron::Image> image;
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
                     const boost::shared_ptr<Natron::Image>& image,
                     const RectI& rect,
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

struct FinishedHistogram {
    std::vector<float> histogram1;
    std::vector<float> histogram2;
    std::vector<float> histogram3;
    int mode;
    int binsCount;
    int pixelsCount;
    double vmin,vmax;
    
    FinishedHistogram()
    : histogram1()
    , histogram2()
    , histogram3()
    , mode(0)
    , binsCount(0)
    , pixelsCount(0)
    , vmin(0)
    , vmax(0)
    {
        
    }
};

struct HistogramCPUPrivate
{
    
    QWaitCondition requestCond;
    QMutex requestMutex;
    std::list<HistogramRequest> requests;
    
    QMutex producedMutex;
    std::list<boost::shared_ptr<FinishedHistogram> > produced;
    
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
, _imp(new HistogramCPUPrivate())
{
}

HistogramCPU::~HistogramCPU()
{
    quitAnyComputation();
}

void HistogramCPU::computeHistogram(int mode, //< corresponds to the enum Histogram::DisplayMode
                                    const boost::shared_ptr<Natron::Image>& image,
                                    const RectI& rect,
                                    int binsCount,
                                    double vmin,
                                    double vmax,
                                    int smoothingKernelSize)
{
    /*Starting or waking-up the thread*/
    QMutexLocker quitLocker(&_imp->mustQuitMutex);
    QMutexLocker locker(&_imp->requestMutex);
    _imp->requests.push_back(HistogramRequest(binsCount,mode,image,rect,vmin,vmax,smoothingKernelSize));
    if (!isRunning() && !_imp->mustQuit) {
        quitLocker.unlock();
        start(HighestPriority);
    } else {
        quitLocker.unlock();
        _imp->requestCond.wakeOne();
    }

}

void HistogramCPU::quitAnyComputation()
{
    if (isRunning()) {
        QMutexLocker l(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        ///post a fake request to wakeup the thread
        l.unlock();
        computeHistogram(0, boost::shared_ptr<Natron::Image>(), RectI(), 0,0,0,0);
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
                                               double* vmax)
{
    assert(histogram1 && histogram2 && histogram3 && binsCount && pixelsCount && mode && vmin && vmax);

    QMutexLocker l(&_imp->producedMutex);
    if (_imp->produced.empty()) {
        return false;
    }
    
#pragma message WARN("this is not the most recent! the most recent is back(), since we push_back() below")
    boost::shared_ptr<FinishedHistogram> h = _imp->produced.front();

    *histogram1 = h->histogram1;
    *histogram2 = h->histogram2;
    *histogram3 = h->histogram3;
    *binsCount = h->binsCount;
    *pixelsCount = h->pixelsCount;
    *mode = h->mode;
    *vmin = h->vmin;
    *vmax = h->vmax;
    _imp->produced.pop_front();

    return true;
}

namespace {
struct pix_red {
    static float
    val(float *pix)
    {
        return pix[0];
    }
};

struct pix_green {
    static float
    val(float *pix)
    {
        return pix[1];
    }
};

struct pix_blue {
    static float
    val(float *pix)
    {
        return pix[2];
    }
};

struct pix_alpha {
    static float
    val(float *pix)
    {
        return pix[3];
    }
};

struct pix_lum {
    static float
    val(float *pix)
    {
        return 0.299 * pix[0] + 0.587 * pix[1] + 0.114 * pix[2];
    }
};
}

template <float pix_func(float*)>
void
computeHisto(const HistogramRequest& request, std::vector<float> *histo)
{
    assert(histo);
    histo->resize(request.binsCount);
    std::fill(histo->begin(), histo->end(), 0.f);

    double binSize = (request.vmax - request.vmin) / request.binsCount;


    for (int y = request.rect.bottom() ; y < request.rect.top(); ++y) {
        for (int x = request.rect.left(); x < request.rect.right(); ++x) {
            float *pix = request.image->pixelAt(x, y) ;
            float v = pix_func(pix);
            if (request.vmin <= v && v < request.vmax) {
                int index = (int)((v - request.vmin) / binSize);
                assert(0 <= index && index < (int)histo->size());
                (*histo)[index] += 1.f;
            }
        }
    }
}


static void
computeHistogramStatic(const HistogramRequest& request, boost::shared_ptr<FinishedHistogram> ret, int histogramIndex)
{
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

    /// keep the mode parameter in sync with Histogram::DisplayMode

    int mode = request.mode;

    ///if the mode is RGB, adjust the mode to either R,G or B depending on the histogram index
    if (mode == 0) {
        mode = histogramIndex + 2;
    }

    ret->pixelsCount = request.rect.area();

    switch (mode) {
        case 1: //< A
            computeHisto<&pix_alpha::val>(request, histo);
            break;
        case 2: //<Y
            computeHisto<&pix_lum::val>(request, histo);
            break;
        case 3: //< R
            computeHisto<&pix_red::val>(request, histo);
            break;
        case 4: //< G
            computeHisto<&pix_green::val>(request, histo);
            break;
        case 5: //< B
            computeHisto<&pix_blue::val>(request, histo);
            break;

        default:
            assert(false);
            break;
    }

    ///Apply the binomial filter if any (the filter size has to be an odd number)
    assert(request.smoothingKernelSize >= 0);
    assert((request.smoothingKernelSize == 0) || (request.smoothingKernelSize & 1));

    for (int k = request.smoothingKernelSize; k > 1; k -=2) {
        const std::vector<float> histogramCopy = *histo;

        for (int i = 1; i < ((int)histo->size() - 1); ++i) {
            (*histo)[i] = (histogramCopy[i-1] + 2*histogramCopy[i] + histogramCopy[i+1]) / 4;
        }
    }
}

void
HistogramCPU::run()
{
    for (;;) {
        HistogramRequest request;
        {
            QMutexLocker l(&_imp->requestMutex);
            while (_imp->requests.empty()) {
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
        
        
        boost::shared_ptr<FinishedHistogram> ret(new FinishedHistogram);
        ret->binsCount = request.binsCount;
        ret->mode = request.mode;
        ret->vmin = request.vmin;
        ret->vmax = request.vmax;
        

        switch (request.mode) {
            case 0: //< RGB
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
                assert(false); //< unknown case.
                break;
        }
        
        
        {
            QMutexLocker l(&_imp->producedMutex);
            _imp->produced.push_back(ret);
        }
        emit histogramProduced();
    }
}