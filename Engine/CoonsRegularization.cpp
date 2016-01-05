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

/*****************************************************************************/
/*                                                                           */
/*  Routines for Arbitrary Precision Floating-point Arithmetic               */
/*  and Fast Robust Geometric Predicates                                     */
/*  (predicates.c)                                                           */
/*                                                                           */
/*  May 18, 1996                                                             */
/*                                                                           */
/*  Placed in the public domain by                                           */
/*  Jonathan Richard Shewchuk                                                */
/*  School of Computer Science                                               */
/*  Carnegie Mellon University                                               */
/*  5000 Forbes Avenue                                                       */
/*  Pittsburgh, Pennsylvania  15213-3891                                     */
/*  jrs@cs.cmu.edu                                                           */
/*                                                                           */
/*  This file contains C implementation of algorithms for exact addition     */
/*    and multiplication of floating-point numbers, and predicates for       */
/*    robustly performing the orientation and incircle tests used in         */
/*    computational geometry.  The algorithms and underlying theory are      */
/*    described in Jonathan Richard Shewchuk.  "Adaptive Precision Floating- */
/*    Point Arithmetic and Fast Robust Geometric Predicates."  Technical     */
/*    Report CMU-CS-96-140, School of Computer Science, Carnegie Mellon      */
/*    University, Pittsburgh, Pennsylvania, May 1996.  (Submitted to         */
/*    Discrete & Computational Geometry.)                                    */
/*                                                                           */
/*  This file, the paper listed above, and other information are available   */
/*    from the Web page http://www.cs.cmu.edu/~quake/robust.html .           */
/*                                                                           */
/*****************************************************************************/

#include "CoonsRegularization.h"

#include <limits>
#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

#include "Engine/Bezier.h"
#include "Engine/BezierCP.h"
#include "Engine/Interpolation.h"
#include "Engine/RectD.h"
#include "Engine/Transform.h"

using namespace Natron;

#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif


static Point getPointAt(const BezierCPs& cps, double time, double t)
{
    int ncps = (int)cps.size();
    assert(ncps);
    if (!ncps) {
        throw std::logic_error("getPointAt()");
    }
    if (t < 0) {
        t += ncps;
    }
    int t_i = (int)std::floor(t) % ncps;
    int t_i_plus_1 = t_i % ncps;
    assert(t_i >= 0 && t_i < ncps && t_i_plus_1 >= 0 && t_i_plus_1 < ncps);
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getPositionAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getPositionAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    }
    
    BezierCPs::const_iterator it = cps.begin();
    std::advance(it, t_i);
    BezierCPs::const_iterator next = it;
    ++next;
    if (next == cps.end()) {
        next = cps.begin();
    }
    Point p0,p1,p2,p3;
    (*it)->getPositionAtTime(false ,time, &p0.x, &p0.y);
    (*it)->getRightBezierPointAtTime(false ,time, &p1.x, &p1.y);
    (*next)->getLeftBezierPointAtTime(false ,time, &p2.x, &p2.y);
    (*next)->getPositionAtTime(false ,time, &p3.x, &p3.y);
    Point ret;
    Bezier::bezierPoint(p0, p1, p2, p3, t - t_i, &ret);
    return ret;
}

static Point getLeftPointAt(const BezierCPs& cps, double time, double t)
{
    int ncps = (int)cps.size();
    assert(ncps);
    if (!ncps) {
        throw std::logic_error("getLeftPointAt()");
    }
    if (t < 0) {
        t += ncps;
    }
    int t_i = (int)std::floor(t) % ncps;
    int t_i_plus_1 = t_i % ncps;
    assert(t_i >= 0 && t_i < ncps && t_i_plus_1 >= 0 && t_i_plus_1 < ncps);
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getLeftBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getLeftBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    }
    
    BezierCPs::const_iterator it = cps.begin();
    std::advance(it, t_i);
    BezierCPs::const_iterator next = it;
    ++next;
    if (next == cps.end()) {
        next = cps.begin();
    }
    
    t = t - t_i;
    
    Point a,b,c,ab,bc,abc;
    Point ret;
    (*it)->getPositionAtTime(false ,time, &a.x, &a.y);
    (*it)->getRightBezierPointAtTime(false ,time, &b.x, &b.y);
    (*next)->getLeftBezierPointAtTime(false ,time, &c.x, &c.y);
    ab.x = (1. - t) * a.x + t * b.x;
    ab.y = (1. - t) * a.y + t * b.y;
    
    bc.x = (1. - t) * b.x + t * c.x;
    bc.y = (1. - t) * b.y + t * c.y;
    
    abc.x = (1. - t) * ab.x + t * bc.x;
    abc.y = (1. - t) * ab.y + t * bc.y;
    if (abc.x == a.x && abc.y == a.y) {
        (*it)->getLeftBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    } else {
        return abc;
    }
}

static Point getRightPointAt(const BezierCPs& cps, double time, double t)
{
    int ncps = cps.size();
    assert(ncps);
    if (!ncps) {
        throw std::logic_error("getRightPointAt()");
    }
    if (t < 0) {
        t += ncps;
    }
    int t_i = (int)std::floor(t) % ncps;
    int t_i_plus_1 = t_i % ncps;
    assert(t_i >= 0 && t_i < ncps && t_i_plus_1 >= 0 && t_i_plus_1 < ncps);
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getRightBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getRightBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    }
    
    BezierCPs::const_iterator it = cps.begin();
    std::advance(it, t_i);
    BezierCPs::const_iterator next = it;
    ++next;
    if (next == cps.end()) {
        next = cps.begin();
    }
    
    t = t - t_i;
    
    Point a,b,c,ab,bc,abc;
    Point ret;
    (*it)->getRightBezierPointAtTime(false ,time, &a.x, &a.y);
    (*next)->getLeftBezierPointAtTime(false ,time, &b.x, &b.y);
    (*next)->getPositionAtTime(false ,time, &c.x, &c.y);
    ab.x = (1. - t) * a.x + t * b.x;
    ab.y = (1. - t) * a.y + t * b.y;
    
    bc.x = (1. - t) * b.x + t * c.x;
    bc.y = (1. - t) * b.y + t * c.y;
    
    abc.x = (1. - t) * ab.x + t * bc.x;
    abc.y = (1. - t) * ab.y + t * bc.y;
    if (abc.x == c.x && abc.y == c.y) {
        (*next)->getRightBezierPointAtTime(false ,time, &ret.x, &ret.y);
        return ret;
    } else {
        return abc;
    }
}

