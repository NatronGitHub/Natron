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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "FitCurve.h"

#include <cmath>

#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif

using namespace FitCurve;
using namespace Natron;

/*
 * This implementation is based on Graphic Gems I
 */

namespace {
    
static double euclideanDistance(const Point& a, const Point& b)
{
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}
   
static double dotProduct(const Point& a, const Point& b)
{
   return (a.x * b.x) + (a.y * b.y);
}
    
/**
 * @brief computeEndTangent
 * Approximate unit tangents at endpoints of digitized curve
 * On the left it should be called with points 0 and 1 (left tangent)
 * On the right it should be called with points N and N -1 (right tangent)
 **/
static Point computeEndTangent(const Point& first, const Point& second)
{
    Point ret;
    ret.x = second.x - first.x;
    ret.y = second.y - first.y;
    double dist = std::sqrt(ret.x * ret.x + ret.y * ret.y);
    if (dist != 0.) {
        ret.x /= dist;
        ret.y /= dist;
    }
    return ret;
}
    
/**
* @brief computeCenterTangent
 * Approximate unit tangents at the center of digitized curve
**/
static Point computeCenterTangent(const Point& prev,const Point& center,const Point& next) {
    Point v1,v2,ret;
    v1.x = prev.x - center.x;
    v1.y = prev.y - center.y;
    v2.x = center.x - next.x;
    v2.y = center.y - next.y;
    ret.x = (v1.x + v2.x) / 2.;
    ret.y = (v1.y + v2.y) / 2.;
    double dist = std::sqrt(ret.x * ret.x + ret.y * ret.y);
    if (dist != 0.) {
        ret.x /= dist;
        ret.y /= dist;
    }

    return ret;
}
    
/**
  *  @brief Evaluate a Bezier curve at a particular parameter value
  *
 **/
static Point bezierEval(int degree, const std::vector<Point>& v, double t)
{
    std::vector<Point> 	vtemp(v.size());		/* Local copy of control points		*/
    assert((int)v.size() == degree + 1);
    
    for (int i = 0; i <= degree; ++i) {
        vtemp[i] = v[i];
    }
    
    // Triangle computation
    for (int i = 1; i <= degree; ++i) {
        for (int j = 0; j <= degree-i; ++j) {
            vtemp[j].x = (1.0 - t) * vtemp[j].x + t * vtemp[j+1].x;
            vtemp[j].y = (1.0 - t) * vtemp[j].y + t * vtemp[j+1].y;
        }
    }
    return vtemp[0];
}
    
/**
 * @brief Use Newton-Raphson iteration to find better root.
 **/
static double newtonRaphsonRootFind(const std::vector<Point>& Q, const Point& P, double u)
{
    std::vector<Point> Q1(3); // Q'
    std::vector<Point> Q2(2); // Q''
    Point Q_u, Q1_u, Q2_u; /*u evaluated at Q, Q', & Q''	*/
    
    // Compute Q(u)
    Q_u = bezierEval(3, Q, u);
    
    // Generate control vertices for Q'
    for (int i = 0; i < 3; ++i) {
        Q1[i].x = (Q[i+1].x - Q[i].x) * 3.0;
        Q1[i].y = (Q[i+1].y - Q[i].y) * 3.0;
    }
    
    // Generate control vertices for Q''
    for (int i = 0; i < 2; ++i) {
        Q2[i].x = (Q1[i+1].x - Q1[i].x) * 2.0;
        Q2[i].y = (Q1[i+1].y - Q1[i].y) * 2.0;
    }
    
    // Compute Q'(u) and Q''(u)
    Q1_u = bezierEval(2, Q1, u);
    Q2_u = bezierEval(1, Q2, u);
    
    // Compute f(u)/f'(u)
    double numerator = (Q_u.x - P.x) * (Q1_u.x) + (Q_u.y - P.y) * (Q1_u.y);
    double denominator = (Q1_u.x) * (Q1_u.x) + (Q1_u.y) * (Q1_u.y) +
    (Q_u.x - P.x) * (Q2_u.x) + (Q_u.y - P.y) * (Q2_u.y);
    if (denominator == 0.0f) {
        return u;
    }
    
    // u = u - f(u)/f'(u)
    return u - (numerator/denominator);
}
    
/**
 * @brief Given set of points and their parameterization, try to find
 * a better parameterization.
 *
 **/
static void reparameterize(const std::vector<Point> &points, const std::vector<SimpleBezierCP>& bezCurve,  std::vector<double>& u)
{
    std::vector<Point> bezSeg;
    bezSeg.push_back(bezCurve[0].p);
    bezSeg.push_back(bezCurve[0].rightTan);
    bezSeg.push_back(bezCurve[1].leftTan);
    bezSeg.push_back(bezCurve[1].p);

    for (std::size_t i = 0; i < u.size(); ++i) {
        u[i] = newtonRaphsonRootFind(bezSeg, points[i], u[i]);
    }
}
    
/**
  *	Find the maximum squared distance of digitized points
  *	to fitted curve.
 **/
static double computeMaxError(const std::vector<Point>& points,const std::vector<SimpleBezierCP>& bezierCurve,
                                  const std::vector<double>& u, int* splitPoint)
{
    *splitPoint = points.size() / 2;
    double maxDist = 0.0;
    std::vector<Point> bezierSegment;
    bezierSegment.push_back(bezierCurve[0].p);
    bezierSegment.push_back(bezierCurve[0].rightTan);
    bezierSegment.push_back(bezierCurve[1].leftTan);
    bezierSegment.push_back(bezierCurve[1].p);
    for (std::size_t i = 1; i < points.size() - 1; ++i) {
        Point p = bezierEval(3, bezierSegment, u[i]);
        Point v;
        v.x = p.x - points[i].x;
        v.y = p.y - points[i].y;
        double distSq = v.x * v.x + v.y * v.y;
        if (distSq >= maxDist) {
            maxDist = distSq;
            *splitPoint = i;
        }
    }
    return maxDist;
}
 
/**
 *  ChordLengthParameterize :
 *	Assign parameter values to digitized points
 *	using relative distances between points.
**/
static void chordLengthParametrize(const std::vector<Point>& points,std::vector<double>* u)
{
    assert(points.size() > 1);
    u->push_back(0.);
    std::vector<Point>::const_iterator prev = points.begin();
    std::vector<Point>::const_iterator cur = points.begin();
    ++cur;
    
    for (; cur != points.end(); ++prev, ++cur) {
        double last = u->back();
        double dist = euclideanDistance(*cur, *prev);
        u->push_back(last + dist);
    }
    assert(u->size() == points.size());
    double last = u->back();
    assert(last != 0.);
    
    std::vector<double>::iterator it = u->begin();
    ++it;
    for (; it != u->end(); ++it) {
        *it /= last;
    }
}
    
/**
 * @brief Bezier multipliers
 **/
static double Bezier0(double u)
{
        double tmp = 1.0 - u;
        return (tmp * tmp * tmp);
}
    
    
static double Bezier1(double u)
{
        double tmp = 1.0 - u;
        return (3 * u * (tmp * tmp));
}
    
static double Bezier2(double u)
{
        double tmp = 1.0 - u;
        return (3 * u * u * tmp);
}
    
static double Bezier3(double u)
{
        return (u * u * u);
}
    
    
static void generateBezier(const std::vector<Point>& points, const std::vector<double>& u, const Point& tHat1, const Point& tHat2,
                           std::vector<SimpleBezierCP>* generatedBezier)
{
    assert(points.size() == u.size());
    
    std::vector<std::pair<Point,Point> > a(points.size()); // Precomputed rhs for eqn
    for (std::size_t i = 0; i < points.size(); ++i) {
        Point v1,v2;
        v1 = tHat1;
        v2 = tHat2;
        v1.x *= Bezier1(u[i]);
        v1.y *= Bezier1(u[i]);
        
        v2.x *= Bezier2(u[i]);
        v2.y *= Bezier2(u[i]);
        a[i].first = v1;
        a[i].second = v2;
    }
    
    assert(a.size() == points.size());
    
    double c[2][2]; // Matrix C
    c[0][0] = c[0][1] = c[1][0] = c[1][1] = 0.0;
    double x[2]; // Matrix X
    x[0] = x[1] = 0.;
    
    Point tmp;
    for (std::size_t i = 0; i < points.size(); ++i) {
        c[0][0] += dotProduct(a[i].first, a[i].first);
        c[0][1] += dotProduct(a[i].first, a[i].second);
        c[1][0] += c[0][1];
        c[1][1] += dotProduct(a[i].second, a[i].second);
        tmp = points[i];
        
        Point firstBezier0 = points[0];
        firstBezier0.x *= Bezier0(u[i]);
        firstBezier0.y *= Bezier0(u[i]);
        
        Point firstBezier1 = points[0];
        firstBezier1.x *= Bezier1(u[i]);
        firstBezier1.y *= Bezier1(u[i]);
        
        Point lastBezier3 = points[points.size() - 1];
        lastBezier3.x *= Bezier3(u[i]);
        lastBezier3.y *= Bezier3(u[i]);
        
        Point lastBezier2 = points[points.size() - 1];
        lastBezier2.x *= Bezier2(u[i]);
        lastBezier2.y *= Bezier2(u[i]);
        
        tmp.x -= (firstBezier0.x + firstBezier1.x +  lastBezier2.x + lastBezier3.x);
        tmp.y -= (firstBezier0.y + firstBezier1.y +  lastBezier2.y + lastBezier3.y);
        
        x[0] += dotProduct(a[i].first, tmp);
        x[1] += dotProduct(a[i].second, tmp);
    }
    
    //Compute the determinants of C and X
    double det_c0_c1 = c[0][0] * c[1][1] - c[1][0] * c[0][1];
    double det_c0_x  = c[0][0] * x[1] -    c[1][0] * x[0];
    double det_x_C1  = x[0] *    c[1][1] - x[1] *    c[0][1];
    
    // Finally, derive alpha values
    double alpha_l = (det_c0_c1 == 0) ? 0.0 : det_x_C1 / det_c0_c1;
    double alpha_r = (det_c0_c1 == 0) ? 0.0 : det_c0_x / det_c0_c1;
    
    // If alpha negative, use the Wu/Barsky heuristic (see text)
    // (if alpha is 0, you get coincident control points that lead to
    // divide by zero in any subsequent NewtonRaphsonRootFind() call.
    double segLength = euclideanDistance(points[points.size() - 1], points[0]);
    double epsilon = 1.0e-6 * segLength;
    if (alpha_l < epsilon || alpha_r < epsilon) {
        // fall back on standard (probably inaccurate) formula, and subdivide further if needed.
        double dist = segLength / 3.0;
        alpha_l = alpha_r = dist;
    }
    
    // First and last control points of the Bezier curve are
    //  positioned exactly at the first and last data points
    //  Control points 1 and 2 are positioned an alpha distance out
    //  on the tangent vectors, left and right, respectively
    SimpleBezierCP firstCp,lastCp;
    firstCp.p = points[0];
    firstCp.leftTan = firstCp.p;
    lastCp.p = points[points.size() - 1];
    lastCp.rightTan = lastCp.p;
    firstCp.rightTan.x = firstCp.p.x + tHat1.x * alpha_l;
    firstCp.rightTan.y = firstCp.p.y + tHat1.y * alpha_l;
    lastCp.leftTan.x = lastCp.p.x + tHat2.x * alpha_r;
    lastCp.leftTan.y = lastCp.p.y + tHat2.y * alpha_r;
    generatedBezier->push_back(firstCp);
    generatedBezier->push_back(lastCp);
    
}
    
static void fit_cubic_internal(const std::vector<Point>& points, const Point&  tHat1, const Point& tHat2, double error,
                               std::vector<SimpleBezierCP>* generatedBezier)
{
    //Error below which you try iterating
    double iterationError = error * error;
    ;
    int maxIterations = 4;
    
    //Use heuristic if the region only has 2 points
    if (points.size() == 2) {
        const Point& first = points.front();
        const Point& last = points.back();
        double dist =  euclideanDistance(first, last);
        SimpleBezierCP firstCp,lastCp;
        firstCp.p = first;
        firstCp.leftTan = first;
        firstCp.rightTan.x = firstCp.p.x + tHat1.x * dist / 3.;
        firstCp.rightTan.y = firstCp.p.y + tHat1.y * dist / 3.;
        lastCp.p = last;
        lastCp.rightTan = last;
        lastCp.leftTan.x = lastCp.p.x + tHat2.x * dist / 3.;
        lastCp.leftTan.y = lastCp.p.y + tHat2.y * dist / 3.;
        generatedBezier->push_back(firstCp);
        generatedBezier->push_back(lastCp);
        return;
    }
    
    // Parameterize points, and attempt to fit curve
    std::vector<double> u;
    chordLengthParametrize(points, &u);
    generateBezier(points, u, tHat1, tHat2, generatedBezier);
    
    int splitPoint;
    double maxError = computeMaxError(points, *generatedBezier, u, &splitPoint);
    
    //  If error not too large, try some reparameterization and iteration
    if (maxError < iterationError) {
        for (int i = 0; i < maxIterations; ++i) {
            reparameterize(points, *generatedBezier, u);
            generatedBezier->clear();
            generateBezier(points, u, tHat1, tHat2, generatedBezier);
            maxError = computeMaxError(points, *generatedBezier, u, &splitPoint);
            if (maxError < error) {
                return;
            }
        }
    }

    assert(splitPoint >= 1 && splitPoint < (int)points.size() - 1);
    // Fitting failed -- split at max error point and fit recursively
    Point tHatCenter = computeCenterTangent(points[splitPoint - 1], points[splitPoint], points[splitPoint + 1]);
    
    std::vector<Point> firstSplit,secondSplit;
    for (std::size_t i = 0; i <= (std::size_t)splitPoint; ++i) {
        firstSplit.push_back(points[i]);
    }
    for (std::size_t i = (std::size_t)splitPoint; i < points.size(); ++i) {
        secondSplit.push_back(points[i]);
    }
    
    std::vector<SimpleBezierCP> first,second;
    fit_cubic_internal(firstSplit, tHat1, tHatCenter, error, &first);
    tHatCenter.x = -tHatCenter.x;
    tHatCenter.y = -tHatCenter.y;
    fit_cubic_internal(secondSplit, tHatCenter, tHat2, error, &second);
    generatedBezier->clear();
    if (!first.empty()) {
        generatedBezier->insert(generatedBezier->end(), first.begin(), first.end());
    }
    
    for (std::vector<SimpleBezierCP>::iterator it = second.begin(); it!= second.end();++it) {
        
        /*
         * We already inserted the last control point of the "first" segment which should be equal to the first control
         * point of our segment, so give it the left tangent computed in the "first" segment and the right tangent
         * computed in the "second" segment
         */
        if (it == second.begin()) {
            if (!generatedBezier->empty()) {
                generatedBezier->back().rightTan = it->rightTan;
            }
        } else {
            generatedBezier->push_back(*it);
        }
    }

}
    
static void fit_cubic_for_sub_set(const std::vector<Point>& points,double error,std::vector<SimpleBezierCP>* generatedBezier)
{
    if (points.size() == 1) {
        SimpleBezierCP cp;
        cp.p = points.front();
        cp.leftTan = cp.rightTan = cp.p;
        generatedBezier->push_back(cp);
        return;
    }
    Point tHat1 = computeEndTangent(points[0], points[1]);
    Point tHat2 = computeEndTangent(points[points.size() - 1], points[points.size() - 2]);
    fit_cubic_internal(points, tHat1, tHat2, error, generatedBezier);
}
    
} // anon namespace

