/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef HISTOGRAMCPU_H
#define HISTOGRAMCPU_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <vector>
#include <QThread>
#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif
#include "Global/Macros.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

struct HistogramCPUPrivate;

class HistogramCPU
    : public QThread
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

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

NATRON_NAMESPACE_EXIT;

#endif // HISTOGRAMCPU_H
