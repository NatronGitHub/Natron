/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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


#include "Smooth1D.h"

NATRON_NAMESPACE_ENTER

NATRON_NAMESPACE_ANONYMOUS_ENTER

/// IIR Gaussian filter: recursive implementation.

static void
YvVfilterCoef(double sigma,
              double *filter)
{
    /* the recipe in the Young-van Vliet paper:
     * I.T. Young, L.J. van Vliet, M. van Ginkel, Recursive Gabor filtering.
     * IEEE Trans. Sig. Proc., vol. 50, pp. 2799-2805, 2002.
     *
     * (this is an improvement over Young-Van Vliet, Sig. Proc. 44, 1995)
     */

    double q, qsq;
    double scale;
    double B, b1, b2, b3;

    /* initial values */
    double m0 = 1.16680, m1 = 1.10783, m2 = 1.40586;
    double m1sq = m1 * m1, m2sq = m2 * m2;

    /* calculate q */
    if (sigma < 3.556) {
        q = -0.2568 + 0.5784 * sigma + 0.0561 * sigma * sigma;
    } else {
        q = 2.5091 + 0.9804 * (sigma - 3.556);
    }

    qsq = q * q;

    /* calculate scale, and b[0,1,2,3] */
    scale = (m0 + q) * (m1sq + m2sq + 2 * m1 * q + qsq);
    b1 = -q * (2 * m0 * m1 + m1sq + m2sq + (2 * m0 + 4 * m1) * q + 3 * qsq) / scale;
    b2 = qsq * (m0 + 2 * m1 + 3 * q) / scale;
    b3 = -qsq * q / scale;

    /* calculate B */
    B = ( m0 * (m1sq + m2sq) ) / scale;

    /* fill in filter */
    filter[0] = -b3;
    filter[1] = -b2;
    filter[2] = -b1;
    filter[3] = B;
    filter[4] = -b1;
    filter[5] = -b2;
    filter[6] = -b3;
}

/* Triggs matrix, from
 B. Triggs and M. Sdika. Boundary conditions for Young-van Vliet
 recursive filtering. IEEE Trans. Signal Processing,
 vol. 54, pp. 2365-2367, 2006.
 */
static void
TriggsM(double *filter,
        double *M)
{
    double scale;
    double a1, a2, a3;

    a3 = filter[0];
    a2 = filter[1];
    a1 = filter[2];

    scale = 1.0 / ( (1.0 + a1 - a2 + a3) * (1.0 - a1 - a2 - a3) * (1.0 + a2 + (a1 - a3) * a3) );
    M[0] = scale * (-a3 * a1 + 1.0 - a3 * a3 - a2);
    M[1] = scale * (a3 + a1) * (a2 + a3 * a1);
    M[2] = scale * a3 * (a1 + a3 * a2);
    M[3] = scale * (a1 + a3 * a2);
    M[4] = -scale * (a2 - 1.0) * (a2 + a3 * a1);
    M[5] = -scale * a3 * (a3 * a1 + a3 * a3 + a2 - 1.0);
    M[6] = scale * (a3 * a1 + a2 + a1 * a1 - a2 * a2);
    M[7] = scale * (a1 * a2 + a3 * a2 * a2 - a1 * a3 * a3 - a3 * a3 * a3 - a3 * a2 + a3);
    M[8] = scale * a3 * (a1 + a3 * a2);
}

// IIR filter.
// may operate in-place, using src=dest
template<class SrcIterator, class DstIterator>
static void
iir_1d_filter(SrcIterator src,
              DstIterator dest,
              int sx,
              double *filter)
{
    int j;
    double b1, b2, b3;
    double pix, p1, p2, p3;
    double sum, sumsq;
    double iplus, uplus, vplus;
    double unp, unp1, unp2;
    double M[9];

    sumsq = filter[3];
    sum = sumsq * sumsq;

    /* causal filter */
    b1 = filter[2]; b2 = filter[1]; b3 = filter[0];
    p1 = *src / sumsq; p2 = p1; p3 = p1;

    iplus = src[sx - 1];
    for (j = 0; j < sx; j++) {
        pix = *src++ + b1 * p1 + b2 * p2 + b3 * p3;
        *dest++ = pix;
        p3 = p2; p2 = p1; p1 = pix; /* update history */
    }

    /* anti-causal filter */

    /* apply Triggs border condition */
    uplus = iplus / (1.0 - b1 - b2 - b3);
    b1 = filter[4]; b2 = filter[5]; b3 = filter[6];
    vplus = uplus / (1.0 - b1 - b2 - b3);

    unp = p1 - uplus;
    unp1 = p2 - uplus;
    unp2 = p3 - uplus;

    TriggsM(filter, M);

    pix = M[0] * unp + M[1] * unp1 + M[2] * unp2 + vplus;
    p1  = M[3] * unp + M[4] * unp1 + M[5] * unp2 + vplus;
    p2  = M[6] * unp + M[7] * unp1 + M[8] * unp2 + vplus;
    pix *= sum; p1 *= sum; p2 *= sum;

    *(--dest) = pix;
    p3 = p2; p2 = p1; p1 = pix;

    for (j = sx - 2; j >= 0; j--) {
        pix = sum * *(--dest) + b1 * p1 + b2 * p2 + b3 * p3;
        *dest = pix;
        p3 = p2; p2 = p1; p1 = pix;
    }
} // iir_1d_filter

NATRON_NAMESPACE_ANONYMOUS_EXIT

namespace Smooth1D {


    void iir_gaussianFilter1D(std::vector<float>& curve, int smoothingKernelSize)
    {
        if (curve.size() < 3) {
            return;
        }
        double sigma = 5.;
        if (smoothingKernelSize > 1) {
            sigma *= smoothingKernelSize;
        }

        double filter[7];
        // calculate filter coefficients
        YvVfilterCoef(sigma, filter);
        // filter
        iir_1d_filter(curve.begin(), curve.begin(), curve.size(), filter);
    }

    void laplacian_1D(std::vector<float>& curve)
    {
        if (curve.size() < 3) {
            return;
        }
        std::vector<float>::iterator prev = curve.begin();
        std::vector<float>::iterator it = prev;
        ++it;
        std::vector<float>::iterator next = it;
        ++next;
        for (; next != curve.end(); ++prev, ++it, ++next) {
            *it = ((*prev) + (2.f * *it) + (*next)) / 4.f;
        }
    }

} // namespace Smooth1D

NATRON_NAMESPACE_EXIT