void FitCurve::fit_cubic(const std::vector<Point>& points, double error,std::vector<SimpleBezierCP>* generatedBezier)
{
    if (points.size() < 2) {
        return;
    }
    
    //First remove (almost) duplicate points
    std::list<Point> newPoints;
    for (std::vector<Point>::const_iterator it = points.begin(); it!=points.end(); ++it) {
        
        bool foundDuplicate = false;
        for (std::list<Point>::const_iterator it2 = newPoints.begin(); it2!=newPoints.end(); ++it2) {
            if (std::abs(it2->x - it->x) < 1e-4 && std::abs(it2->y - it->y) < 1e-4) {
                foundDuplicate = true;
                break;
            }
        }
        if (!foundDuplicate) {
            newPoints.push_back(*it);
        }
    }
    
    //Divide the original points by identifying "corners": points where the angle between the previous, current and next point
    //creates a discontinuity
    std::list<std::vector<Point> > pointSets;
    
    
    bool foundCorner;
    
    do {
        foundCorner = false;
        if (newPoints.size() <= 2) {
            break;
        }
        std::list<Point>::iterator prev = newPoints.begin();
        std::list<Point>::iterator it = newPoints.begin();
        ++it;
        std::list<Point>::iterator next = it;
        if (next != newPoints.end()) {
            ++next;
        }
        for (; it != newPoints.end(); ++prev, ++it, ++next) {
            if (it == newPoints.end()) {
                it = newPoints.begin();
            }
            if (next == newPoints.end()) {
                break;
                //next = newPoints.begin();
            }
            
            Point u,v;
            u.x = it->x - prev->x;
            u.y = it->y - prev->y;
            
            v.x = next->x - it->x;
            v.y = next->y - it->y;
            
            double distU = std::sqrt(u.x * u.x + u.y * u.y);
            double distV = std::sqrt(v.x * v.x + v.y * v.y);
            assert(distV != 0);
            double alpha = std::acos(distU / distV);
            if (alpha > M_PI_2) {
                std::vector<Point> subset;
                
                for (std::list<Point>::iterator it2 = newPoints.begin(); it2!=next; ++it2) {
                    subset.push_back(*it2);
                }
                newPoints.erase(newPoints.begin(), next);
                
                //If only a single point remains, just add it to this bezier curve
                if (newPoints.size() == 1) {
                    subset.push_back(newPoints.front());
                    newPoints.clear();
                }
                pointSets.push_back(subset);
                foundCorner = true;
                break;
            }
        }
    } while (foundCorner);
    if (!newPoints.empty()) {
        std::vector<Point> subset;
        for (std::list<Point>::iterator it2 = newPoints.begin(); it2!=newPoints.end(); ++it2) {
            subset.push_back(*it2);
        }
        pointSets.push_back(subset);
    }
    
    
    for (std::list<std::vector<Point> >::iterator it = pointSets.begin(); it!=pointSets.end(); ++it) {
        std::vector<SimpleBezierCP> subsetBezier;
        fit_cubic_for_sub_set(*it, error, &subsetBezier);
        for (std::size_t i = 0; i < subsetBezier.size(); ++i) {
            
            //For the first segment point check if the  point is not already inserted in generatedBezie
            bool found = false;
            if (i == 0) {
                for (std::vector<SimpleBezierCP>::iterator it2 = generatedBezier->begin(); it2!=generatedBezier->end(); ++it2) {
                    if (std::abs(it2->p.x - subsetBezier[i].p.x) < 1e-6 && std::abs(it2->p.y - subsetBezier[i].p.y) < 1e-6) {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                generatedBezier->push_back(subsetBezier[i]);
            }
        }
    }
}

