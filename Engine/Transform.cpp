
/*
   Copyright (C) 2014 INRIA

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the {organization} nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   INRIA
   Domaine de Voluceau
   Rocquencourt - B.P. 105
   78153 Le Chesnay Cedex - France


   This file was taken from https://github.com/devernay/openfx-misc
   Maybe we should make this a git submodule instead.
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Transform.h"

#include <cassert>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>

#include <ofxCore.h> // kOfxFlagInfiniteMin

#include "Engine/RectD.h"

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

using namespace Transform;
namespace Transform {
double
toDegrees(double rad)
{
    rad = rad * 180.0 / M_PI;

    return rad;
}

double
toRadians(double deg)
{
    deg = deg * M_PI / 180.0;

    return deg;
}

Point3D::Point3D()
    : x(0), y(0), z(0)
{
}

Point3D::Point3D(double x,
                 double y,
                 double z)
    : x(x),y(y),z(z)
{
}

Point3D::Point3D(const Point3D & p)
    : x(p.x),y(p.y),z(p.z)
{
}

bool
Point3D::operator==(const Point3D & other) const
{
    return x == other.x && y == other.y && z == other.z;
}

Point4D::Point4D()
    : x(0), y(0), z(0), w(0)
{
}

Point4D::Point4D(double x,
                 double y,
                 double z,
                 double w)
    : x(x), y(y), z(z), w(w)
{
}

Point4D::Point4D(const Point4D & o)
    : x(o.x), y(o.y), z(o.z), w(o.w)
{
}

double &
Point4D::operator() (int i)
{
    switch (i) {
    case 0:

        return x;
    case 1:

        return y;
    case 2:

        return z;
    case 3:

        return w;
    default:
        assert(false);
        throw std::out_of_range("Point4D");
    }
    ;
}

const double&
Point4D::operator() (int i) const
{
    switch (i) {
    case 0:

        return x;
    case 1:

        return y;
    case 2:

        return z;
    case 3:

        return w;
    default:
        assert(false);
        throw std::out_of_range("Point4D");
    }
    ;
}

bool
Point4D::operator==(const Point4D & o)  const
{
    return x == o.x && y == o.y && z == o.z && w == o.w;
}

Matrix3x3::Matrix3x3()
    : a(1), b(0), c(0), d(0), e(1), f(0), g(0), h(0), i(1)
{
}

Matrix3x3::Matrix3x3(double a_,
                     double b_,
                     double c_,
                     double d_,
                     double e_,
                     double f_,
                     double g_,
                     double h_,
                     double i_)
    : a(a_),b(b_),c(c_),d(d_),e(e_),f(f_),g(g_),h(h_),i(i_)
{
}

Matrix3x3::Matrix3x3(const Matrix3x3 & mat)
    : a(mat.a), b(mat.b), c(mat.c), d(mat.d), e(mat.e), f(mat.f), g(mat.g), h(mat.h), i(mat.i)
{
}

Matrix3x3 &
Matrix3x3::operator=(const Matrix3x3 & m)
{
    a = m.a; b = m.b; c = m.c; d = m.d; e = m.e; f = m.f; g = m.g; h = m.h; i = m.i; return *this;
}

bool
Matrix3x3::isIdentity() const
{
    return a == 1 && b == 0 && c == 0 && d == 0 && e == 1 && f && 0 && g == 0 && h == 0 && i == 1;
}

void
Matrix3x3::setIdentity()
{
    a = 1; b = 0; c = 0;
    d = 0; e = 1; f = 0;
    g = 0; h = 0; i = 1;
}

Matrix3x3
matMul(const Matrix3x3 & m1,
       const Matrix3x3 & m2)
{
    return Matrix3x3(m1.a * m2.a + m1.b * m2.d + m1.c * m2.g,
                     m1.a * m2.b + m1.b * m2.e + m1.c * m2.h,
                     m1.a * m2.c + m1.b * m2.f + m1.c * m2.i,
                     m1.d * m2.a + m1.e * m2.d + m1.f * m2.g,
                     m1.d * m2.b + m1.e * m2.e + m1.f * m2.h,
                     m1.d * m2.c + m1.e * m2.f + m1.f * m2.i,
                     m1.g * m2.a + m1.h * m2.d + m1.i * m2.g,
                     m1.g * m2.b + m1.h * m2.e + m1.i * m2.h,
                     m1.g * m2.c + m1.h * m2.f + m1.i * m2.i);
}

Point3D
matApply(const Matrix3x3 & m,
         const Point3D & p)
{
    Point3D ret;

    ret.x = m.a * p.x + m.b * p.y + m.c * p.z;
    ret.y = m.d * p.x + m.e * p.y + m.f * p.z;
    ret.z = m.g * p.x + m.h * p.y + m.i * p.z;

    return ret;
}
    
void matApply(const Matrix3x3 & m,double* x, double *y, double *z)
{
    double tmpX,tmpY,tmpZ;
    tmpX = m.a * *x + m.b * *y + m.c * *z;
    tmpY = m.d * *x + m.e * *y + m.f * *z;
    tmpZ = m.g * *x + m.h * *y + m.i * *z;
    
    *x = tmpX;
    *y = tmpY;
    *z = tmpZ;
}

Matrix4x4::Matrix4x4()
{
    std::fill(data, data + 16, 0.);
}

Matrix4x4::Matrix4x4(const double d[16])
{
    std::copy(d,d + 16,data);
}

Matrix4x4::Matrix4x4(const Matrix4x4 & o)
{
    std::copy(o.data,o.data + 16,data);
}

double &
Matrix4x4::operator()(int row,
                      int col)
{
    assert(row >= 0 && row < 4 && col >= 0 && col < 4);

    return data[row * 4 + col];
}

double
Matrix4x4::operator()(int row,
                      int col) const
{
    assert(row >= 0 && row < 4 && col >= 0 && col < 4);

    return data[row * 4 + col];
}

Matrix4x4
matMul(const Matrix4x4 & m1,
       const Matrix4x4 & m2)
{
    Matrix4x4 ret;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int x = 0; x < 4; ++x) {
                ret(i,j) += m1(i,x) * m2(x,j);
            }
        }
    }

    return ret;
}

Point4D
matApply(const Matrix4x4 & m,
         const Point4D & p)
{
    Point4D ret;

    for (int i = 0; i < 4; ++i) {
        ret(i) = 0.;
        for (int j = 0; j < 4; ++j) {
            ret(i) += m(i,j) * p(j);
        }
    }

    return ret;
}


//static
//Matrix4x4
//matrix4x4FromMatrix3x3(const Matrix3x3 & m)
//{
//    Matrix4x4 ret;
//
//    ret(0,0) = m.a; ret(0,1) = m.b; ret(0,2) = m.c; ret(0,3) = 0.;
//    ret(1,0) = m.d; ret(1,1) = m.e; ret(1,2) = m.f; ret(1,3) = 0.;
//    ret(2,0) = m.g; ret(2,1) = m.h; ret(2,2) = m.i; ret(2,3) = 0.;
//    ret(3,0) = 0.;  ret(3,1) = 0.;  ret(3,2) = 0.;  ret(3,3) = 1.;
//
//    return ret;
//}

////////////////////
// IMPLEMENTATION //
////////////////////


double
matDeterminant(const Matrix3x3 & M)
{
    return M.a * (M.e * M.i - M.h * M.f)
           - M.b * (M.d * M.i - M.g * M.f)
           + M.c * (M.d * M.h - M.g * M.e);
}


Matrix3x3
matScaleAdjoint(const Matrix3x3 & M,
                double s)
{
    Matrix3x3 ret;

    ret.a = (s) * (M.e * M.i - M.h * M.f);
    ret.d = (s) * (M.f * M.g - M.d * M.i);
    ret.g = (s) * (M.d * M.h - M.e * M.g);

    ret.b = (s) * (M.c * M.h - M.b * M.i);
    ret.e = (s) * (M.a * M.i - M.c * M.g);
    ret.h = (s) * (M.b * M.g - M.a * M.h);

    ret.c = (s) * (M.b * M.f - M.c * M.e);
    ret.f = (s) * (M.c * M.d - M.a * M.f);
    ret.i = (s) * (M.a * M.e - M.b * M.d);

    return ret;
}


Matrix3x3
matInverse(const Matrix3x3 & M)
{
    return matScaleAdjoint( M, 1. / matDeterminant(M) );
}


Matrix3x3
matInverse(const Matrix3x3 & M,
           double det)
{
    return matScaleAdjoint(M, 1. / det);
}


static
Matrix3x3
matRotation(double rads)
{
    double c = std::cos(rads);
    double s = std::sin(rads);

    return Matrix3x3(c,s,0,-s,c,0,0,0,1);
}

static
Matrix3x3
matTranslation(double x,
               double y)
{
    return Matrix3x3(1., 0., x,
                     0., 1., y,
                     0., 0., 1.);
}

#if 0
static
Matrix3x3
matRotationAroundPoint(double rads,
                       double px,
                       double py)
{
    return matMul( matTranslation(px, py), matMul( matRotation(rads), matTranslation(-px, -py) ) );
}

#endif

static
Matrix3x3
matScale(double x,
         double y)
{
    return Matrix3x3(x,  0., 0.,
                     0., y,  0.,
                     0., 0., 1.);
}

#if 0
static
Matrix3x3
matScale(double s)
{
    return matScale(s, s);
}

static
Matrix3x3
matScaleAroundPoint(double scaleX,
                    double scaleY,
                    double px,
                    double py)
{
    return matMul( matTranslation(px,py), matMul( matScale(scaleX,scaleY), matTranslation(-px, -py) ) );
}

#endif

static
Matrix3x3
matSkewXY(double skewX,
          double skewY,
          bool skewOrderYX)
{
    return Matrix3x3(skewOrderYX ? 1. : (1. + skewX * skewY), skewX, 0.,
                     skewY, skewOrderYX ? (1. + skewX * skewY) : 1, 0.,
                     0., 0., 1.);
}

#if 0
// matrix transform from destination to source
static
Matrix3x3
matInverseTransformCanonical(double translateX,
                             double translateY,
                             double scaleX,
                             double scaleY,
                             double skewX,
                             double skewY,
                             bool skewOrderYX,
                             double rads,
                             double centerX,
                             double centerY)
{
    ///1) We translate to the center of the transform.
    ///2) We scale
    ///3) We apply skewX and skewY in the right order
    ///4) We rotate
    ///5) We apply the global translation
    ///5) We translate back to the origin

    // since this is the inverse, oerations are in reverse order
    return matMul( matMul( matMul( matMul( matMul( matTranslation(centerX, centerY),
                                                   matScale(1. / scaleX, 1. / scaleY) ),
                                           matSkewXY(-skewX, -skewY, !skewOrderYX) ),
                                   matRotation(rads) ),
                           matTranslation(-translateX,-translateY) ),
                   matTranslation(-centerX,-centerY) );
}

#endif

// matrix transform from source to destination
Matrix3x3
matTransformCanonical(double translateX,
                      double translateY,
                      double scaleX,
                      double scaleY,
                      double skewX,
                      double skewY,
                      bool skewOrderYX,
                      double rads,
                      double centerX,
                      double centerY)
{
    ///1) We translate to the center of the transform.
    ///2) We scale
    ///3) We apply skewX and skewY in the right order
    ///4) We rotate
    ///5) We apply the global translation
    ///5) We translate back to the origin

    return matMul( matMul( matMul( matMul( matMul( matTranslation(centerX, centerY),
                                                   matTranslation(translateX, translateY) ),
                                           matRotation(-rads) ),
                                   matSkewXY(skewX, skewY, skewOrderYX) ),
                           matScale(scaleX, scaleY) ),
                   matTranslation(-centerX,-centerY) );
}

// The transforms between pixel and canonical coordinated
// http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html#MappingCoordinates


/// transform from pixel coordinates to canonical coordinates
Matrix3x3
matPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                    double renderscaleX,     //!< 0.5 for a half-resolution image
                    double renderscaleY,
                    bool fielded)     //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
{
    /*
       To map an X and Y coordinates from Pixel coordinates to Canonical coordinates, we perform the following multiplications...

       X' = (X * PAR)/SX
       Y' = Y/(SY * FS)
     */

    // FIXME: when it's the Upper field, showuldn't the first pixel start at canonical coordinate (0,0.5) ?
    return matScale( pixelaspectratio / renderscaleX, 1. / ( renderscaleY * (fielded ? 0.5 : 1.0) ) );
}


