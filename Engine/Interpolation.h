
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

namespace Natron{
    
    enum Interpolation{
        CONSTANT = 0,
        LINEAR = 1,
        SMOOTH = 2,
        CATMULL_ROM = 3,
        CUBIC = 4,
        HORIZONTAL = 5
    };
    
    /**
     * @brief Interpolates using the control points P0(t0,v0) , P3(t3,v3)
     * and the tangents P1(t1,v1) (being the tangent of P0) and P2(t2,v2)
     * (being the tangent of P3) the value at 'currentTime' using the
     * interpolation method "interp".
     * Note that for CATMULL-ROM you must use the function interpolate_catmullRom
     * which will compute the tangents for you.
     **/
    template <typename T>
    T interpolate(double t0,const T v0, //start control point
                  const T v1, //being the tangent of (t0,v0)
                  const T v2, //being the tangent of (t3,v3)
                  double t3,const T v3, //end control point
                  double currentTime,
                  Interpolation interp){
        
        double tsquare = currentTime * currentTime;
        double tcube = tsquare * currentTime;
        switch(interp){
                
            case LINEAR:
                
                return ((currentTime - t0) / (t3 - t0)) * v0 +
                ((t3 - currentTime) / (t3 - t0)) * v3;
            
            case HORIZONTAL:
            case CATMULL_ROM:
            case SMOOTH:
            case CUBIC:
                //i.e: hermite cubic spline interpolation
                return ((2 * tcube - 3 * tsquare + 1) * v0) +
                ((tcube - 2 * tsquare + currentTime) * v1) +
                ((-2 * tcube + 3 * tsquare) * v3) +
                (tcube - tsquare) * v2;
           
            case CONSTANT:
            default: //defaults to a constant
                return currentTime < t3 ? v0 : v3;
        }
    }
    
    
    /**
     * @brief This function will set the left and right tangent of 'cur', depending on the interpolation method 'interp' and the
     * previous and next key frames.
     * ----------------------------------------------------------------------------
     * Using the Bezier cubic equation, its 2nd derivative can be expressed as such:
     * B''(t) = 6(1-t)(P2 - 2P1 + P0) + 6t(P3 - 2P2 + P1)
     * We have P1 = P0 + P0'_r / 3
     * and P2 = P3 - P3'_l / 3
     * We can insert it in the 2nd derivative form, which yields:
     * B''(t) = 6(1-t)(P3 - P3'_l/3 - P0 - 2P0'_r/3) + 6t(P0 - P3 + 2P3'_l/3 + P0'_r/3)
     *
     * So for t = 0, we have:
     * B''(0) = P3 - P0 - P3'_l / 3 - 2P0'_r / 3
     * and for t = 1 , we have:
     * B''(1) = P0 - P3 + 2P3'_l / 3 + P0'_r / 3
     *
     * We also know that the 1st derivative of B(t) at 0 is the tangent to P0
     * and the 1st derivative of B(t) at 1 is the tangent to P3, i.e:
     * B'(0) = P0'_r
     * B'(1) = P3'_l
     **/

    template <typename T>
    void autoComputeTangents(Natron::Interpolation interp,
                            double tprev,const T vprev,
                            double tcur,const T vcur,
                            double tnext,const T vnext,
                            double *tcur_leftTan,T* vcur_leftTan,
                            double *tcur_rightTan,T* vcur_rightTan){
        if(interp == CONSTANT){
            /*Constant interpolation is meaningless (i.e: we don't need tangents) so return*/
            return;
        }else if(interp == LINEAR){
            /* Linear means the the 2nd derivative of the cubic curve at the point 'cur' is null.
             * That means
             * B''(0) = B''(1) = 0
             * We have 2 equations:
             *          P3 - P0 - P3'_l / 3 - 2P0'_r / 3 = 0 (1)
             *          P0 - P3 + 2P3'_l / 3 - P0'_r / 3 = 0 (2)
             * Resolving this system yields:
             * R3_l = (3 / 5) * (P3 - P0) and
             * R0_r = (9 / 5) * (P3 - P0)
             */
            *tcur_rightTan = (1. / 3.) * (tnext - tcur);
            *vcur_rightTan = (9. / 5.) * (vnext - vcur);
            
            *tcur_leftTan = (2. / 3.) * (tcur - tprev);
            *vcur_leftTan = (3. / 5.) * (vcur - vprev);
        }else if(interp == CATMULL_ROM || interp == SMOOTH){
            /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the tangents*/
            *tcur_rightTan = (1. / 3.) * (tnext - tcur);
            *vcur_rightTan = (vnext - vprev) / (tnext - tprev);

            *tcur_leftTan = (2. / 3.) * (tcur - tprev);
            *vcur_leftTan = std::abs((vprev - vnext) / (tnext - tprev));
            
            if(interp == SMOOTH){
                /*Now that we have the tangent by catmull-rom's formula, we compute the bezier
                 point on the left and on the right from the tangents (i.e: P1 and Q2, Q being the segment before P)
                 */
                T p1 = vcur + *vcur_rightTan / 3.;
                T q2 = vcur - *vcur_leftTan / 3.;
                
                /*We clamp Q2 to Q0(aka vprev) and Q3(aka vcur)
                 and P1 to P0(aka vcur) and P3(aka vnext)*/
                
                if(q2 < vprev)
                    q2 = vprev;
                if(q2 > vcur)
                    q2 = vcur;
                if(p1 < vcur)
                    p1 = vcur;
                if(p1 > vnext)
                    p1 = vnext;
                
                /*We recompute the tangents from the new clamped control points*/
                
                *vcur_rightTan = 3. * (p1 - vcur);
                *vcur_leftTan = 3. * (vcur - q2);
                
                /*And set the tangent on the left and on the right to be the minimum
                 of the 2.*/
                
                T min = std::min(*vcur_rightTan,*vcur_leftTan);
                *vcur_rightTan = min;
                *vcur_leftTan = min;
            }
            
        }else if(interp == HORIZONTAL){
            /*The values are the same than the keyframe they belong*/
            
            *tcur_rightTan = (1. / 3.) * (tnext - tcur);
            *vcur_rightTan = vcur;
            
            *tcur_leftTan = (2. / 3.) * (tcur - tprev);
            *vcur_leftTan = vcur;
        }else if(interp == CUBIC){
            //to do
        }
        
    }
    
}



#endif // INTERPOLATION_H
