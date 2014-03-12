//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef HISTOGRAMCPU_H
#define HISTOGRAMCPU_H
#include <vector>
#include <QThread>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include "Global/Macros.h"
namespace Natron {
    class Image;
}
class RectI;
struct HistogramCPUPrivate;
class HistogramCPU : public QThread
{
    
    Q_OBJECT
    
public:
    
    HistogramCPU();
    
    virtual ~HistogramCPU();
    
    void computeHistogram(int mode, //< corresponds to the enum Histogram::DisplayMode
                          const boost::shared_ptr<Natron::Image>& leftImage,
                          const boost::shared_ptr<Natron::Image>& rightImage,
                          const RectI& leftRect,
                          const RectI& rightRect,
                          int binsCount,
                          double vmin,
                          double vmax);
    
    ////Returns true if a new histogram fully computed is available
    bool hasProducedHistogram() const;
    
    ///Returns the most recently produced histogram.
    ///This function should be called as a result of the histogramProduced signal reception.
    ///If this function couldn't return a valid histogram, it will return false.
    ///It is safe to assert this function returns true if it is called in the slot connected
    ///to the histogramProduced signal.
    ///
    ///This function returns in histogram1 the first histogram of the produced histogram
    bool getMostRecentlyProducedHistogram(std::vector<unsigned int>* histogram1,
                                          std::vector<unsigned int>* histogram2,
                                          std::vector<unsigned int>* histogram3,
                                          unsigned int* binsCount,
                                          unsigned int* pixelsCount,
                                          int* mode,
                                          double* vmin,double* vmax);
    
    void quitAnyComputation();
    
signals:
    
    void histogramProduced();
    
private:
    
    virtual void run() OVERRIDE FINAL;
    
    boost::scoped_ptr<HistogramCPUPrivate> _imp;
    
    
};

#endif // HISTOGRAMCPU_H