/// transform from canonical coordinates to pixel coordinates
Matrix3x3
matCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                    double renderscaleX,     //!< 0.5 for a half-resolution image
                    double renderscaleY,
                    bool fielded)     //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
{
    /*
       To map an X and Y coordinates from Canonical coordinates to Pixel coordinates, we perform the following multiplications...

       X' = (X * SX)/PAR
       Y' = Y * SY * FS
     */

    // FIXME: when it's the Upper field, showuldn't the first pixel start at canonical coordinate (0,0.5) ?
    return matScale( renderscaleX / pixelaspectratio, renderscaleY * (fielded ? 0.5 : 1.0) );
}

    
 #if 0
// matrix transform from destination to source
static
Matrix3x3
matInverseTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                         double renderscaleX,     //!< 0.5 for a half-resolution image
                         double renderscaleY,
                         bool fielded,
                         double translateX,
                         double translateY,
                         double scaleX,
                         double scaleY,
                         double skewX,
                         double skewY,
                         bool skewOrderYX,
                         double rads,
                         double centerX,
                         double centerY)
{
    ///1) We go from pixel to canonical
    ///2) we apply transform
    ///3) We go back to pixels

    return matMul( matMul( matCanonicalToPixel(pixelaspectratio, renderscaleX, renderscaleY, fielded),
                           matInverseTransformCanonical(translateX, translateY, scaleX, scaleY, skewX, skewY, skewOrderYX, rads, centerX, centerY) ),
                   matPixelToCanonical(pixelaspectratio, renderscaleX, renderscaleY, fielded) );
}

