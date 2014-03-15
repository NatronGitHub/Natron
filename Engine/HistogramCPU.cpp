    //  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "HistogramCPU.h"
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
    float* histogram1;
    float* histogram2;
    float* histogram3;
    int mode;
    int binsCount;
    int pixelsCount;
    double vmin,vmax;
    
    FinishedHistogram()
    : histogram1(0)
    , histogram2(0)
    , histogram3(0)
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
    std::list<FinishedHistogram> produced;
    
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

HistogramCPU::~HistogramCPU() {
    quitAnyComputation();
    
 
    ///the thread is not running any longer.
    ///clear the histogram produced left over.
    {
        QMutexLocker l(&_imp->producedMutex);
        while (!_imp->produced.empty()) {
            FinishedHistogram& h = _imp->produced.front();
            free(h.histogram1);
            free(h.histogram2);
            free(h.histogram3);
            _imp->produced.pop_front();
        }
    }
}

void HistogramCPU::computeHistogram(int mode, //< corresponds to the enum Histogram::DisplayMode
                                    const boost::shared_ptr<Natron::Image>& image,
                                    const RectI& rect,
                                    int binsCount,
                                    double vmin,
                                    double vmax,
                                    int smoothingKernelSize) {
    
    
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

void HistogramCPU::quitAnyComputation() {
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

bool HistogramCPU::hasProducedHistogram() const {
    QMutexLocker l(&_imp->producedMutex);
    return !_imp->produced.empty();
}

bool HistogramCPU::getMostRecentlyProducedHistogram(std::vector<float>* histogram1,
                                      std::vector<float>* histogram2,
                                      std::vector<float>* histogram3,
                                      unsigned int* binsCount,
                                      unsigned int* pixelsCount,
                                      int* mode,
                                      double* vmin,double* vmax) {
    
    histogram1->clear();
    histogram2->clear();
    histogram3->clear();
    
    QMutexLocker l(&_imp->producedMutex);
    if (_imp->produced.empty()) {
        return false;
    }
    
    FinishedHistogram& h = *_imp->produced.begin();
    *binsCount = h.binsCount;
    *pixelsCount = h.pixelsCount;
    *mode = h.mode;
    *vmin = h.vmin;
    *vmax = h.vmax;
    
    if (h.histogram1) {
        histogram1->resize(*binsCount);
    }
    if (h.histogram2) {
        histogram2->resize(*binsCount);
    }
    if (h.histogram3) {
        histogram3->resize(*binsCount);
    }
    
    for (unsigned int i = 0; i < *binsCount; ++i) {
        if (h.histogram1) {
            histogram1->at(i) = h.histogram1[i];
        }
        if (h.histogram2) {
            histogram2->at(i) = h.histogram2[i];
        }
        if (h.histogram3) {
            histogram3->at(i) = h.histogram3[i];
        }
    }
    
    if (h.histogram1) {
        free(h.histogram1);
    }
    if (h.histogram2) {
        free(h.histogram2);
    }
    if (h.histogram3) {
        free(h.histogram3);
    }
    
    
    _imp->produced.pop_front();
    
    return true;
}

namespace  {

static void computeHistogramStatic(const HistogramRequest& request,FinishedHistogram* ret,int histogramIndex) {
    
    
    float* histo = 0;
    switch (histogramIndex) {
        case 1:
            ret->histogram1 = (float*)calloc(request.binsCount,sizeof(float));
            histo = ret->histogram1;
            break;
        case 2:
            ret->histogram2 = (float*)calloc(request.binsCount,sizeof(float));
            histo = ret->histogram2;
            break;
        case 3:
            ret->histogram3 = (float*)calloc(request.binsCount,sizeof(float));
            histo = ret->histogram3;
            break;
        default:
            break;
    }
    
    double binSize = (request.vmax - request.vmin) / request.binsCount;
    
    /// keep the mode parameter in sync with Histogram::DisplayMode
    
    int mode = request.mode;
    
    ///if the mode is RGB, adjust the mode to either R,G or B depending on the histogram index
    if (mode == 0) {
        mode = histogramIndex + 2;
    }
    
    ret->pixelsCount = request.rect.area();
    
    for (int y = request.rect.bottom() ; y < request.rect.top(); ++y) {
        for (int x = request.rect.left(); x < request.rect.right(); ++x) {
            float *pix = request.image->pixelAt(x, y) ;
            float v;
            switch (mode) {
                case 1: //< A
                    v = pix[3];
                    break;
                case 2: //<Y
                    v = 0.299 * pix[0] + 0.587 * pix[1] + 0.114 * pix[2];
                    break;
                case 3: //< R
                    v = pix[0];
                    break;
                case 4: //< G
                    v = pix[1];
                    break;
                case 5: //< B
                    v = pix[2];
                    break;
                    
                default:
                    assert(false);
                    break;
            }
            if (v >= request.vmin && v <= request.vmax) {
                int index = (int)((v - request.vmin) / binSize);
                ++histo[index];
            }
        }
    }
    
    ///Apply the filter if any
    assert(request.smoothingKernelSize == 0 || request.smoothingKernelSize == 3 || request.smoothingKernelSize == 5);
    
    if (request.smoothingKernelSize > 0) {
        
        std::vector<int> kernel;
        kernel.resize(request.smoothingKernelSize);
        if (request.smoothingKernelSize == 3) {
            kernel[0] = 1;kernel[1] = 2;kernel[2] = 1;
        } else if (request.smoothingKernelSize == 5) {
            kernel[0] = 1;kernel[1] = 4;kernel[2] = 6;kernel[3] = 4;kernel[4] = 1;
        }
        
        std::vector<float> histogramCopy(request.binsCount);
        std::copy(histo, histo + request.binsCount, histogramCopy.begin());
        
        int kernelHalfSize = std::floor(request.smoothingKernelSize / 2.);
        for (int i = kernelHalfSize; i < (request.binsCount - kernelHalfSize); ++i) {
            float sum = 0;
            float weights = 0;
            int kernelIndex = 0;
            for (int j = i - kernelHalfSize; j <= i + kernelHalfSize; ++j,++kernelIndex) {
                int w = kernel[kernelIndex];
                sum += w * histogramCopy[j];
                weights += w;
            }
            sum /= weights;
            histo[i] = sum;
        }
    }
}

}

void HistogramCPU::run() {
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
        
        
        FinishedHistogram ret;
        ret.binsCount = request.binsCount;
        ret.mode = request.mode;
        ret.vmin = request.vmin;
        ret.vmax = request.vmax;
        

        switch (request.mode) {
            case 0: //< RGB
                computeHistogramStatic(request, &ret, 1);
                computeHistogramStatic(request, &ret, 2);
                computeHistogramStatic(request, &ret, 3);
                break;
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
                computeHistogramStatic(request,&ret,1);
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