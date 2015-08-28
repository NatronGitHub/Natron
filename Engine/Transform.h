/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef TRANSFORM_H
#define TRANSFORM_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <cmath>

class RectD;

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

 double matDeterminant(const Matrix3x3& M);

 Matrix3x3 matScaleAdjoint(const Matrix3x3& M, double s);

 Matrix3x3 matInverse(const Matrix3x3& M);
 Matrix3x3 matInverse(const Matrix3x3& M,double det);

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
Matrix3x3 matPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                         double renderscaleX, //!< 0.5 for a half-resolution image
                                         double renderscaleY,
                                         bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
/// transform from canonical coordinates to pixel coordinates
Matrix3x3 matCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                                double renderscaleX, //!< 0.5 for a half-resolution image
                                double renderscaleY,
                                bool fielded); //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5field scale in Y

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
    
void matApply(const Matrix3x3 & m,double* x, double *y, double *z);

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

// compute the bounding box of the transform of a rectangle
void transformRegionFromRoD(const RectD &srcRect, const Matrix3x3 &transform, RectD &dstRect);
    
// Matrix4x4 matrix4x4FromMatrix3x3(const Matrix3x3& m);
}

#endif // TRANSFORM_H