// matrix transform from source to destination
static
Matrix3x3
matTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                  double renderscaleX,     //!< 0.5 for a half-resolution image
                  double renderscaleY,
                  bool fielded,
                  double translateX,
                  double translateY,
                  double scaleX,
                  double scaleY,
                  double skewX,
                  double skewY,
                  bool skewOrderYX,
                  double rads,
                  double centerX,
                  double centerY)
{
    ///1) We go from pixel to canonical
    ///2) we apply transform
    ///3) We go back to pixels

    return matMul( matMul( matCanonicalToPixel(pixelaspectratio, renderscaleX, renderscaleY, fielded),
                           matTransformCanonical(translateX, translateY, scaleX, scaleY, skewX, skewY, skewOrderYX, rads, centerX, centerY) ),
                   matPixelToCanonical(pixelaspectratio, renderscaleX, renderscaleY, fielded) );
}

#endif // if 0
    
    
// compute the bounding box of the transform of four points
static void
transformRegionFromPoints(const Point3D p[4], RectD &rod)
{
    // extract the x/y bounds
    double x1, y1, x2, y2;
    
    // if all z's have the same sign, we can compute a reasonable ROI, else we give the whole image (the line at infinity crosses the rectangle)
    bool allpositive = true;
    bool allnegative = true;
    for (int i = 0; i < 4; ++i) {
        allnegative = allnegative && (p[i].z < 0.);
        allpositive = allpositive && (p[i].z > 0.);
    }
    
    if (!allpositive && !allnegative) {
        // the line at infinity crosses the source RoD
        x1 = kOfxFlagInfiniteMin;
        x2 = kOfxFlagInfiniteMax;
        y1 = kOfxFlagInfiniteMin;
        y2 = kOfxFlagInfiniteMax;
    } else {
        OfxPointD q[4];
        for (int i = 0; i < 4; ++i) {
            q[i].x = p[i].x / p[i].z;
            q[i].y = p[i].y / p[i].z;
        }
        
        x1 = x2 = q[0].x;
        y1 = y2 = q[0].y;
        for (int i = 1; i < 4; ++i) {
            if (q[i].x < x1) {
                x1 = q[i].x;
            } else if (q[i].x > x2) {
                x2 = q[i].x;
            }
            if (q[i].y < y1) {
                y1 = q[i].y;
            } else if (q[i].y > y2) {
                y2 = q[i].y;
            }
        }
    }
    
    // GENERIC
    rod.x1 = x1;
    rod.x2 = x2;
    rod.y1 = y1;
    rod.y2 = y2;
    assert(rod.x1 <= rod.x2 && rod.y1 <= rod.y2);
}
    
// compute the bounding box of the transform of a rectangle
void
transformRegionFromRoD(const RectD &srcRect, const Matrix3x3 &transform, RectD &dstRect)
{
    /// now transform the 4 corners of the source clip to the output image
    Point3D p[4];
    p[0] = matApply(transform, Point3D(srcRect.x1,srcRect.y1,1));
    p[1] = matApply(transform, Point3D(srcRect.x1,srcRect.y2,1));
    p[2] = matApply(transform, Point3D(srcRect.x2,srcRect.y2,1));
    p[3] = matApply(transform, Point3D(srcRect.x2,srcRect.y1,1));
    
    
    transformRegionFromPoints(p, dstRect);
}

    
    
} //namespace Transform




