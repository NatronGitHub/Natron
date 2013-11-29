
//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <cmath>
#include <stdexcept>
#include "Global/Enums.h"
namespace Natron
{



/**
 * @brief Interpolates using the control points P0(t0,v0) , P3(t3,v3)
 * and the tangents P1(t1,v1) (being the tangent of P0) and P2(t2,v2)
 * (being the tangent of P3) the value at 'currentTime' using the
 * interpolation method "interp".
 * Note that for CATMULL-ROM you must use the function interpolate_catmullRom
 * which will compute the tangents for you.
 **/
template <typename T>
T interpolate(double tcur, const T vcur, //start control point
              const T vcurDerivRight, //being the derivative dv/dt at tcur
              const T vnextDerivLeft, //being the derivative dv/dt at tnext
              double tnext, const T vnext, //end control point
              double currentTime,
              KeyframeType interp,
              KeyframeType interpNext)
{
    const T P0 = vcur;
    const T P0pr = vcurDerivRight;
    const T P3pl = vnextDerivLeft;
    const T P3 = vnext;
    assert((interp == KEYFRAME_NONE || currentTime >= tcur) && (interpNext == KEYFRAME_NONE || currentTime <= tnext));
    // after the last / before the first keyframe, derivatives are wrt currentTime (i.e. non-normalized)
    if (interp == KEYFRAME_NONE) {
        tcur = tnext - 1.;
    }
    if (interpNext == KEYFRAME_NONE) {
        tnext = tcur - 1;
    }
    const double t = (currentTime - tcur)/(tnext - tcur);
    const double t2 = t * t;
    const double t3 = t2 * t;
    if (interpNext == KEYFRAME_NONE) {
        assert(interp != KEYFRAME_NONE);
        // t is normalized between 0 and 1, and P0pr is the derivative wrt currentTime
        return P0 + t * P0pr;
    }
    
    switch (interp) {
    case KEYFRAME_LINEAR:
    case KEYFRAME_HORIZONTAL:
    case KEYFRAME_CATMULL_ROM:
    case KEYFRAME_SMOOTH:
    case KEYFRAME_CUBIC:
    case KEYFRAME_FREE:
    case KEYFRAME_BROKEN:
        //i.e: hermite cubic spline interpolation
        return ((2 * t3 - 3 * t2 + 1) * P0 +
                (t3 - 2 * t2 + t) * P0pr +
                (-2 * t3 + 3 * t2) * P3 +
                (t3 - t2) * P3pl);
    case KEYFRAME_CONSTANT:
        return t < tnext ? P0 : P3;
    case KEYFRAME_NONE:
        // t is normalized between 0 and 1, and P3pl is the derivative wrt currentTime
        return P3 - (1. - t) * P3pl;
    }
}


/**
 * @brief This function will set the left and right tangent of 'cur', depending on the interpolation method 'interp' and the
 * previous and next key frames.
 * ----------------------------------------------------------------------------
 * Using the Bezier cubic equation, its 2nd derivative can be expressed as such:
 * B''(t) = 6(1-t)(P2 - 2P1 + P0) + 6t(P3 - 2P2 + P1)
 * We have P1 = P0 + P0'_r / 3
 * and Q2 = Q3 - Q3'_l / 3
 * We can insert it in the 2nd derivative form, which yields:
 * B''(t) = 6(1-t)(P3 - P3'_l/3 - P0 - 2P0'_r/3) + 6t(P0 - P3 + 2P3'_l/3 + P0'_r/3)
 *
 * So for t = 0, we have:
 * B''(0) = 6(P3 - P0 - P3'_l / 3 - 2P0'_r / 3)
 * and for t = 1 , we have:
 * Q''(1) = 6(Q0 - Q3 + 2Q3'_l / 3 + Q0'_r / 3)
 *
 * We also know that the 1st derivative of B(t) at 0 is the tangent to P0
 * and the 1st derivative of B(t) at 1 is the tangent to P3, i.e:
 * B'(0) = P0'_r
 * B'(1) = P3'_l
 **/
/*
  Maple code to compute the values for each case:
with(CodeGeneration):

P := t -> (1-t)**3 * P0 + 3 * (1-t)**2 * t * P1 + 3 * (1-t) * t**2 * P2 + t**3 * P3:
Q := t -> (1-t)**3 * Q0 + 3 * (1-t)**2 * t * Q1 + 3 * (1-t) * t**2 * Q2 + t**3 * Q3:

dP := D(P):
dP2 := D(dP):
dQ := D(Q):
dQ2 := D(dQ):

P1 := P0 + P0pr / 3:
Q2 := Q3 - Q3pl / 3:
Q1 := Q0 + Q0pr / 3:
P2 := P3 - P3pl / 3:
Q3 := P0:

tangentAtCurRight := dP(0)/(tnext-tcur):
curvatureAtCurRight := dP2(0)/(tnext-tcur):
curvatureAtNextLeft:= dP2(1)/(tnext - tcur):
 tangentAtCurLeft := dQ(1)/(tcur-tprev):
curvatureAtCurLeft:= dQ2(1)/(tcur - tprev):
curvatureAtPrevRight:= dQ2(0)/(tcur - tprev):

printf("linear, general case:"):
solve( {curvatureAtCurRight = 0, curvatureAtCurLeft = 0}, { P0pr, Q3pl });
map(C,%):

printf("linear, prev is linear:"):
solve({curvatureAtCurRight = 0, curvatureAtCurLeft = 0, curvatureAtPrevRight = 0}, { P0pr, Q3pl, Q0pr});
map(C,%):

printf("linear, next is linear:"):
solve({curvatureAtCurRight = 0, curvatureAtCurLeft = 0, curvatureAtNextLeft = 0}, {P0pr, Q3pl, P3pl});
map(C,%):

printf("linear, prev and next are linear:"):
solve({curvatureAtCurRight = 0, curvatureAtCurLeft = 0, curvatureAtPrevRight = 0, curvatureAtNextLeft = 0}, {P0pr, Q3pl, Q0pr, P3pl});
map(C,%):

printf("cubic, general case:"):
solve({curvatureAtCurRight = curvatureAtCurLeft, tangentAtCurRight = tangentAtCurLeft}, {P0pr, Q3pl});
map(C,%):

printf("cubic, prev is linear:"):
solve({curvatureAtCurRight = curvatureAtCurLeft, tangentAtCurRight = tangentAtCurLeft, curvatureAtPrevRight = 0},{P0pr, Q3pl, Q0pr});
map(C,%):

printf("cubic, next is linear:"):
solve({curvatureAtCurRight = curvatureAtCurLeft, tangentAtCurRight = tangentAtCurLeft, curvatureAtNextLeft = 0}, {P0pr, Q3pl, P3pl});
map(C,%):

printf("cubic, prev and next are linear"):
solve({curvatureAtCurRight = curvatureAtCurLeft, tangentAtCurRight = tangentAtCurLeft, curvatureAtPrevRight = 0, curvatureAtNextLeft = 0},{P0pr, Q3pl, Q0pr, P3pl});
map(C,%):

*/
template <typename T>
void autoComputeTangents(Natron::KeyframeType interpPrev,
                         Natron::KeyframeType interp,
                         Natron::KeyframeType interpNext,
                         double tprev, const T vprev, // vprev = Q0
                         double tcur, const T vcur, // vcur = Q3 = P0
                         double tnext, const T vnext, // vnext = P3
                         const T vprevDerivRight, // Q0'_r
                         const T vnextDerivLeft, // P3'_l
                         T *vcurDerivLeft, // Q3'_l
                         T *vcurDerivRight)  // P0'_r
{
    const T Q0 = vprev;
    const T Q3 = vcur;
    const T P0 = vcur;
    const T P3 = vnext;
    const T Q0pr = vprevDerivRight;
    const T P3pl = vnextDerivLeft;
    T P0pr;
    T Q3pl;

    // if there are no keyframes before and after, the derivatives are zero
    if (interpPrev == KEYFRAME_NONE && interpNext == KEYFRAME_NONE) {
        *vcurDerivRight = 0.;
        *vcurDerivLeft = 0.;
    }

    // If there is no next/previous keyframe, should there be a continuous derivative?
    bool keyframe_none_same_derivative = false;

    // if there is no next/previous keyframe, use LINEAR interpolation, and set keyframe_none_same_derivative
    if (interpPrev == KEYFRAME_NONE || interpNext == KEYFRAME_NONE) {
        // Do this before modifying interp (next line)
        keyframe_none_same_derivative = (interp == KEYFRAME_CATMULL_ROM || interp == KEYFRAME_CUBIC);
        interp = KEYFRAME_LINEAR;
    }

    switch (interp) {
    case KEYFRAME_LINEAR:
        /* Linear means the the 2nd derivative of the cubic curve at the point 'cur' is zero. */
        if (interpNext == KEYFRAME_NONE) {
            P0pr = 0.;
        } else if (interpNext == KEYFRAME_LINEAR) {
            P0pr = -P0 + P3;
        } else {
            P0pr = -0.3e1 / 0.2e1 * P0 + 0.3e1 / 0.2e1 * P3 - P3pl / 0.2e1;
        }

        if (interpPrev == KEYFRAME_NONE) {
            Q3pl = 0.;
        } else if (interpPrev == KEYFRAME_LINEAR) {
            Q3pl = -Q0 + P0;
        } else {
            Q3pl = -0.3e1 / 0.2e1 * Q0 - Q0pr / 0.2e1 + 0.3e1 / 0.2e1 * P0;
        }

        if (keyframe_none_same_derivative) {
            if (interpNext == KEYFRAME_NONE) {
                P0pr = Q3pl/(tcur-tprev);
            } else if (interpPrev == KEYFRAME_NONE) {
                Q3pl = P0pr/(tnext-tcur);
            }
        }
        break;

    case KEYFRAME_CATMULL_ROM:
        {
            /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the tangents*/
            T deriv = (vnext - vprev) / (tnext - tprev);
            P0pr = deriv * (tnext - tcur);
            Q3pl = deriv * (tcur - tprev);
        }
        break;

    case KEYFRAME_SMOOTH:
        // If vcur is outside of the range [vprev,vnext], then interpolation is horizontal
        if ((vprev > vcur && vcur < vnext) || (vprev < vcur && vcur > vnext)) {
            P0pr = 0.;
            Q3pl = 0.;
        } else {
            // Catmull-Rom interpolatio, see above
            /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the tangents*/
            T deriv = (vnext - vprev) / (tnext - tprev);
            P0pr = deriv * (tnext - tcur);
            Q3pl = deriv * (tcur - tprev);

            /*Now that we have the tangent by catmull-rom's formula, we compute the bezier
              point on the left and on the right from the tangents (i.e: P1 and Q2, Q being the segment before P)
            */
            T P1 = P0 + P0pr / 3.;
            T Q2 = Q3 - Q3pl / 3.;

            /*We clamp Q2 to Q0(aka vprev) and Q3(aka vcur)
              and P1 to P0(aka vcur) and P3(aka vnext)*/
            T prevMax = std::max(vprev, vcur);
            T prevMin = std::min(vprev, vcur);
            Q2 = std::max(prevMin, std::min(Q2, prevMax));

            T nextMax = std::max(vcur, vnext);
            T nextMin = std::min(vcur, vnext);
            P1 = std::max(nextMin, std::min(P1, nextMax));

            /*We recompute the tangents from the new clamped control points*/

            P0pr = 3. * (P1 - P3);
            Q3pl = 3. * (Q3 - Q2);

            /*And set the tangent on the left and on the right to be the minimum
              of the 2.*/


            if (std::abs(Q3pl) < std::abs(P0pr)) {
                P0pr = Q3pl;
            } else {
                Q3pl = P0pr;
            }

        }
       break;

    case KEYFRAME_HORIZONTAL:
    case KEYFRAME_CONSTANT:
        /*The values are the same than the keyframe they belong. */
        P0pr = 0.;
        Q3pl = 0.;
        break;

    case KEYFRAME_CUBIC:
        /* Cubic means the the 2nd derivative of the cubic curve at the point 'cur' are equal. */
        if (interpPrev == KEYFRAME_LINEAR && interpNext == KEYFRAME_LINEAR) {
            P0pr = -(double)((Q0 * tnext - Q0 * tcur - P0 * tprev - P3 * tcur + P3 * tprev - P0 * tnext + 2 * P0 * tcur) / (tcur - tprev)) / 0.2e1;
            Q3pl = (double)((Q0 * tnext - Q0 * tcur - P0 * tprev - P3 * tcur + P3 * tprev - P0 * tnext + 2 * P0 * tcur) / (-tnext + tcur)) / 0.2e1;
        } else if (interpPrev == KEYFRAME_LINEAR) {
            P0pr = -(double)((-6 * P0 * tprev - 6 * P3 * tcur + 6 * P3 * tprev + 2 * P3pl * tcur - 2 * P3pl * tprev + 3 * Q0 * tnext - 3 * Q0 * tcur - 3 * P0 * tnext + 9 * P0 * tcur) / (tcur - tprev)) / 0.7e1;
            Q3pl = (double)((-6 * P0 * tprev - 6 * P3 * tcur + 6 * P3 * tprev + 2 * P3pl * tcur - 2 * P3pl * tprev + 3 * Q0 * tnext - 3 * Q0 * tcur - 3 * P0 * tnext + 9 * P0 * tcur) / (-tnext + tcur)) / 0.7e1;
        } else if (interpNext == KEYFRAME_LINEAR) {
            P0pr = -(double)((-3 * P0 * tprev - 3 * P3 * tcur + 3 * P3 * tprev + 6 * Q0 * tnext - 6 * Q0 * tcur + 2 * Q0pr * tnext - 2 * Q0pr * tcur - 6 * P0 * tnext + 9 * P0 * tcur) / (tcur - tprev)) / 0.7e1;
            Q3pl = (double)((-3 * P0 * tprev - 3 * P3 * tcur + 3 * P3 * tprev + 6 * Q0 * tnext - 6 * Q0 * tcur + 2 * Q0pr * tnext - 2 * Q0pr * tcur - 6 * P0 * tnext + 9 * P0 * tcur) / (-tnext + tcur)) / 0.7e1;
        } else {
            P0pr = -(double)((6 * P0 * tcur - 3 * P0 * tprev - 3 * P3 * tcur + 3 * P3 * tprev + P3pl * tcur - P3pl * tprev + 3 * Q0 * tnext - 3 * Q0 * tcur + Q0pr * tnext - Q0pr * tcur - 3 * P0 * tnext) / (tcur - tprev)) / 0.4e1;

            Q3pl = (double)((6 * P0 * tcur - 3 * P0 * tprev - 3 * P3 * tcur + 3 * P3 * tprev + P3pl * tcur - P3pl * tprev + 3 * Q0 * tnext - 3 * Q0 * tcur + Q0pr * tnext - Q0pr * tcur - 3 * P0 * tnext) / (-tnext + tcur)) / 0.4e1;

        }
        break;

    case KEYFRAME_NONE:
    case KEYFRAME_FREE:
    case KEYFRAME_BROKEN:
        throw std::runtime_error("Cannot compute derivatives at KEYFRAME_NONE, KEYFRAME_FREE or KEYFRAME_BROKEN");
    }

    *vcurDerivRight = P0pr;
    *vcurDerivLeft = Q3pl;
}

}



#endif // INTERPOLATION_H
