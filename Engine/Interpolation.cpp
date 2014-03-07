
//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  Created by Frédéric Devernay on 06/03/2014.

#include "Interpolation.h"

#include <cassert>
#include <cmath>
#include <stdexcept>
#include <boost/math/special_functions/fpclassify.hpp>

using namespace Natron;

static void
hermiteToCubicCoeffs(double P0, double P0pr, double P3pl, double P3, double *c0, double *c1, double *c2, double *c3)
{
    *c0 = P0;
    *c1 = P0pr;
    *c2 = 3 * (P3 - P0) - 2 * P0pr - P3pl;
    *c3 = -2 * (P3 - P0) + P0pr + P3pl;
}

// evaluate at t
static double
cubicEval(double c0, double c1, double c2, double c3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return c0 + c1*t + c2*t2 + c3*t3;
}


// integrate from 0 to t
static double
cubicIntegrate(double c0, double c1, double c2, double c3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    const double t4 = t3 * t;
    return c0*t + c1*t2/2. + c2*t3/3 + c3*t4/4;
}

// derive at t
static double
cubicDerive(double /*c0*/, double c1, double c2, double c3, double t)
{
    const double t2 = t * t;
    return c1 + 2*c2*t + 3*c3*t2;
}

/**
 * @brief Interpolates using the control points P0(t0,v0) , P3(t3,v3)
 * and the derivatives P1(t1,v1) (being the derivative at P0 with respect to
 * t \in [t1,t2]) and P2(t2,v2) (being the derivative at P3 with respect to
 * t \in [t1,t2]) the value at 'currentTime' using the
 * interpolation method "interp".
 * Note that for CATMULL-ROM you must use the function interpolate_catmullRom
 * which will compute the derivatives for you.
 **/
double Natron::interpolate(double tcur, const double vcur, //start control point
                           const double vcurDerivRight, //being the derivative dv/dt at tcur
                           const double vnextDerivLeft, //being the derivative dv/dt at tnext
                           double tnext, const double vnext, //end control point
                           double currentTime,
                           Natron::KeyframeType interp,
                           Natron::KeyframeType interpNext)
{
    double P0 = vcur;
    double P3 = vnext;
    // Hermite coefficients P0' and P3' are the derivatives with respect to x \in [0,1]
    double P0pr = vcurDerivRight*(tnext-tcur); // normalize for x \in [0,1]
    double P3pl = vnextDerivLeft*(tnext-tcur); // normalize for x \in [0,1]
    // if the following is true, this makes the special case for KEYFRAME_CONSTANT at tnext useless, and we can always use a cubic - the strict "currentTime < tnext" is the key
    assert(((interp == KEYFRAME_NONE) || (tcur <= currentTime)) && ((currentTime < tnext) || (interpNext == KEYFRAME_NONE)));
    // after the last / before the first keyframe, derivatives are wrt currentTime (i.e. non-normalized)
    if (interp == KEYFRAME_NONE) {
        // virtual previous frame at t-1
        P0 = P3 - P3pl;
        P0pr = P3pl;
        tcur = tnext - 1.;
    } else if (interp == KEYFRAME_CONSTANT) {
        P0pr = 0.;
        P3pl = 0.;
        P3 = P0;
    }
    if (interpNext == KEYFRAME_NONE) {
        // virtual next frame at t+1
        P3pl = P0pr;
        P3 = P0 + P0pr;
        tnext = tcur + 1;
    }
    double c0, c1, c2, c3;
    hermiteToCubicCoeffs(P0, P0pr, P3pl, P3, &c0, &c1, &c2, &c3);

    const double t = (currentTime - tcur)/(tnext - tcur);
    double ret = cubicEval(c0, c1, c2, c3, t);

    // cubicDerive: divide the result by (tnext-tcur)
    // cubicIntegrate: multiply the result by (tnext-tcur)
    return ret;
}