static double norm(const Point& z0, const Point& c0, const Point& c1, const Point& z1)
{
    double fuzz2 = 1000. * std::numeric_limits<double>::epsilon() * 1000. * std::numeric_limits<double>::epsilon();
    double p1_p1p2Norm = (c0.x - z0.x) * (c0.x - z0.x) + (c0.y - z0.y) * (c0.y - z0.y);
    double p1_p2p1Norm = (c1.x - z0.x) * (c1.x - z0.x) + (c1.y - z0.y) * (c1.y - z0.y);
    double p1_p2Norm = (z1.x - z0.x) * (z1.x - z0.x) + (z1.y - z0.y) * (z1.y - z0.y);
    return fuzz2 * std::max(p1_p1p2Norm,std::max(p1_p2p1Norm,p1_p2Norm));
}

static Point predir(const BezierCPs& cps, double time, double t)
{
    //Compute the unit vector in the direction of the bysector angle
    Point dir;
    Point p1,p2,p2p1,p1p2;
    p2 = getPointAt(cps, time, t);
    p2p1 = getLeftPointAt(cps, time, t);
    p1p2 = getRightPointAt(cps, time, t - 1);
    p1 = getPointAt(cps, time, t - 1);
    dir.x =  3. * (p2.x - p2p1.x);
    dir.y =  3. * (p2.y - p2p1.y);
    
    double epsilon = norm(p1, p1p2, p2p1, p2);
    
    double predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    if (predirNormSquared > epsilon) {
        double normDir = std::sqrt(predirNormSquared);
        assert(normDir != 0);
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = 2. * p2p1.x - p1p2.x - p2.x;
    dir.y = 2. * p2p1.y - p1p2.y - p2.y;
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;

    if (predirNormSquared > epsilon) {
        double normDir = std::sqrt(predirNormSquared);
        assert(normDir != 0);
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p2.x - p1.x + 3. * (p1p2.x - p2p1.x);
    dir.y = p2.y - p1.y + 3. * (p1p2.y - p2p1.y);
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    double normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    Point ret;
    ret.x = dir.x / normDir;
    ret.y = dir.y / normDir;
    return ret;
}

static Point postdir(const BezierCPs& cps, double time, double t)
{
    Point dir;
    Point p2,p3,p2p3,p3p2;
    p2 = getPointAt(cps, time, t);
    p2p3 = getRightPointAt(cps, time, t);
    p3 = getPointAt(cps, time, t+1);
    p3p2 = getLeftPointAt(cps, time, t+1);
    dir.x = 3. * (p2p3.x - p2.x);
    dir.y = 3. * (p2p3.y - p2.y);
    double epsilon = norm(p2, p2p3, p3p2, p3);
    double predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    
    if (predirNormSquared > epsilon) {
        double normDir = std::sqrt(predirNormSquared);
        assert(normDir != 0);
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p2.x - 2 * p2p3.x + p3p2.x;
    dir.y = p2.y - 2 * p2p3.y + p3p2.y;
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    if (predirNormSquared > epsilon) {
        double normDir = std::sqrt(predirNormSquared);
        assert(normDir != 0);

        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p3.x - p2.x + 3. * (p2p3.x - p3p2.x);
    dir.y = p3.y - p2.y + 3. * (p2p3.y - p3p2.y);
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    double normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    Point ret;
    ret.x = dir.x / normDir;
    ret.y = dir.y / normDir;
    return ret;
}

static Point dirVect(const BezierCPs& cps, double time, double t, int sign)
{
    Point ret;
    if (sign == 0) {
        Point pre = predir(cps, time, t);
        Point post = postdir(cps, time, t);
        ret.x = pre.x + post.x;
        ret.y = pre.y + post.y;
    } else if (sign < 0) {
        ret = predir(cps, time, t);
    } else if (sign > 0) {
        ret = postdir(cps, time, t);
    }
    
    double norm = std::sqrt(ret.x * ret.x + ret.y * ret.y);
    assert(norm != 0);
    ret.x /= norm;
    ret.y /= norm;

    return ret;
}

static Point dirVect(const BezierCPs& cps, double time, double t)
{
    int t_i = std::floor(t);
    t -= t_i;
    if (t == 0) {
        Point pre = predir(cps,time ,t);
        
        Point post = postdir(cps, time ,t);
        
        Point ret;
        ret.x = pre.x + post.x;
        ret.y = pre.y + post.y;
        double norm = std::sqrt(ret.x * ret.x + ret.y * ret.y);
        if (norm == 0) {
            ret.x = ret.y = 0;
            return ret;
        }
        ret.x /= norm;
        ret.y /= norm;
        return ret;
    }
    Point z0 = getPointAt(cps, time, t_i);
    Point c0 = getRightPointAt(cps, time, t_i);
    Point c1 = getLeftPointAt(cps, time, t_i + 1);
    Point z1 = getPointAt(cps, time, t_i + 1);
    Point a,b,c;
    a.x = 3. * (z1.x - z0.x) + 9. * (c0.x - c1.x);
    a.y = 3. * (z1.y - z0.y) + 9. * (c0.y - c1.y);
    
    b.x = 6. * (z0.x + c1.x) - 12. * c0.x;
    b.y = 6. * (z0.y + c1.y) - 12. * c0.y;
    
    c.x = 3. * (c0.x - z0.x);
    c.y = 3. * (c0.y - z0.y);
    
    Point ret;
    ret.x = a.x * t * t + b.x * t + c.x;
    ret.y = a.y * t * t + b.y * t + c.y;
    double epsilon = norm(z0, c0, c1, z1);
    double dirNormSquared = ret.x * ret.x + ret.y * ret.y;
    double normDir = std::sqrt(dirNormSquared);
    assert(normDir != 0);
    if (dirNormSquared > epsilon) {
        ret.x /= normDir;
        ret.y /= normDir;
        return ret;
    }
    ret.x = 2. * a.x * t + b.x;
    ret.y = 2. * a.y * t + b.y;
    dirNormSquared = ret.x * ret.x + ret.y * ret.y;
    normDir = std::sqrt(dirNormSquared);
    assert(normDir != 0);
    if (dirNormSquared > epsilon) {
        ret.x /= normDir;
        ret.y /= normDir;
        return ret;
    }
    normDir = std::sqrt(a.x * a.x + a.y * a.y);
    assert(normDir != 0);
    a.x /= normDir;
    a.y /= normDir;
    return a;
}


static boost::shared_ptr<BezierCP> makeBezierCPFromPoint(const Point& p, const Point& left, const Point& right)
{
    boost::shared_ptr<BezierCP> ret(new BezierCP);
    ret->setStaticPosition(false ,p.x, p.y);
    ret->setLeftBezierStaticPosition(false ,left.x, left.y);
    ret->setRightBezierStaticPosition(false ,right.x, right.y);
    return ret;
}

static void findIntersection(const BezierCPs& cps,
                             double time,
                             const Point& p,
                             const Point& q,
                             boost::shared_ptr<BezierCP>* newPoint,
                             int* before)
{
    double fuzz = 1000. * std::numeric_limits<double>::epsilon();
    double fuzz2 = fuzz * fuzz;

    
    double dx = q.x - p.x;
    double dy = q.y - p.y;
    double det = p.y * q.x - p.x * q.y;
    std::vector<std::pair<boost::shared_ptr<BezierCP>,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > > intersections;
    
    BezierCPs::const_iterator s1 = cps.begin();
    BezierCPs::const_iterator s2 = cps.begin();
    ++s2;
    int index = 0;
    for (; s1 != cps.end(); ++s1,++s2, ++index) {
        if (s2 == cps.end()) {
            s2 = cps.begin();
        }
      
        Point z0,z1,c0,c1;
        (*s1)->getPositionAtTime(false ,time, &z0.x, &z0.y);
        (*s1)->getRightBezierPointAtTime(false ,time, &c0.x, &c0.y);
        (*s2)->getPositionAtTime(false ,time, &z1.x, &z1.y);
        (*s2)->getLeftBezierPointAtTime(false ,time, &c1.x, &c1.y);
        
        
        Point t3,t2,t1;
        t3.x = z1.x - z0.x + 3. * (c0.x - c1.x);
        t3.y = z1.y - z0.y + 3. * (c0.y - c1.y);
        
        t2.x = 3. * (z0.x + c1.x) - 6. * c0.x;
        t2.y = 3. * (z0.y + c1.y) - 6. * c0.y;
        
        t1.x = 3. * (c0.x - z0.x);
        t1.y = 3. * (c0.y - z0.y);
        
        double a = dy * t3.x - dx * t3.y;
        double b = dy * t2.x - dx * t2.y;
        double c = dy * t1.x - dx * t1.y;
        double d = dy * z0.x - dx * z0.y + det;
        
        std::vector<double> roots;
        if (std::max(std::max(std::max(a * a, b * b), c * c), d * d) >
            fuzz2 * std::max(std::max(std::max(z0.x * z0.x + z0.y * z0.y, z1.x * z1.x + z1.y * z1.y),
                                      c0.x * c0.x + c0.y * c0.y),
                             c1.x * c1.x + c1.y * c1.y)) {
                double r[3];
                int order[3];
                int nsols = Natron::solveCubic(d, c, b, a, r, order);
                for (int i = 0; i < nsols; ++i) {
                    roots.push_back(r[i]);
                }
            } else {
                roots.push_back(0);
            }
        
        for (std::size_t i = 0; i < roots.size(); ++i) {
            if (roots[i] >= -fuzz && roots[i] <= 1. + fuzz) {
                if (i + roots[i] >= roots.size() - fuzz) {
                    roots[i] = 0;
                }
                Point interP = getPointAt(cps, time, roots[i] + index);
                Point interLeft = getLeftPointAt(cps, time, roots[i] + index);
                Point interRight = getRightPointAt(cps, time, roots[i] + index);
                boost::shared_ptr<BezierCP> intersection = makeBezierCPFromPoint(interP,interLeft,interRight);
            
                double distToP = std::sqrt((p.x - interP.x) * (p.x - interP.x) + (p.y - interP.y) * (p.y - interP.y));
                if (std::abs(distToP) < 1e-4) {
                    continue;
                }
                
                bool found = false;
                for (std::vector<std::pair<boost::shared_ptr<BezierCP>,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > >::iterator it = intersections.begin(); it!=intersections.end(); ++it) {
                    Point other;
                    it->first->getPositionAtTime(false ,time, &other.x, &other.y);
                    double distSquared = (interP.x - other.x) * (interP.x - other.x) + (interP.y - other.y) * (interP.y - other.y);
                    if (distSquared <= fuzz2) {
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    intersections.push_back(std::make_pair(intersection, std::make_pair(s1, s2)));
                }
            }
        }
    }
    
    
    
    assert(intersections.size() >= 1);
    const std::pair<boost::shared_ptr<BezierCP>,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > & inter = intersections.front();
    *newPoint = inter.first;
    *before = std::distance(cps.begin(), inter.second.first);
}

static bool splitAt(const BezierCPs &cps, double time, double t, std::list<BezierCPs>* ret)
{
    Point dir = dirVect(cps, time, t);
    if (dir.x != 0. || dir.y != 0.) {
        Point z = getPointAt(cps, time, t);
        Point zLeft = getLeftPointAt(cps, time, t);
        Point zRight = getRightPointAt(cps, time, t);
        Point q;
        q.x = z.x;
        q.y = z.y + dir.y;
        boost::shared_ptr<BezierCP> newPoint;
        int pointIdx = -1;
        findIntersection(cps, time, z, q, &newPoint, &pointIdx);
        assert(pointIdx >= 0 && pointIdx < (int)cps.size());
        
        //Separate the original patch in 2 parts and call regularize again on each of them
        BezierCPs firstPart,secondPart;
        
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, std::ceil(t));
        /*
         "it" is now pointing to the next control point after the split point
         */
        
        BezierCPs::const_iterator start = it;
        
        BezierCPs::const_iterator end = cps.begin();
        /*
         "end" is the control point before the intersection point
         */
        if (pointIdx > 0) {
            std::advance(end, pointIdx);
        }
        
        
        boost::shared_ptr<BezierCP> startingPoint = makeBezierCPFromPoint(z, zLeft, zRight);
        
        //Start by adding the split point (if it is not a control point)
        if (std::ceil(t) != t) {
            firstPart.push_back(startingPoint);
        }
        
        
        //Add all control points until we reach the point before the intersection point
        for (;it!=end;) {
            
            firstPart.push_back(*it);
            
            ++it;
            if (it == cps.end()) {
                it = cps.begin();
            }
        }
        firstPart.push_back(*end);
        //Add the intersection point
        firstPart.push_back(newPoint);
        
        //Make it go after the intersection point to start the second split
        ++it;
        if (it == cps.end()) {
            it = cps.begin();
        }
        
        //Add the intersection point as a starting point of the second split
        secondPart.push_back(newPoint);
        
        //Add control points until we find the starting point (i.e:  the control point after the original split point)
        for (;it != start;) {
            secondPart.push_back(*it);
            ++it;
            if (it == cps.end()) {
                it = cps.begin();
            }
        }
        //Finally add the split point to finish the second split
        secondPart.push_back(startingPoint);
        
        std::list<BezierCPs> regularizedFirst;
        std::list<BezierCPs> regularizedSecond;
        Natron::regularize(firstPart, time, &regularizedFirst);
        Natron::regularize(secondPart, time, &regularizedSecond);
        ret->insert(ret->begin(),regularizedFirst.begin(),regularizedFirst.end());
        ret->insert(ret->end(), regularizedSecond.begin(), regularizedSecond.end());
        return true;
    }
    return false;
}

/**
 * @brief Given the original coon's patch, check if all interior angles are inferior to 180°. If not then we split
 * along the bysector angle and separate the patch.
 **/
static bool checkAnglesAndSplitIfNeeded(const BezierCPs &cps, double time, int sign, std::list<BezierCPs>* ret)
{
    int ncps = (int)cps.size();
    assert(ncps >= 3);
    
    
    for (int i = 0; i < ncps; ++i) {
        Point negativeDir = dirVect(cps, time, i, -1);
        negativeDir.y = -negativeDir.y;
        Point positiveDir = dirVect(cps, time, i, 1);
        
        double py = negativeDir.x * positiveDir.y + negativeDir.y * positiveDir.x;
        if (py * sign < -1e-4) {
            if (splitAt(cps, time, i, ret)) {
                return true;
            }
            return false;
        }
    }
    return false;
    
}

static void tensor(const BezierCPs& p, double time, const Point* internal, Point ret[4][4])
{
    ret[0][0] = getPointAt(p, time, 0);
    ret[0][1] = getLeftPointAt(p, time, 0);
    ret[0][2] = getRightPointAt(p, time, 3);
    ret[0][3] = getPointAt(p, time, 3);
    
    ret[1][0] = getRightPointAt(p, time, 0);
    ret[1][1] = internal[0];
    ret[1][2] = internal[3];
    ret[1][3] = getLeftPointAt(p, time, 3);
    
    ret[2][0] = getLeftPointAt(p, time, 1);
    ret[2][1] = internal[1];
    ret[2][2] = internal[2];
    ret[2][3] = getRightPointAt(p, time, 2);
    
    ret[3][0] = getPointAt(p, time, 1);
    ret[3][1] = getRightPointAt(p, time, 1);
    ret[3][2] = getLeftPointAt(p, time, 2);
    ret[3][3] = getPointAt(p, time, 2);
}

static void coonsPatch(const BezierCPs& p, double time, Point ret[4][4])
{
    assert(p.size() >= 3);
    Point internal[4];
    BezierCPs::const_iterator cur = p.begin();
    BezierCPs::const_iterator prev = p.end();
    --prev;
    BezierCPs::const_iterator next = cur;
    ++next;
    BezierCPs::const_iterator nextNext = next;
    ++nextNext;
    
    for (int j = 0; j < 4; ++j,
         ++prev,++cur,++next,++nextNext) {
        if (cur == p.end()) {
            cur = p.begin();
        }
        if (prev == p.end()) {
            prev = p.begin();
        }
        if (next == p.end()) {
            next = p.begin();
        }
        if (nextNext == p.end()) {
            nextNext = p.begin();
        }
        
        Point p1;
        (*cur)->getPositionAtTime(false ,time, &p1.x, &p1.y);
        
        Point p1left;
        (*cur)->getLeftBezierPointAtTime(false ,time, &p1left.x, &p1left.y);
        
        Point p1right;
        (*cur)->getRightBezierPointAtTime(false ,time, &p1right.x, &p1right.y);
        
        Point p0;
        (*prev)->getPositionAtTime(false ,time, &p0.x, &p0.y);
        
        Point p2;
        (*next)->getPositionAtTime(false ,time, &p2.x, &p2.y);
        
        Point p0left;
        (*prev)->getLeftBezierPointAtTime(false ,time, &p0left.x, &p0left.y);
        
        Point p2right;
        (*next)->getRightBezierPointAtTime(false ,time, &p2right.x, &p2right.y);
        
        Point p3;
        (*nextNext)->getPositionAtTime(false ,time, &p3.x, &p3.y);
        
        internal[j].x = 1. / 9. * (-4. * p1.x + 6. * (p1left.x + p1right.x) - 2. * (p0.x + p2.x) + 3. * (p0left.x + p2right.x) - p3.x);
        internal[j].y = 1. / 9. * (-4. * p1.y + 6. * (p1left.y + p1right.y) - 2. * (p0.y + p2.y) + 3. * (p0left.y + p2right.y) - p3.y);
    }
    return tensor(p, time, internal, ret);
}



static
Point bezier(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    double onemt = 1. - t;
    double onemt2 = onemt * onemt;
    Point ret;
    ret.x = onemt2 * onemt * a.x + t * (3.0 * (onemt2 * b.x + t * onemt * c.x) + t * t * d.x);
    ret.y = onemt2 * onemt * a.y + t * (3.0 * (onemt2 * b.y + t * onemt * c.y) + t * t * d.y);
    return ret;
}

static
Point bezierP(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    Point ret;
    ret.x =  3.0 * (t * t * (d.x - a.x + 3.0 * (b.x - c.x)) + t * (2.0 * (a.x + c.x) - 4.0 * b.x) + b.x - a.x);
    ret.y =  3.0 * (t * t * (d.y - a.y + 3.0 * (b.y - c.y)) + t * (2.0 * (a.y + c.y) - 4.0 * b.y) + b.y - a.y);
    return ret;
}

#ifdef DEAD_CODE
static
Point bezierPP(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    Point ret;
    ret.x = 6.0 * (t * (d.x - a.x + 3.0 * (b.x - c.x)) + a.x + c.x - 2.0 * b.x);
    ret.y = 6.0 * (t * (d.y - a.y + 3.0 * (b.y - c.y)) + a.y + c.y - 2.0 * b.y);
    return ret;
}
#endif // DEAD_CODE

#ifdef DEAD_CODE
static
Point bezierPPP(const Point& a, const Point& b, const Point& c, const Point& d)
{
    Point ret;
    ret.x =  6.0 * (d.x - a.x + 3.0 * (b.x - c.x));
    ret.y =  6.0 * (d.y - a.y + 3.0 * (b.y - c.y));
    return ret;
}
#endif // DEAD_CODE

static
Point BuP(const Point P[4][4], int j, double u) {
    return bezierP(P[0][j],P[1][j],P[2][j],P[3][j],u);
}

static
Point BvP(const Point P[4][4], int i, double v) {
    return bezierP(P[i][0],P[i][1],P[i][2],P[i][3],v);
}

static
double normal(const Point P[4][4], double u, double v)
{
    Point a = bezier(BuP(P,0,u),BuP(P,1,u),BuP(P,2,u),BuP(P,3,u),v);
    Point b = bezier(BvP(P,0,v),BvP(P,1,v),BvP(P,2,v),BvP(P,3,v),u);
    return a.x * b.y - a.y * b.x;
}

static
Point findPointInside(const BezierCPs& cps, double time)
{
    /*
     Given a simple polygon, find some point inside it. Here is a method based on the proof that
     there exists an internal diagonal, in [O'Rourke, 13-14]. The idea is that the midpoint of
     a diagonal is interior to the polygon.
     */
    assert(!cps.empty());
    int i = 0;
    for (BezierCPs::const_iterator it = cps.begin(); it != cps.end(); ++it, ++i) {
        Point dir = dirVect(cps, time, i);
        if (dir.x == 0. && dir.y == 0.) {
            continue;
        }
        Point p;
        (*it)->getPositionAtTime(false ,time, &p.x, &p.y);
        Point q;
        q.x = p.x;
        q.y = p.y + dir.y;
        boost::shared_ptr<BezierCP> newPoint;
        int beforeIndex = -1;
        findIntersection(cps, time, p, q, &newPoint, &beforeIndex);
        if (newPoint && beforeIndex != -1) {
            Point np;
            newPoint->getPositionAtTime(false ,time, &np.x, &np.y);
            if (np.x != p.x || np.y != p.y) {
                Point m;
                m.x = 0.5 * (p.x + np.x);
                m.y = 0.5 * (p.y + np.y);
                return m;
            }
        }
    }
    Point ret;
    cps.front()->getPositionAtTime(false ,time, &ret.x, &ret.y);
    return ret;
}


static double estimate(int elen, double *e)
{
    double Q;
    int eindex;
    
    Q = e[0];
    for (eindex = 1; eindex < elen; ++eindex) {
        Q += e[eindex];
    }
    return Q;
}

#define Fast_Two_Sum_Tail(a, b, x, y) \
    bvirt = x - a; \
    y = b - bvirt

#define Fast_Two_Sum(a, b, x, y) \
    x = (a + b); \
    Fast_Two_Sum_Tail(a, b, x, y)

#define Two_Sum_Tail(a, b, x, y) \
    bvirt = (x - a); \
    avirt = x - bvirt; \
    bround = b - bvirt; \
    around = a - avirt; \
    y = around + bround

#define Two_Sum(a, b, x, y) \
    x = (a + b); \
    Two_Sum_Tail(a, b, x, y)

#define Two_Diff_Tail(a, b, x, y) \
    bvirt = (a - x); \
    avirt = x + bvirt; \
    bround = bvirt - b; \
    around = a - avirt; \
    y = around + bround

#define Two_Diff(a, b, x, y) \
    x = (a - b); \
    Two_Diff_Tail(a, b, x, y)

#define Split_two_product(a, ahi, alo) \
    c = splitter * a; \
    abig = (c - a); \
    ahi = c - abig; \
    alo = a - ahi

#define Two_Product_Tail(a, b, x, y) \
    Split_two_product(a, ahi, alo); \
    Split_two_product(b, bhi, blo); \
    err1 = x - (ahi * bhi); \
    err2 = err1 - (alo * bhi); \
    err3 = err2 - (ahi * blo); \
    y = (alo * blo) - err3

#define Two_Product(a, b, x, y) \
    x = a * b; \
    Two_Product_Tail(a, b, x, y);

#define Two_One_Diff(a1, a0, b, x2, x1, x0) \
    Two_Diff(a0, b , _i, x0); \
    Two_Sum( a1, _i, x2, x1)

#define Two_Two_Diff(a1, a0, b1, b0, x3, x2, x1, x0) \
    Two_One_Diff(a1, a0, b0, _j, _0, x0); \
    Two_One_Diff(_j, _0, b1, x3, x2, x1)


/*****************************************************************************/
/*                                                                           */
/*  fast_expansion_sum_zeroelim()   Sum two expansions, eliminating zero     */
/*                                  components from the output expansion.    */
/*                                                                           */
/*  Sets h = e + f.  See the long version of my paper for details.           */
/*                                                                           */
/*  If round-to-even is used (as with IEEE 754), maintains the strongly      */
/*  nonoverlapping property.  (That is, if e is strongly nonoverlapping, h   */
/*  will be also.)  Does NOT maintain the nonoverlapping or nonadjacent      */
/*  properties.                                                              */
/*                                                                           */
/*****************************************************************************/

static int fast_expansion_sum_zeroelim(int elen, double *e,
                                       int flen, double *f, double *h)
/* h cannot be e or f. */
{
    double Q;
    double Qnew;
    double hh;
    double bvirt;
    double avirt, bround, around;
    int eindex, findex, hindex;
    double enow, fnow;
    
    enow = e[0];
    fnow = f[0];
    eindex = findex = 0;
    if ((fnow > enow) == (fnow > -enow)) {
        Q = enow;
        enow = e[++eindex];
    } else {
        Q = fnow;
        fnow = f[++findex];
    }
    hindex = 0;
    if ((eindex < elen) && (findex < flen)) {
        if ((fnow > enow) == (fnow > -enow)) {
            Fast_Two_Sum(enow, Q, Qnew, hh);
            enow = e[++eindex];
        } else {
            Fast_Two_Sum(fnow, Q, Qnew, hh);
            fnow = f[++findex];
        }
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
        while ((eindex < elen) && (findex < flen)) {
            if ((fnow > enow) == (fnow > -enow)) {
                Two_Sum(Q, enow, Qnew, hh);
                enow = e[++eindex];
            } else {
                Two_Sum(Q, fnow, Qnew, hh);
                fnow = f[++findex];
            }
            Q = Qnew;
            if (hh != 0.0) {
                h[hindex++] = hh;
            }
        }
    }
    while (eindex < elen) {
        Two_Sum(Q, enow, Qnew, hh);
        enow = e[++eindex];
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    while (findex < flen) {
        Two_Sum(Q, fnow, Qnew, hh);
        fnow = f[++findex];
        Q = Qnew;
        if (hh != 0.0) {
            h[hindex++] = hh;
        }
    }
    if ((Q != 0.0) || (hindex == 0)) {
        h[hindex++] = Q;
    }
    return hindex;
}


static double orient2dadapt(const Point &pa, const Point &pb, const Point& pc, double detsum)
{
    double acx, acy, bcx, bcy;
    double acxtail, acytail, bcxtail, bcytail;
    double detleft, detright;
    double detlefttail, detrighttail;
    double det, errbound;
    double B[4], C1[8], C2[12], D[16];
    double B3;
    int C1length, C2length, Dlength;
    double u[4];
    double u3;
    double s1, t1;
    double s0, t0;
    
    double bvirt;
    double avirt, bround, around;
    double c;
    double abig;
    double ahi, alo, bhi, blo;
    double err1, err2, err3;
    double _i, _j;
    double _0;
    
    /* 2^ceiling(p/2) + 1.  Used to split floats in half. */
    double epsilon = std::numeric_limits<double>::epsilon() * 0.5;
    static const double splitter = std::sqrt((DBL_MANT_DIG % 2 ? 2.0 : 1.0) / epsilon)+1.0;
    static const double ccwerrboundB = (2.0 + 12.0 * epsilon) * epsilon;
    static const double ccwerrboundC = (9.0 + 64.0 * epsilon) * epsilon * epsilon;
    static const double resulterrbound = (3.0 + 8.0 * epsilon) * epsilon;
    
    acx = pa.x - pc.x;
    bcx = pb.x - pc.x;
    acy = pa.y - pc.y;
    bcy = pb.y - pc.y;
    
    Two_Product(acx, bcy, detleft, detlefttail);
    Two_Product(acy, bcx, detright, detrighttail);
    
    Two_Two_Diff(detleft, detlefttail, detright, detrighttail,
                 B3, B[2], B[1], B[0]);
    B[3] = B3;
    
    det = estimate(4, B);
    errbound = ccwerrboundB * detsum;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }
    
    Two_Diff_Tail(pa.x, pc.x, acx, acxtail);
    Two_Diff_Tail(pb.x, pc.x, bcx, bcxtail);
    Two_Diff_Tail(pa.y, pc.y, acy, acytail);
    Two_Diff_Tail(pb.y, pc.y, bcy, bcytail);
    
    if ((acxtail == 0.0) && (acytail == 0.0)
        && (bcxtail == 0.0) && (bcytail == 0.0)) {
        return det;
    }
    
    errbound = ccwerrboundC * detsum + resulterrbound * std::fabs(det);
    det += (acx * bcytail + bcy * acxtail)
    - (acy * bcxtail + bcx * acytail);
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }
    
    Two_Product(acxtail, bcy, s1, s0);
    Two_Product(acytail, bcx, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    C1length = fast_expansion_sum_zeroelim(4, B, 4, u, C1);
    
    Two_Product(acx, bcytail, s1, s0);
    Two_Product(acy, bcxtail, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    C2length = fast_expansion_sum_zeroelim(C1length, C1, 4, u, C2);
    
    Two_Product(acxtail, bcytail, s1, s0);
    Two_Product(acytail, bcxtail, t1, t0);
    Two_Two_Diff(s1, s0, t1, t0, u3, u[2], u[1], u[0]);
    u[3] = u3;
    Dlength = fast_expansion_sum_zeroelim(C2length, C2, 4, u, D);
    
    return(D[Dlength - 1]);
}

// Interface to orient2d predicate optimized for pairs.
static double orient2d(const Point& a, const Point& b, const Point& c)
{
    double detsum, errbound;
    double orient;
    
    
    double detleft = (a.x - c.x) * (b.y - c.y);
    double detright = (a.y - c.y) * (b.x - c.x);
    double det = detleft - detright;
    
    if (detleft > 0.0) {
        if (detright <= 0.0) {
            return det;
        } else {
            detsum = detleft + detright;
        }
    } else if (detleft < 0.0) {
        if (detright >= 0.0) {
            return det;
        } else {
            detsum = -detleft - detright;
        }
    } else {
        return det;
    }
    
    double ccwerrboundA = std::numeric_limits<double>::epsilon() * 0.5;
    ccwerrboundA = (3.0 + 16.0 * ccwerrboundA) * ccwerrboundA;
    
    errbound = ccwerrboundA * detsum;
    if ((det >= errbound) || (-det >= errbound)) {
        return det;
    }
    
    Point pa,pb,pc;
    pa = a;
    pb = b;
    pc = c;
    orient = orient2dadapt(pa, pb, pc, detsum);
    return orient;
}

// Returns true if the point z lies in or on the bounding box
// of a,b,c, and d.
static bool insidebbox(const Point& a, const Point& b, const Point& c, const Point& d,
                const Point& z)
{
    RectD bbox;
    bbox.x1 = std::numeric_limits<double>::infinity();
    bbox.x2 = -std::numeric_limits<double>::infinity();
    bbox.y1 = std::numeric_limits<double>::infinity();
    bbox.y2 = -std::numeric_limits<double>::infinity();
    bbox.merge(RectD(a.x,a.y,a.x,a.y));
    bbox.merge(RectD(b.x,b.y,b.x,b.y));
    bbox.merge(RectD(c.x,c.y,c.x,c.y));
    bbox.merge(RectD(d.x,d.y,d.x,d.y));
    return bbox.contains(z.x, z.y);
}



inline static
bool inrange(double x0, double x1, double x)
{
    return (x0 <= x && x <= x1) || (x1 <= x && x <= x0);
}

// Return true if point z is on z0--z1; otherwise compute contribution to
// winding number.
static
bool checkstraight(const Point& z0, const Point& z1, const Point& z, int* count)
{
    if (z0.y <= z.y && z.y <= z1.y) {
        double side = orient2d(z0,z1,z);
        if (side == 0.0 && inrange(z0.x,z1.x,z.x)) {
            return true;
        }
        if (z.y < z1.y && side > 0) {
            *count = *count + 1;
        }
    } else if (z1.y <= z.y && z.y <= z0.y) {
        double side = orient2d(z0,z1,z);
        if (side == 0.0 && inrange(z0.x,z1.x,z.x)) {
            return true;
        }
        if (z.y < z0.y && side < 0) {
            *count = *count - 1;
        }
    }
    return false;
}

// returns true if point is on curve; otherwise compute contribution to
// winding number.
static
bool checkCurve(const Point& z0, const Point& c0, const Point& z1, const Point& c1, const Point& z, int* count, unsigned int depth)
{
    if (!depth) {
        return true;
    }
    --depth;
    if (insidebbox(z0,c0,c1,z1,z)) {
        Point m0,m1,m2,m3,m4,m5;
        m0.x = 0.5 * (z0.x + c0.x);
        m0.y = 0.5 * (z0.y + c0.y);
        
        m1.x = 0.5 * (c0.x + c1.x);
        m1.y = 0.5 * (c0.y + c1.y);
        
        m2.x = 0.5 * (c1.x + z1.x);
        m2.y = 0.5 * (c1.y + z1.y);
        
        m3.x = 0.5 * (m0.x + m1.x);
        m3.y = 0.5 * (m0.y + m1.y);
        
        m4.x = 0.5 * (m1.x + m2.x);
        m4.y = 0.5 * (m1.y + m2.y);
        
        m5.x = 0.5 * (m3.x + m4.x);
        m5.y = 0.5 * (m3.y + m4.y);
        if (checkCurve(z0,m0,m3,m5,z,count,depth) ||
            checkCurve(m5,m4,m2,z1,z,count,depth)) {
            return true;
        }
    } else if (checkstraight(z0,z1,z,count)) {
        return true;
    }
    return false;
}

// Return the winding number of the region bounded by the (cyclic) path
// relative to the point z, or the largest odd integer if the point lies on
// the path.
static int computeWindingNumber(const BezierCPs& patch, double time, const Point& z) {
    
    assert(patch.size() >= 3);
    
    static const int undefined = INT_MAX % 2 ? INT_MAX : INT_MAX - 1;
    const unsigned maxdepth = DBL_MANT_DIG;
    
    int count = 0;
    
    BezierCPs::const_iterator it = patch.begin();
    BezierCPs::const_iterator next = it;
    ++next;
    for (; it != patch.end(); ++it, ++next) {
        if (next == patch.end()) {
            next = patch.begin();
        }
        
        Point p0,p1,p2,p3;
        (*it)->getPositionAtTime(false ,time, &p0.x, &p0.y);
        (*it)->getRightBezierPointAtTime(false ,time, &p1.x, &p1.y);
        (*next)->getLeftBezierPointAtTime(false ,time, &p2.x, &p2.y);
        (*next)->getPositionAtTime(false ,time, &p3.x, &p3.y);
        
        if (checkCurve(p0, p1, p2, p3, z, &count, maxdepth)) {
            return undefined;
        }
    }
    return count;
}

void Natron::regularize(const BezierCPs &patch, double time, std::list<BezierCPs> *fixedPatch)
{
    if (patch.size() < 3) {
        fixedPatch->push_back(patch);
        return;
    }
    
    Point pointInside = findPointInside(patch, time);
    int sign;
    {
        RectD bbox;
        bbox.x1 = std::numeric_limits<double>::infinity();
        bbox.x2 = -std::numeric_limits<double>::infinity();
        bbox.y1 = std::numeric_limits<double>::infinity();
        bbox.y2 = -std::numeric_limits<double>::infinity();
        Bezier::bezierSegmentListBboxUpdate(false ,patch, true, false, time, 0, Transform::Matrix3x3(), &bbox);
        if (!bbox.contains(pointInside.x, pointInside.y)) {
            sign = 0;
        } else {
            int winding_number = computeWindingNumber(patch, time, pointInside);
            sign = (winding_number < 0) ? -1 : ((winding_number > 0) ? 1 : 0);
        }
        
    }
    
    std::list<BezierCPs> splits;
    if (checkAnglesAndSplitIfNeeded(patch, time, sign, &splits)) {
        *fixedPatch = splits;
        return;
    }
    
    Point P[4][4];
    coonsPatch(patch, time, P);
    
    //Check for degeneracy
    Point U[3][4];
    Point V[4][3];
    
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 4; ++j) {
            U[i][j].x = P[i+1][j].x - P[i][j].x;
            U[i][j].y = P[i+1][j].y - P[i][j].y;
        }
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 3; ++j) {
            V[i][j].x = P[i][j+1].x - P[i][j].x;
            V[i][j].y = P[i][j+1].y - P[i][j].y;
        }
    }
    
    int choose2[3] = {1, 2, 1};
    int choose3[4] = {1, 3, 3, 1};
    
    double T[6][6];
    for (int p = 0; p < 6; ++p) {
        int kstart = std::max(p - 2,0);
        int kstop = std::min(p,3);
        for (int q = 0; q < 6; ++q) {
            double Tpq = 0.;
            int jstop = std::min(q,3);
            int jstart = std::max(q - 2,0);
            for (int k = kstart; k <= kstop; ++k) {
                for (int j = jstart; j <= jstop; ++j) {
                    int i = p - k;
                    int l = q - j;
                    Tpq += (-U[i][j].x * V[k][l].y - U[i][j].y * V[k][l].x) * choose2[i] * choose3[k] * choose3[j] * choose2[l];
                }
            }
            T[p][q] = Tpq;
        }
    }
    
    double sqrtEpsilon = std::sqrt(std::numeric_limits<double>::epsilon());
    //0 = default, 1 = true, 2 = false
    int aligned = 0;
    bool degenerate = false;
    for (int p = 0; p < 6; ++p) {
        for (int q = 0; q < 6; ++q) {
            if (aligned == 0) {
                if (T[p][q] > sqrtEpsilon) {
                    aligned = 1;
                }
                if (T[p][q] < -sqrtEpsilon) {
                    aligned = 2;
                }
            } else {
                if ((T[p][q] > sqrtEpsilon && aligned == 2) ||
                    (T[p][q] < -sqrtEpsilon && aligned == 1)) {
                    degenerate = true;
                    break;
                }
            }
        }
    }
    if (!degenerate) {
        //We do not care if the coons patch orientation, if we would though, we would have to reverse the patch
        //if ((sign >= 0 && aligned == 1) || (sign < 0 && aligned == 2)) {
            fixedPatch->push_back(patch);
            return;
        //}
        //fixedPatch->push_back(patch);
        //return;
    }
    
    // Polynomial coefficients of (B_i'' B_j + B_i' B_j')/3.
    static const double fpv0[4][4][5]={
        {{5, -20, 30, -20, 5},
            {-3, 24, -54, 48, -15},
            {0, -6, 27, -36, 15},
            {0, 0, -3, 8, -5}},
        {{-7, 36, -66, 52, -15},
            {3, -36, 108, -120, 45},
            {0, 6, -45, 84, -45},
            {0, 0, 3, -16, 15}},
        {{2, -18, 45, -44, 15},
            {0, 12, -63, 96, -45},
            {0, 0, 18, -60, 45},
            {0, 0, 0, 8, -15}},
        {{0, 2, -9, 12, -5},
            {0, 0, 9, -24, 15},
            {0, 0, 0, 12, -15},
            {0, 0, 0, 0, 5}}
    };
    
    // Compute one-ninth of the derivative of the Jacobian along the boundary.
    
    double c[4][5];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 5; ++j) {
            c[i][j] = 0;
        }
    }
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            const double* w = fpv0[i][j];
            for (int k = 0; k < 5 ;++k) {
                c[0][k] += w[k] * (-P[i][0].x * (P[j][1].y - P[j][0].y) - P[i][0].y * (P[j][1].x - P[j][0].x));
            }
            for (int k = 0; k < 5 ;++k) {
                c[1][k] += w[k] * (-(P[3][j].x - P[2][j].x) * P[3][i].y - (P[3][j].y - P[2][j].y) * P[3][i].x);
            }
            for (int k = 0; k < 5 ;++k) {
                c[2][k] += w[k] * (-P[i][3].x * (P[j][3].y - P[j][2].y) - P[i][3].y * (P[j][3].x - P[j][2].x));
            }
            for (int k = 0; k < 5 ;++k) {
                c[3][k] += w[k] * (-(P[0][j].x - P[1][j].x) * P[0][i].y - (P[0][j].y - P[1][j].y) * P[0][i].x);
            }
        }
    }
    
     // Use Rolle's theorem to check for degeneracy on the boundary.
    double M = 0;
    double cut = 0.;
    for (int i = 0; i < 4; ++i) {
        double sol[4];
        int o[4];
        int nSols = Natron::solveQuartic(c[i][0], c[i][1], c[i][2], c[i][3], c[i][4], sol, o);
        for (int k = 0; k < nSols; ++k) {
            if (std::fabs(sol[k]) < sqrtEpsilon) {
                if (0 <= sol[k] && sol[k] <= 1) {
                    double U[4] = {sol[k], 1, sol[k], 0};
                    double V[4] = {0, sol[k], 1, sol[k]};
                    double T[4] = {sol[k], sol[k], 1 - sol[k], 1 - sol[k]};
                    double N = sign * normal(P, U[i], V[i]);
                    if (N < M) {
                        M = N;
                        cut = i + T[i];
                    }
                }
            }
        }
    }
    
    // Split at the worst boundary degeneracy.
    if (M < 0 && splitAt(patch, time, cut, fixedPatch)) {
        return;
    }
    
    
    // Split arbitrarily to resolve any remaining (internal) degeneracy.
    splitAt(patch, time, 0.5, fixedPatch);
}