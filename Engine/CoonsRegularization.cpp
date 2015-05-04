//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "CoonsRegularization.h"


#include "Engine/RotoContext.h"
#include "Engine/RotoContextPrivate.h"
#include "Engine/Interpolation.h"

using namespace Natron;



static Point getPointAt(const BezierCPs& cps, int time, double t)
{
    if (t < 0) {
        t += (int)cps.size();
    }
    int t_i = (int)std::floor(t) % (int)cps.size();
    int t_i_plus_1 = t_i % (int)cps.size();
    assert(t_i >= 0 && t_i < (int)cps.size() && t_i_plus_1 >= 0 && t_i_plus_1 < (int)cps.size());
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getPositionAtTime(time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getPositionAtTime(time, &ret.x, &ret.y);
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
    (*it)->getPositionAtTime(time, &p0.x, &p0.y);
    (*it)->getRightBezierPointAtTime(time, &p1.x, &p1.y);
    (*next)->getLeftBezierPointAtTime(time, &p2.x, &p2.y);
    (*next)->getPositionAtTime(time, &p3.x, &p3.y);
    Point ret;
    Bezier::bezierPoint(p0, p1, p2, p3, t - t_i, &ret);
    return ret;
}

static Point getLeftPointAt(const BezierCPs& cps, int time, double t)
{
    if (t < 0) {
        t += (int)cps.size();
    }
    int t_i = (int)std::floor(t) % (int)cps.size();
    int t_i_plus_1 = t_i % (int)cps.size();
    assert(t_i >= 0 && t_i < (int)cps.size() && t_i_plus_1 >= 0 && t_i_plus_1 < (int)cps.size());
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getLeftBezierPointAtTime(time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getLeftBezierPointAtTime(time, &ret.x, &ret.y);
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
    (*it)->getPositionAtTime(time, &a.x, &a.y);
    (*it)->getRightBezierPointAtTime(time, &b.x, &b.y);
    (*next)->getLeftBezierPointAtTime(time, &c.x, &c.y);
    ab.x = (1. - t) * a.x + t * b.x;
    ab.y = (1. - t) * a.y + t * b.y;
    
    bc.x = (1. - t) * b.x + t * c.x;
    bc.y = (1. - t) * b.y + t * c.y;
    
    abc.x = (1. - t) * ab.x + t * bc.x;
    abc.y = (1. - t) * ab.y + t * bc.y;
    if (abc.x == a.x && abc.y == a.y) {
        (*it)->getLeftBezierPointAtTime(time, &ret.x, &ret.y);
        return ret;
    } else {
        return abc;
    }
}

static Point getRightPointAt(const BezierCPs& cps, int time, double t)
{
    if (t < 0) {
        t += (int)cps.size();
    }
    int t_i = (int)std::floor(t) % (int)cps.size();
    int t_i_plus_1 = t_i % (int)cps.size();
    assert(t_i >= 0 && t_i < (int)cps.size() && t_i_plus_1 >= 0 && t_i_plus_1 < (int)cps.size());
    if (t == t_i) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i);
        Point ret;
        (*it)->getRightBezierPointAtTime(time, &ret.x, &ret.y);
        return ret;
    } else if (t == t_i_plus_1) {
        BezierCPs::const_iterator it = cps.begin();
        std::advance(it, t_i_plus_1);
        Point ret;
        (*it)->getRightBezierPointAtTime(time, &ret.x, &ret.y);
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
    (*it)->getRightBezierPointAtTime(time, &a.x, &a.y);
    (*next)->getLeftBezierPointAtTime(time, &b.x, &b.y);
    (*next)->getPositionAtTime(time, &c.x, &c.y);
    ab.x = (1. - t) * a.x + t * b.x;
    ab.y = (1. - t) * a.y + t * b.y;
    
    bc.x = (1. - t) * b.x + t * c.x;
    bc.y = (1. - t) * b.y + t * c.y;
    
    abc.x = (1. - t) * ab.x + t * bc.x;
    abc.y = (1. - t) * ab.y + t * bc.y;
    if (abc.x == c.x && abc.y == c.y) {
        (*next)->getRightBezierPointAtTime(time, &ret.x, &ret.y);
        return ret;
    } else {
        return abc;
    }
}

static double norm(const Point& z0, const Point& c0, const Point& c1, const Point& z1)
{
    double fuzz2 = 1000. * std::numeric_limits<double>::epsilon() * 1000. * std::numeric_limits<double>::epsilon();
    double p1_p1p2Norm = std::sqrt((c0.x - z0.x) * (c0.x - z0.x) + (c0.y - z0.y) * (c0.y - z0.y));
    double p1_p2p1Norm = std::sqrt((c1.x - z0.x) * (c1.x - z0.x) + (c1.y - z0.y) * (c1.y - z0.y));
    double p1_p2Norm = std::sqrt((z1.x - z0.x) * (z1.x - z0.x) + (z1.y - z0.y) * (z1.y - z0.y));
    return fuzz2 * std::max(p1_p1p2Norm,std::max(p1_p2p1Norm,p1_p2Norm));
}

static Point predir(const BezierCPs& cps, int time, double t)
{
    //Compute the unit vector in the direction of the bysector angle
    Point dir;
    Point p1,p2,p2p1,p1p2;
    p2 = getPointAt(cps, time, t);
    p2p1 = getLeftPointAt(cps, time, t);
    p1p2 = getRightPointAt(cps, time, t - 1);
    p1 = getPointAt(cps, time, t);
    dir.x =  3. * (p2.x - p2p1.x);
    dir.y =  3. * (p2.y - p2p1.y);
    
    double epsilon = norm(p1, p1p2, p2p1, p2);
    
    double predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    double normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    if (predirNormSquared > epsilon) {
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = 2. * p2p1.x - p1p2.x - p2.x;
    dir.y = 2. * p2p1.y - p1p2.y - p2.y;
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    if (predirNormSquared > epsilon) {
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p2.x - p1.x + 3. * (p1p2.x - p2p1.x);
    dir.y = p2.y - p1.y + 3. * (p1p2.y - p2p1.y);
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    Point ret;
    ret.x = dir.x / normDir;
    ret.y = dir.y / normDir;
    return ret;
}

static Point postdir(const BezierCPs& cps, int time, double t)
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
    double normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    if (predirNormSquared > epsilon) {
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p2.x - 2 * p2p3.x + p3p2.x;
    dir.y = p2.y - 2 * p2p3.y + p3p2.y;
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    if (predirNormSquared > epsilon) {
        Point ret;
        ret.x = dir.x / normDir;
        ret.y = dir.y / normDir;
        return ret;
    }
    dir.x = p3.x - p2.x + 3. * (p2p3.x - p3p2.x);
    dir.y = p3.y - p2.y + 3. * (p2p3.y - p3p2.y);
    predirNormSquared = dir.x * dir.x + dir.y * dir.y;
    normDir = std::sqrt(predirNormSquared);
    assert(normDir != 0);
    Point ret;
    ret.x = dir.x / normDir;
    ret.y = dir.y / normDir;
    return ret;
}


static Point dirVect(const BezierCPs& cps, int time, double t)
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
        assert(norm != 0);
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


static boost::shared_ptr<BezierCP> makeBezierCPFromPoint(const Point& p)
{
    boost::shared_ptr<BezierCP> ret(new BezierCP);
    ret->setStaticPosition(p.x, p.y);
    return ret;
}

static void findIntersection(const BezierCPs& cps,
                             int time,
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
    std::vector<std::pair<Point,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > > intersections;
    
    BezierCPs::const_iterator s1 = cps.begin();
    BezierCPs::const_iterator s2 = cps.begin();
    ++s2;
    for (; s1 != cps.end(); ++s1,++s2) {
        if (s2 == cps.end()) {
            s2 = cps.begin();
        }
      
        Point z0,z1,c0,c1;
        (*s1)->getPositionAtTime(time, &z0.x, &z0.y);
        (*s1)->getRightBezierPointAtTime(time, &c0.x, &c0.y);
        (*s2)->getPositionAtTime(time, &z1.x, &z1.y);
        (*s2)->getLeftBezierPointAtTime(time, &c1.x, &c0.y);
        
        
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
                int nsols = Natron::solveCubic(a, b, c, d, r, order);
                for (int i = 0; i < nsols; ++i) {
                    roots.push_back(r[i]);
                }
            }
        
        for (std::size_t i = 0; i < roots.size(); ++i) {
            if (roots[i] >= -fuzz && roots[i] <= 1. + fuzz) {
                if (i + roots[i] >= roots.size() - fuzz) {
                    roots[i] = 0;
                }
                Point intersection;
                Bezier::bezierPoint(z0,c0,c1,z1,roots[i],&intersection);
                
                double distToP = std::sqrt((p.x - intersection.x) * (p.x - intersection.x) + (p.y - intersection.y) * (p.y - intersection.y));
                if (std::abs(distToP) < 1e-4) {
                    continue;
                }
                
                bool found = false;
                for (std::vector<std::pair<Point,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > >::iterator it = intersections.begin(); it!=intersections.end(); ++it) {
                    
                    double distSquared = (intersection.x - it->first.x) * (intersection.x - it->first.x) + (intersection.y - it->first.y) * (intersection.y - it->first.y);
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
    
    
    
    assert(intersections.size() == 1 || intersections.size() == 2);
    const std::pair<Point,std::pair<BezierCPs::const_iterator,BezierCPs::const_iterator> > & inter = intersections.front();
    
    newPoint->reset(new BezierCP);
    (*newPoint)->setStaticPosition(inter.first.x, inter.first.y);
    *before = std::distance(cps.begin(), inter.second.first);
}

static bool splitAt(const BezierCPs &cps, int time, double t, std::list<BezierCPs>* ret)
{
    Point dir = dirVect(cps, time, t);
    if (dir.x != 0. || dir.y != 0.) {
        Point z = getPointAt(cps, time, t);
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
        BezierCPs::const_iterator start = it;
        
        BezierCPs::const_iterator end = cps.begin();
        std::advance(end, pointIdx);
        
        boost::shared_ptr<BezierCP> startingPoint = makeBezierCPFromPoint(z);
        firstPart.push_back(startingPoint);
        for (; it!=end; ++it) {
            if (it == cps.end()) {
                it = cps.begin();
            }
            firstPart.push_back(*it);
        }
        firstPart.push_back(*end);
        firstPart.push_back(newPoint);
        
        ++it;
        if (it == cps.end()) {
            it = cps.begin();
        }
        
        secondPart.push_back(newPoint);
        for (; it != start; ++it) {
            if (it == cps.end()) {
                it = cps.begin();
            }
            secondPart.push_back(*it);
        }
        secondPart.push_back(startingPoint);
        
        std::list<BezierCPs> regularizedFirst;
        std::list<BezierCPs> regularizedSecond;
        Natron::regularize(firstPart, time, &regularizedFirst);
        Natron::regularize(secondPart, time, &regularizedSecond);
        ret->insert(ret->begin(),regularizedFirst.begin(),regularizedSecond.end());
        ret->insert(ret->end(), regularizedSecond.begin(), regularizedSecond.end());
        return true;
    }
    return false;
}

/**
 * @brief Given the original coon's patch, check if all interior angles are inferior to 180°. If not then we split
 * along the bysector angle and separate the patch.
 **/
static bool checkAnglesAndSplitIfNeeded(const BezierCPs &cps, int time,std::list<BezierCPs>* ret)
{
    assert(cps.size() == 4);
    
    
    
    BezierCPs::const_iterator prev = cps.end();
    --prev;
    BezierCPs::const_iterator cur = cps.begin();
    BezierCPs::const_iterator next = cur;
    ++next;
    int t = 0;
    while (cur != cps.end()) {
        if (next == cps.end()) {
            next = cps.begin();
        }
        if (prev == cps.end()) {
            prev = cps.begin();
        }
        
        Point p1,p2,p3,p2p1,p2p3,p1p2,p3p2;
        (*prev)->getPositionAtTime(time, &p1.x, &p1.y);
        (*prev)->getRightBezierPointAtTime(time, &p1p2.x, &p1p2.y);
        (*cur)->getPositionAtTime(time, &p2.x, &p2.y);
        (*cur)->getLeftBezierPointAtTime(time, &p2p1.x, &p2p1.y);
        (*cur)->getRightBezierPointAtTime(time, &p2p3.x, &p2p3.y);
        (*next)->getPositionAtTime(time, &p3.x, &p3.y);
        (*next)->getLeftBezierPointAtTime(time, &p3p2.x, &p3p2.y);
        
        //use inner product: alpha = acos(u.v / |u||v| ) to find out the angle
        Point u,v;
        u.x = p1.x - p2.x;
        u.y = p1.y - p2.y;
        v.x = p3.x - p2.x;
        v.y = p3.y - p2.y;
        double normU = std::sqrt(u.x * u.x + u.y * u.y);
        double normV = std::sqrt(v.x * v.x + v.y * v.y);
        assert(normU != 0 && normV != 0);
        double alpha = std::acos((u.x * v.x + u.y * v.y) / normU * normV);
        
        if (alpha > M_PI_2) {
            splitAt(cps, time, t, ret);
            return true;
        }
        
        ++cur;
        ++prev;
        ++next;
        ++t;
    }
    
    ret->push_back(cps);
    return false;
    
}

static void tensor(const BezierCPs& p, int time, const Point* internal, Point ret[4][4])
{
    ret[0][0] = getPointAt(p, time, 0);
    ret[0][1] = getLeftPointAt(p, time, 0);
    ret[0][2] = getRightPointAt(p, time, 3);
    ret[0][3] = getPointAt(p, time, 0);
    
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

static void coonsPatch(const BezierCPs& p, int time, Point ret[4][4])
{
    assert(p.size() == 4);
    Point internal[4];
    for (int j = 0; j < 4; ++j) {
        Point p1 = getPointAt(p, time, j);
        Point p1left = getLeftPointAt(p, time, j);
        Point p1right = getRightPointAt(p, time, j);
        
        Point p0 = getPointAt(p, time, j - 1);
        Point p2 = getPointAt(p, time, j + 1);
        Point p0left = getLeftPointAt(p, time, j - 1);
        Point p2right = getRightPointAt(p, time, j + 1);
        Point p3 = getPointAt(p, time, j + 2);
        
        internal[j].x = 1. / 9. * (-4. * p1.x + 6. * (p1left.x + p1right.x) - 2. * (p0.x + p2.x) + 3. * (p0left.x + p2right.x) - p3.x);
        internal[j].y = 1. / 9. * (-4. * p1.y + 6. * (p1left.y + p1right.y) - 2. * (p0.y + p2.y) + 3. * (p0left.y + p2right.y) - p3.y);
    }
    return tensor(p, time, internal, ret);
}



Point bezier(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    double onemt = 1. - t;
    double onemt2 = onemt * onemt;
    Point ret;
    ret.x = onemt2 * onemt * a.x + t * (3.0 * (onemt2 * b.x + t * onemt * c.x) + t * t * d.x);
    ret.y = onemt2 * onemt * a.y + t * (3.0 * (onemt2 * b.y + t * onemt * c.y) + t * t * d.y);
    return ret;
}

Point bezierP(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    Point ret;
    ret.x =  3.0 * (t * t * (d.x - a.x + 3.0 * (b.x - c.x)) + t * (2.0 * (a.x + c.x) - 4.0 * b.x) + b.x - a.x);
    ret.y =  3.0 * (t * t * (d.y - a.y + 3.0 * (b.y - c.y)) + t * (2.0 * (a.y + c.y) - 4.0 * b.y) + b.y - a.y);
    return ret;
}

Point bezierPP(const Point& a, const Point& b, const Point& c, const Point& d, double t)
{
    Point ret;
    ret.x = 6.0 * (t * (d.x - a.x + 3.0 * (b.x - c.x)) + a.x + c.x - 2.0 * b.x);
    ret.y = 6.0 * (t * (d.y - a.y + 3.0 * (b.y - c.y)) + a.y + c.y - 2.0 * b.y);
    return ret;
}

Point bezierPPP(const Point& a, const Point& b, const Point& c, const Point& d)
{
    Point ret;
    ret.x =  6.0 * (d.x - a.x + 3.0 * (b.x - c.x));
    ret.y =  6.0 * (d.y - a.y + 3.0 * (b.y - c.y));
    return ret;
}

Point BuP(const Point P[4][4], int j, double u) {
    return bezierP(P[0][j],P[1][j],P[2][j],P[3][j],u);
}

Point BvP(const Point P[4][4], int i, double v) {
    return bezierP(P[i][0],P[i][1],P[i][2],P[i][3],v);
}

double normal(const Point P[4][4], double u, double v) {
    Point a = bezier(BuP(P,0,u),BuP(P,1,u),BuP(P,2,u),BuP(P,3,u),v);
    Point b = bezier(BvP(P,0,v),BvP(P,1,v),BvP(P,2,v),BvP(P,3,v),u);
    return a.x * b.y - a.y * b.x;
}

Point findPointInside(const BezierCPs& cps, int time)
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
        (*it)->getPositionAtTime(time, &p.x, &p.y);
        Point q;
        q.x = p.x;
        q.y = p.y + dir.y;
        boost::shared_ptr<BezierCP> newPoint;
        int beforeIndex = -1;
        findIntersection(cps, time, p, q, &newPoint, &beforeIndex);
        if (newPoint && beforeIndex != -1) {
            Point np;
            newPoint->getPositionAtTime(time, &np.x, &np.y);
            if (np.x != p.x || np.y != p.y) {
                Point m;
                m.x = 0.5 * (p.x + np.x);
                m.y = 0.5 * (p.y + np.y);
                return m;
            }
        }
    }
    Point ret;
    cps.front()->getPositionAtTime(time, &ret.x, &ret.y);
    return ret;
}

void Natron::regularize(const BezierCPs &patch, int time, std::list<BezierCPs> *fixedPatch)
{
    if (patch.size() < 4) {
        fixedPatch->push_back(patch);
        return;
    }
    
    std::list<Point> discretizedPolygon;
    RectD bbox;
    Bezier::deCastelJau(patch, time, 0, true, -1, &discretizedPolygon, &bbox);
    Point pointInside = findPointInside(patch, time);
    int sign;
    {
        int winding_number = 0;
        if ( (pointInside.x < bbox.x1) || (pointInside.x >= bbox.x2) || (pointInside.y < bbox.y1) || (pointInside.y >= bbox.y2) ) {
            winding_number = 0;
        } else {
            std::list<Point>::const_iterator last_pt = discretizedPolygon.begin();
            std::list<Point>::const_iterator last_start = last_pt;
            std::list<Point>::const_iterator cur = last_pt;
            ++cur;
            for (; cur != discretizedPolygon.end(); ++cur, ++last_pt) {
                Bezier::point_line_intersection(*last_pt, *cur, pointInside, &winding_number);
            }
            
            // implicitly close last subpath
            if (last_pt != last_start) {
                Bezier::point_line_intersection(*last_pt, *last_start, pointInside, &winding_number);
            }
        }
        if (winding_number == 0) {
            sign = 0;
        } else if (winding_number < 0) {
            sign = -1;
        } else if (winding_number > 0) {
            sign = 1;
        }
    }
    
    std::list<BezierCPs> splits;
    if (checkAnglesAndSplitIfNeeded(patch, time, &splits)) {
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
        double *Tp = T[p];
        for (int q = 0; q < 6; ++q) {
            double Tpq = 0.;
            int jstop = std::min(q,3);
            int jstart = std::max(q - 2,0);
            for (int k = kstart; k <= kstop; ++k) {
                for (int j = jstart; j <= jstop; ++j) {
                    int i = p - k;
                    int l = q - j;
                    Tpq += (U[i][j].x * V[k][l].y - U[i][j].y * V[k][l].x) * choose2[i] * choose3[k] * choose3[j] * choose2[l];
                }
            }
            Tp[q] = Tpq;
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
        fixedPatch->push_back(patch);
        return;
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
        for (int j = 0; j < 4; ++j) {
            const double* w = fpv0[i][j];
            for (int k = 0; k < 5 ;++k) {
                c[0][k] += w[k] * (P[i][0].x * (P[j][1].y - P[j][0].y) - P[i][0].y * (P[j][1].x - P[j][0].x));
            }
            for (int k = 0; k < 5 ;++k) {
                c[1][k] += w[k] * ((P[3][j].x - P[2][j].x) * P[3][i].y - (P[3][j].y - P[2][j].y) * P[3][i].x);
            }
            for (int k = 0; k < 5 ;++k) {
                c[2][k] += w[k] * (P[i][3].x * (P[j][3].y - P[j][2].y) - P[i][3].y * (P[j][3].x - P[j][2].x));
            }
            for (int k = 0; k < 5 ;++k) {
                c[3][k] += w[k] * ((P[0][j].x - P[1][j].x) * P[0][i].y - (P[0][j].y - P[1][j].y) * P[0][i].x);
            }
        }
    }
    
     // Use Rolle's theorem to check for degeneracy on the boundary.
    double M = 0;
    double cut;
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