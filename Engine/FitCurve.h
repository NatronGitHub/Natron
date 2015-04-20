//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef FITCURVE_H
#define FITCURVE_H

#include <vector>
#include "Global/GlobalDefines.h"


/**
 * Utility functions to fit a bezier curve to a set of points
 **/
namespace FitCurve {
    
    struct SimpleBezierCP {
        Natron::Point p;
        Natron::Point leftTan,rightTan;
    };
    
    /**
     * @brief Fit a Bezier curve to a (sub)set of digitized points
     * @param points Array of digitized points
     * @param error User-defined error squared
     * @param generatedBezier[out] The fitted bezier generated
    **/
    void fit_cubic(const std::vector<Natron::Point>& points, double error,std::vector<SimpleBezierCP>* generatedBezier);
}

#endif // FITCURVE_H