/// derive at currentTime. The derivative is with respect to currentTime
double Natron::derive(double tcur, const double vcur, //start control point
                           const double vcurDerivRight, //being the derivative dv/dt at tcur
                           const double vnextDerivLeft, //being the derivative dv/dt at tnext
                           double tnext, const double vnext, //end control point
                           double currentTime,
                           Natron::KeyframeType interp,
                           Natron::KeyframeType interpNext)
{
    double P0 = vcur;
    double P3 = vnext;
    // Hermite coefficients P0' and P3' are the derivatives with respect to x \in [0,1]
    double P0pr = vcurDerivRight*(tnext-tcur); // normalize for x \in [0,1]
    double P3pl = vnextDerivLeft*(tnext-tcur); // normalize for x \in [0,1]
    // if the following is true, this makes the special case for KEYFRAME_CONSTANT at tnext useless, and we can always use a cubic - the strict "currentTime < tnext" is the key
    assert(((interp == KEYFRAME_NONE) || (tcur <= currentTime)) && ((currentTime < tnext) || (interpNext == KEYFRAME_NONE)));
    // after the last / before the first keyframe, derivatives are wrt currentTime (i.e. non-normalized)
    if (interp == KEYFRAME_NONE) {
        // virtual previous frame at t-1
        P0 = P3 - P3pl;
        P0pr = P3pl;
        tcur = tnext - 1.;
    } else if (interp == KEYFRAME_CONSTANT) {
        P0pr = 0.;
        P3pl = 0.;
        P3 = P0;
    }
    if (interpNext == KEYFRAME_NONE) {
        // virtual next frame at t+1
        P3pl = P0pr;
        P3 = P0 + P0pr;
        tnext = tcur + 1;
    }
    double c0, c1, c2, c3;
    hermiteToCubicCoeffs(P0, P0pr, P3pl, P3, &c0, &c1, &c2, &c3);

    const double t = (currentTime - tcur)/(tnext - tcur);
    double ret = cubicDerive(c0, c1, c2, c3, t);

    // cubicDerive: divide the result by (tnext-tcur)
    // cubicIntegrate: multiply the result by (tnext-tcur)
    return ret / (tnext - tcur);
}

