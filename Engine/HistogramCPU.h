//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef HISTOGRAMCPU_H
#define HISTOGRAMCPU_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <vector>
#include <QThread>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"

namespace Natron {
class Image;
}
class RectI;
struct HistogramCPUPrivate;
class HistogramCPU
    : public QThread
{
CLANG_DIAG_OFF_36(inconsistent-missing-override)
    Q_OBJECT
CLANG_DIAG_ON_36(inconsistent-missing-override)

public:

    HistogramCPU();

    virtual ~HistogramCPU();

    void computeHistogram(int mode, //< corresponds to the enum Histogram::DisplayModeEnum
                          const boost::shared_ptr<Natron::Image> & image,
                          const RectI & rect,
                          int binsCount,
                          double vmin,
                          double vmax,
                          int smoothingKernelSize);

    ////Returns true if a new histogram fully computed is available
    bool hasProducedHistogram() const;

    ///Returns the most recently produced histogram.
    ///This function should be called as a result of the histogramProduced signal reception.
    ///If this function couldn't return a valid histogram, it will return false.
    ///It is safe to assert this function returns true if it is called in the slot connected
    ///to the histogramProduced signal.
    ///
    ///This function returns in histogram1 the first histogram of the produced histogram
    bool getMostRecentlyProducedHistogram(std::vector<float>* histogram1,
                                          std::vector<float>* histogram2,
                                          std::vector<float>* histogram3,
                                          unsigned int* binsCount,
                                          unsigned int* pixelsCount,
                                          int* mode,
                                          double* vmin,double* vmax,unsigned int* mipMapLevel);

    void quitAnyComputation();

Q_SIGNALS:

    void histogramProduced();

private:

    virtual void run() OVERRIDE FINAL;
    boost::scoped_ptr<HistogramCPUPrivate> _imp;
};

#endif // HISTOGRAMCPU_H
