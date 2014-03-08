
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

#ifndef NATRON_ENGINE_INTERPOLATION_H_
#define NATRON_ENGINE_INTERPOLATION_H_

#include "Global/Enums.h"
namespace Natron
{



/**
 * @brief Interpolates using the control points P0(t0,v0) , P3(t3,v3)
 * and the derivatives P1(t1,v1) (being the derivative at P0 with respect to
 * t \in [t1,t2]) and P2(t2,v2) (being the derivative at P3 with respect to
 * t \in [t1,t2]) the value at 'currentTime' using the
 * interpolation method "interp".
 * Note that for CATMULL-ROM you must use the function interpolate_catmullRom
 * which will compute the derivatives for you.
 **/
double interpolate(double tcur, const double vcur, //start control point
                   const double vcurDerivRight, //being the derivative dv/dt at tcur
                   const double vnextDerivLeft, //being the derivative dv/dt at tnext
                   double tnext, const double vnext, //end control point
                   double currentTime,
                   KeyframeType interp,
                   KeyframeType interpNext) WARN_UNUSED_RETURN;

/// derive at currentTime. The derivative is with respect to currentTime
double derive(double tcur, const double vcur, //start control point
              const double vcurDerivRight, //being the derivative dv/dt at tcur
              const double vnextDerivLeft, //being the derivative dv/dt at tnext
              double tnext, const double vnext, //end control point
              double currentTime,
              KeyframeType interp,
              KeyframeType interpNext) WARN_UNUSED_RETURN;

/// interpolate and derive at currentTime. The derivative is with respect to currentTime
void interpolate_and_derive(double tcur, const double vcur, //start control point
                            const double vcurDerivRight, //being the derivative dv/dt at tcur
                            const double vnextDerivLeft, //being the derivative dv/dt at tnext
                            double tnext, const double vnext, //end control point
                            double currentTime,
                            KeyframeType interp,
                            KeyframeType interpNext,
                            double *valueAt,
                            double *derivativeAt);

/// integrate from time1 to time2 - be careful that time1 and time2 have to be in the range [tcur,tnext]
double integrate(double tcur, const double vcur, //start control point
                 const double vcurDerivRight, //being the derivative dv/dt at tcur
                 const double vnextDerivLeft, //being the derivative dv/dt at tnext
                 double tnext, const double vnext, //end control point
                 double time1, double time2,
                 KeyframeType interp,
                 KeyframeType interpNext) WARN_UNUSED_RETURN;

/**
 * @brief This function will set the left and right derivative of 'cur', depending on the interpolation method 'interp' and the
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
 * We also know that the 1st derivative of B(t) at 0 is the derivative to P0
 * and the 1st derivative of B(t) at 1 is the derivative to P3, i.e:
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

derivativeAtCurRight := dP(0)/(tnext-tcur):
curvatureAtCurRight := dP2(0)/(tnext-tcur):
curvatureAtNextLeft:= dP2(1)/(tnext - tcur):
 derivativeAtCurLeft := dQ(1)/(tcur-tprev):
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
solve({curvatureAtCurRight = curvatureAtCurLeft, derivativeAtCurRight = derivativeAtCurLeft}, {P0pr, Q3pl});
map(C,%):

printf("cubic, prev is linear:"):
solve({curvatureAtCurRight = curvatureAtCurLeft, derivativeAtCurRight = derivativeAtCurLeft, curvatureAtPrevRight = 0},{P0pr, Q3pl, Q0pr});
map(C,%):

printf("cubic, next is linear:"):
solve({curvatureAtCurRight = curvatureAtCurLeft, derivativeAtCurRight = derivativeAtCurLeft, curvatureAtNextLeft = 0}, {P0pr, Q3pl, P3pl});
map(C,%):

printf("cubic, prev and next are linear"):
solve({curvatureAtCurRight = curvatureAtCurLeft, derivativeAtCurRight = derivativeAtCurLeft, curvatureAtPrevRight = 0, curvatureAtNextLeft = 0},{P0pr, Q3pl, Q0pr, P3pl});
map(C,%):

*/
void autoComputeDerivatives(Natron::KeyframeType interpPrev,
                            Natron::KeyframeType interp,
                            Natron::KeyframeType interpNext,
                            double tprev, const double vprev, // vprev = Q0
                            double tcur, const double vcur, // vcur = Q3 = P0
                            double tnext, const double vnext, // vnext = P3
                            const double vprevDerivRight, // Q0'_r
                            const double vnextDerivLeft, // P3'_l
                            double *vcurDerivLeft, // Q3'_l
                            double *vcurDerivRight);  // P0'_r

}



#endif // NATRON_ENGINE_INTERPOLATION_H_
