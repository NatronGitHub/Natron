#ifndef TRANSFORM_H
#define TRANSFORM_H
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

#include <cmath>

namespace Transform {
inline double toDegrees(double rad);
inline double toRadians(double deg);
struct Point3D
{
    double x,y,z;

    Point3D();

    Point3D(double x,
            double y,
            double z);

    Point3D(const Point3D & p);

    bool operator==(const Point3D & other) const;
};

struct Point4D
{
    double x,y,z,w;

    Point4D();

    Point4D(double x,
            double y,
            double z,
            double w);

    Point4D(const Point4D & o);
    double & operator() (int i);

    const double& operator() (int i) const;

    bool operator==(const Point4D & o) const;
};


/**
 * @brief A simple 3 * 3 matrix class layed out as such:
 *  a b c
 *  d e f
 *  g h i
 **/
struct Matrix3x3
{
    double a,b,c,d,e,f,g,h,i;

    Matrix3x3();

    Matrix3x3(double a_,
              double b_,
              double c_,
              double d_,
              double e_,
              double f_,
              double g_,
              double h_,
              double i_);

    Matrix3x3(const Matrix3x3 & mat);
    Matrix3x3 & operator=(const Matrix3x3 & m);

    bool isIdentity() const;

    void setIdentity();
};

// double matDeterminant(const Matrix3x3& M);

// Matrix3x3 matScaleAdjoint(const Matrix3x3& M, double s);

// Matrix3x3 matInverse(const Matrix3x3& M);
// Matrix3x3 matInverse(const Matrix3x3& M,double det);

// Matrix3x3 matRotation(double rads);
// Matrix3x3 matRotationAroundPoint(double rads, double pointX, double pointY);

// Matrix3x3 matTranslation(double translateX, double translateY);

// Matrix3x3 matScale(double scaleX, double scaleY);
// Matrix3x3 matScale(double scale);
// Matrix3x3 matScaleAroundPoint(double scaleX, double scaleY, double pointX, double pointY);

// Matrix3x3 matSkewXY(double skewX, double skewY, bool skewOrderYX);

// matrix transform from destination to source, in canonical coordinates
// Matrix3x3 matInverseTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

// matrix transform from source to destination in canonical coordinates
Matrix3x3 matTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

/// transform from pixel coordinates to canonical coordinates
// Matrix3x3 matPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
//                                         double renderscaleX, //!< 0.5 for a half-resolution image
//                                         double renderscaleY,
//                                         bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
/// transform from canonical coordinates to pixel coordinates
// Matrix3x3 matCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
//                                         double renderscaleX, //!< 0.5 for a half-resolution image
//                                         double renderscaleY,
//                                         bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y

// matrix transform from destination to source, in pixel coordinates
//Matrix3x3 matInverseTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
//                                              double renderscaleX, //!< 0.5 for a half-resolution image
//                                              double renderscaleY,
//                                              bool fielded,
//                                              double translateX, double translateY,
//                                              double scaleX, double scaleY,
//                                              double skewX,
//                                              double skewY,
//                                              bool skewOrderYX,
//                                              double rads,
//                                              double centerX, double centerY);

// matrix transform from source to destination in pixel coordinates
//Matrix3x3 matTransformPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
//                                       double renderscaleX, //!< 0.5 for a half-resolution image
//                                       double renderscaleY,
//                                       bool fielded,
//                                       double translateX, double translateY,
//                                       double scaleX, double scaleY,
//                                       double skewX,
//                                       double skewY,
//                                       bool skewOrderYX,
//                                       double rads,
//                                       double centerX, double centerY);


Matrix3x3 matMul(const Matrix3x3 & m1, const Matrix3x3 & m2);

Point3D matApply(const Matrix3x3 & m,const Point3D & p);

struct Matrix4x4
{
    double data[16];

    Matrix4x4();

    Matrix4x4(const double d[16]);

    Matrix4x4(const Matrix4x4 & o);
    double & operator()(int row,int col);

    double operator()(int row,int col) const;
};

Matrix4x4 matMul(const Matrix4x4 & m1, const Matrix4x4 & m2);

Point4D matApply(const Matrix4x4 & m,const Point4D & p);

// Matrix4x4 matrix4x4FromMatrix3x3(const Matrix3x3& m);
}

#endif // TRANSFORM_H