// integrate from time1 to time2
double Natron::integrate(double tcur, const double vcur, //start control point
                         const double vcurDerivRight, //being the derivative dv/dt at tcur
                         const double vnextDerivLeft, //being the derivative dv/dt at tnext
                         double tnext, const double vnext, //end control point
                         double time1, double time2,
                         Natron::KeyframeType interp,
                         Natron::KeyframeType interpNext)
{
    double P0 = vcur;
    double P3 = vnext;
    // Hermite coefficients P0' and P3' are the derivatives with respect to x \in [0,1]
    double P0pr = vcurDerivRight*(tnext-tcur); // normalize for x \in [0,1]
    double P3pl = vnextDerivLeft*(tnext-tcur); // normalize for x \in [0,1]
    // in the next expression, the correct test is t2 <= tnext (not <), in order to integrate from tcur to tnext
    assert(((interp == KEYFRAME_NONE) || (tcur <= time1)) && (time1 <= time2) && ((time2 <= tnext) || (interpNext == KEYFRAME_NONE)));
    // after the last / before the first keyframe, derivatives are wrt currentTime (i.e. non-normalized)
    if (interp == KEYFRAME_NONE) {
        // virtual previous frame at t-1
        P0 = P3 - P3pl;
        P0pr = P3pl;
        tcur = tnext - 1.;
    } else if (interp == KEYFRAME_CONSTANT) {
        P0pr = 0.;
        P3pl = 0.;
        P3 = P0;
    }
    if (interpNext == KEYFRAME_NONE) {
        // virtual next frame at t+1
        P3pl = P0pr;
        P3 = P0 + P0pr;
        tnext = tcur + 1;
    }
    double c0, c1, c2, c3;
    hermiteToCubicCoeffs(P0, P0pr, P3pl, P3, &c0, &c1, &c2, &c3);

    const double t2 = (time2 - tcur)/(tnext - tcur);
    double ret = cubicIntegrate(c0, c1, c2, c3, t2);
    if (time1 != tcur) {
        const double t1 = (time1 - tcur)/(tnext - tcur);
        ret -= cubicIntegrate(c0, c1, c2, c3, t1);
    }
    // cubicDerive: divide the result by (tnext-tcur)
    // cubicIntegrate: multiply the result by (tnext-tcur)
    return ret * (tnext - tcur);
}


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
void Natron::autoComputeDerivatives(Natron::KeyframeType interpPrev,
                                    Natron::KeyframeType interp,
                                    Natron::KeyframeType interpNext,
                                    double tprev, const double vprev, // vprev = Q0
                                    double tcur, const double vcur, // vcur = Q3 = P0
                                    double tnext, const double vnext, // vnext = P3
                                    const double vprevDerivRight, // Q0'_r
                                    const double vnextDerivLeft, // P3'_l
                                    double *vcurDerivLeft, // Q3'_l
                                    double *vcurDerivRight)  // P0'_r
{
    const double Q0 = vprev;
    const double Q3 = vcur;
    const double P0 = vcur;
    const double P3 = vnext;
    // Hermite coefficients P0' and P3' are the derivatives with respect to x \in [0,1]
    if (interpPrev == KEYFRAME_NONE) {
        tprev = tcur - 1.;
    }
    if (interpNext == KEYFRAME_NONE) {
        tnext = tcur + 1.;
    }
    const double Q0pr = vprevDerivRight*(tcur-tprev); // normalize for x \in [0,1]
    const double P3pl = vnextDerivLeft*(tnext-tcur); // normalize for x \in [0,1]
    double P0pr = double();
    double Q3pl = double();

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
            /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the derivatives*/
            double deriv = (vnext - vprev) / (tnext - tprev);
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
                /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the derivatives*/
                double deriv = (vnext - vprev) / (tnext - tprev);
                P0pr = deriv * (tnext - tcur);
                Q3pl = deriv * (tcur - tprev);

                /*Now that we have the derivative by catmull-rom's formula, we compute the bezier
                 point on the left and on the right from the derivatives (i.e: P1 and Q2, Q being the segment before P)
                 */
                double P1 = P0 + P0pr / 3.;
                double Q2 = Q3 - Q3pl / 3.;

                /*We clamp Q2 to Q0(aka vprev) and Q3(aka vcur)
                 and P1 to P0(aka vcur) and P3(aka vnext)*/
                double prevMax = std::max(vprev, vcur);
                double prevMin = std::min(vprev, vcur);
                if (Q2 < prevMin || Q2 > prevMax) {
                    double Q2new = std::max(prevMin, std::min(Q2, prevMax));
                    P1 = P0 + (P1-P0) * (Q3-Q2new)/(Q3-Q2);
                    Q2 = Q2new;
                }

                double nextMax = std::max(vcur, vnext);
                double nextMin = std::min(vcur, vnext);
                if (P1 < nextMin || P1 > nextMax) {
                    double P1new = std::max(nextMin, std::min(P1, nextMax));
                    Q2 = Q3 - (Q3-Q2) * (P1new-P0)/(P1-P0);
                    P1 = P1new;
                }

                /*We recompute the derivatives from the new clamped control points*/

                P0pr = 3. * (P1 - P0);
                Q3pl = 3. * (Q3 - Q2);
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
    
    *vcurDerivRight = P0pr/(tnext-tcur); // denormalize for t \in [tcur,tnext]
    *vcurDerivLeft = Q3pl/(tcur-tprev); // denormalize for t \in [tprev,tcur]
    assert(!boost::math::isnan(*vcurDerivRight) && !boost::math::isnan(*vcurDerivLeft));
}
