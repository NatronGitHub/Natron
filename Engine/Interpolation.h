
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
    
    enum Type{
        CONSTANT = 0,
        LINEAR = 1,
        SMOOTH = 2,
        CATMULL_ROM = 3,
        CUBIC = 4,
        HORIZONTAL = 5,
        FREE = 6,
        BROKEN = 7,
        NONE = 8
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
                  Type interp){
        
        double tsquare = currentTime * currentTime;
        double tcube = tsquare * currentTime;
        switch(interp){
                
            case LINEAR:
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

    template <typename T>
    void autoComputeTangents(Natron::Type interpPrev,
                             Natron::Type interp,
                             Natron::Type interpNext,
                             double tprev,const T vprev, // vprev = Q0
                             double tcur,const T vcur, // vcur = Q3 = P0
                             double tnext,const T vnext, // vnext = P3
                             const T vprevDerivRight, // Q0'_r
                             const T vnextDerivLeft, // P3'_l
                             T* vcurDerivLeft, // Q3'_l
                             T* vcurDerivRight){ // P0'_r
        if(interp == LINEAR){
            /* Linear means the the 2nd derivative of the cubic curve at the point 'cur' is null.
             * That means
             * B''(0) = Q''(1) = 0
             * We have 2 equations:
             *          6(P3 - P0 - P3'_l / 3 - 2P0'_r / 3) = 0 (1)
             *          6(Q0 - Q3 + 2Q3'_l / 3 - Q0'_r / 3) = 0 (2)
             *
             *
             * Continuity yields Q3 = P0
             *
             * P0'_r = 3 / 2 ( P3 - P0 - P3'_l / 3)
             * Q3'_l = 3 / 2 ( Q0'_r / 3 + Q3 - Q0 )
             */

            if(interpNext == LINEAR){
                *vcurDerivRight = (vnext - vcur);
            }else{
                *vcurDerivRight = 1.5 * ( vnext - vcur - vnextDerivLeft / 3.);
            }

            if(interpPrev == LINEAR){
                *vcurDerivLeft = (vcur - vprev);
            }else{
                *vcurDerivLeft = 1.5 * ( vprevDerivRight / 3. + vcur - vprev);
            }

        }else if(interp == CATMULL_ROM || interp == SMOOTH){
            /* http://en.wikipedia.org/wiki/Cubic_Hermite_spline We use the formula given to compute the tangents*/
            T deriv = (vnext - vprev) / (tnext - tprev);
            *vcurDerivRight = deriv * (tnext - tcur);
            *vcurDerivLeft = deriv * (tcur - tprev);
            
            if(interp == SMOOTH){
                /*Now that we have the tangent by catmull-rom's formula, we compute the bezier
                 point on the left and on the right from the tangents (i.e: P1 and Q2, Q being the segment before P)
                 */
                T p1 = vcur + *vcurDerivRight / 3.;
                T q2 = vcur - *vcurDerivLeft / 3.;
                
                /*We clamp Q2 to Q0(aka vprev) and Q3(aka vcur)
                 and P1 to P0(aka vcur) and P3(aka vnext)*/
                T prevMax = std::max(vprev,vcur);
                T prevMin = std::min(vprev,vcur);
                q2 = std::max(prevMin,std::min(q2,prevMax));

                T nextMax = std::max(vcur,vnext);
                T nextMin = std::min(vcur,vnext);
                p1 = std::max(nextMin,std::min(p1,nextMax));
                
                /*We recompute the tangents from the new clamped control points*/
                
                *vcurDerivRight = 3. * (p1 - vcur);
                *vcurDerivLeft = 3. * (vcur - q2);
                
                /*And set the tangent on the left and on the right to be the minimum
                 of the 2.*/
                

                if(std::abs(*vcurDerivLeft) < std::abs(*vcurDerivRight)){
                    *vcurDerivRight = *vcurDerivLeft;
                }else{
                    *vcurDerivLeft = *vcurDerivRight;
                }

            }
            
        }else if(interp == HORIZONTAL || interp == CONSTANT){
            /*The values are the same than the keyframe they belong*/        
            *vcurDerivRight = 0.;
            *vcurDerivLeft = 0.;
        }else if(interp == CUBIC){
            /* Cubic means the the 2nd derivative of the cubic curve at the point 'cur' are equal.
             * That means
             * B''(0)/(tnext - tcur) = Q''(1)/ (tcur - tprev)
             * We have 2 equations:
             *          P3 - P0 - P3'_l / 3 - 2P0'_r / 3 = Q0 - Q3 + 2Q3'_l / 3 - Q0'_r / 3  (1)
             *
             *
             * Continuity yields Q3 = P0
             *
             * P0'_r / (tnext - tcur) = Q3'_l / (tcur - tprev)
             *
             */

            double timeRatio = (tcur - tprev) / (tnext - tcur);

            if(interpPrev == LINEAR && interpNext == LINEAR){
                /** we have two more unknowns, Q0'_r and P3'_l, and two more equations:
                 *        Q"(0) = 6(Q3 - Q0 - Q3'_l / 3 - (2 / 3) * Q0'_r) = 0
                 *        P''(1) = 6(P0 - P3 + (2 / 3) * P3'_l + P0'_r / 3) = 0
                 */
            }else if(interpPrev == LINEAR){
                /** we have one more unknown, Q0'_r, and one more equation:
                 *        Q"(0) = 6(Q3 - Q0 - Q3'_l / 3 - (2 / 3) * Q0'_r) = 0
                 *
                 *  which yields Q0'_r = 3 / 2 ( Q3 - Q0 - Q3'_l /3)
                 *
                 * We inject Q0'_r in equation (1) :
                 *
                 * P3 - P0 - P3'_l /3 - 2P0'_r / 3 = Q0 - Q3 + 2Q3'_l /3 - (1 /2) * (Q3 - Q0 - Q3'_l / 3)
                 *
                 * P3 - P0 - P3'_l /3 - 2P0'_r / 3 = (3 / 2) * Q0 - (3 / 2) * Q3 + (5 / 6) * Q3'_l
                 *
                 * P3 - P0 - P3'_l /3 - 2P0'_r / 3 = (3 / 2) * Q0 - (3 / 2) * P0 + (5 / 6) * P0'_r * timeRatio
                 *
                 * P0'_r = (P3 - P0 - P3'_l / 3 - (3 / 2) * (Q0 - P0)) / ((5 / 6) * timeRatio + 2 / 3 )
                 **/

                *vcurDerivRight = ( 3. *(vnext - vcur) - vnextDerivLeft - 4.5 * (vprev - vcur)) / (2.5 * timeRatio + 2.);
                *vcurDerivLeft = timeRatio * *vcurDerivRight;

            }else if(interpNext == LINEAR){
                /** we have one more unknown, P3'_l, and one more equation:
                 *        P''(1) = 6(P0 - P3 + (2 / 3) * P3'_l + P0'_r / 3) = 0
                 *
                 *  which yields P3'_l = (3 / 2) * (P3 - P0 - P0'_r / 3)
                 *
                 * We inject P3'_l in equation (1) :
                 *
                 * P3 - P0 - (1 / 2) * (P3 - P0 - P0'_r / 3) - 2P0'_r / 3 = Q0 - Q3 + (2/3) Q3'_l - Q0'_r / 3
                 *
                 *
                 * (1 / 2) (P3 - P0) - (1 / 2) * P0'_r  = Q0 - Q3 + (2/3) Q3'_l - Q0'_r / 3
                 *
                 * (1 / 2) (P3 - P0 - P0'_r)  = Q0 - Q3 + (2/3) Q3'_l - Q0'_r / 3
                 *
                 * (1 / 2) (P3 - P0 - P0'_r)  = Q0 - Q3 + (2/3) P0'_r * timeRatio - Q0'_r / 3
                 *
                 * PO'_r = ((1 / 2) ( P3 - P0 ) - Q0 + Q3 + Q0'_r / 3) / ((2 / 3) * timeRatio + 1 / 2)
                 **/

            }else{
                *vcurDerivRight = (1.5 * (vnext - vnextDerivLeft / 3. - vprev + vprevDerivRight / 3.)) / (timeRatio + 1.);
                *vcurDerivLeft = timeRatio * *vcurDerivRight;
            }


        }
        
    }
    
}



#endif // INTERPOLATION_H
