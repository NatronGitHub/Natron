/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <cmath>

#include "Engine/EngineFwd.h"


#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

NATRON_NAMESPACE_ENTER
namespace Transform {
inline double
toDegrees(double rad)
{
    rad = rad * 180.0 / M_PI;

    return rad;
}

inline double
toRadians(double deg)
{
    deg = deg * M_PI / 180.0;

    return deg;
}

struct Point3D
{
    double x, y, z;

    Point3D();

    Point3D(double x,
            double y,
            double z);

    Point3D(const Point3D & p);

    Point3D& operator=(const Point3D& other);

    bool operator==(const Point3D & other) const;

};


/**
 * \brief Compute the cross-product of two vectors
 *
 */
inline Point3D
crossprod(const Point3D & a,
          const Point3D & b)
{
    Point3D c;

    c.x = a.y * b.z - a.z * b.y;
    c.y = a.z * b.x - a.x * b.z;
    c.z = a.x * b.y - a.y * b.x;

    return c;
}

struct Point4D
{
    double x, y, z, w;

    Point4D();

    Point4D(double x,
            double y,
            double z,
            double w);

    Point4D(const Point4D & o);

    Point4D& operator=(const Point4D& other);

    double & operator() (int i);
    const double& operator() (int i) const;

    bool operator==(const Point4D & o) const;
};

/**
 * @brief A simple 3 * 3 matrix class laid out as such:
 *  a b c
 *  d e f
 *  g h i
 **/
struct Matrix3x3
{
    double a, b, c, d, e, f, g, h, i;

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

    /// Construct from columns
    Matrix3x3(const Point3D &m0,
              const Point3D &m1,
              const Point3D &m2)
    {
        a = m0.x; b = m1.x; c = m2.x;
        d = m0.y; e = m1.y; f = m2.y;
        g = m0.z; h = m1.z; i = m2.z;
    }

    bool isIdentity() const;

    void setIdentity();

    /**
     * \brief Compute a homography from 4 points correspondences
     * \param p1 source point
     * \param p2 source point
     * \param p3 source point
     * \param p4 source point
     * \param q1 target point
     * \param q2 target point
     * \param q3 target point
     * \param q4 target point
     * \return the homography matrix that maps pi's to qi's
     *
       Using four point-correspondences pi ↔ pi^, we can set up an equation system to solve for the homography matrix H.
       An algorithm to obtain these parameters requiring only the inversion of a 3 × 3 equation system is as follows.
       From the four point-correspondences pi ↔ pi^ with (i ∈ {1, 2, 3, 4}),
       compute h1 = (p1 × p2 ) × (p3 × p4 ), h2 = (p1 × p3 ) × (p2 × p4 ), h3 = (p1 × p4 ) × (p2 × p3 ).
       Also compute h1^ , h2^ , h3^ using the same principle from the points pi^.
       Now, the homography matrix H can be obtained easily from
       H · [h1 h2 h3] = [h1^ h2^ h3^],
       which only requires the inversion of the matrix [h1 h2 h3].

       Algo from:
       http://www.dirk-farin.net/publications/phd/text/AB_EfficientComputationOfHomographiesFromFourCorrespondences.pdf
     */
    bool setHomographyFromFourPoints(const Point3D &p1,
                                     const Point3D &p2,
                                     const Point3D &p3,
                                     const Point3D &p4,
                                     const Point3D &q1,
                                     const Point3D &q2,
                                     const Point3D &q3,
                                     const Point3D &q4);

    bool setAffineFromThreePoints(const Point3D &p1,
                                  const Point3D &p2,
                                  const Point3D &p3,
                                  const Point3D &q1,
                                  const Point3D &q2,
                                  const Point3D &q3);

    bool setSimilarityFromTwoPoints(const Point3D &p1,
                                    const Point3D &p2,
                                    const Point3D &q1,
                                    const Point3D &q2);

    bool setTranslationFromOnePoint(const Point3D &p1,
                                    const Point3D &q1);
};

double matDeterminant(const Matrix3x3& M);

Matrix3x3 matScaleAdjoint(const Matrix3x3& M, double s);

Matrix3x3 matInverse(const Matrix3x3& M);
Matrix3x3 matInverse(const Matrix3x3& M, double det);

Matrix3x3 matRotation(double rads);
// Matrix3x3 matRotationAroundPoint(double rads, double pointX, double pointY);

// Matrix3x3 matTranslation(double translateX, double translateY);

Matrix3x3 matScale(double scaleX, double scaleY);
// Matrix3x3 matScale(double scale);
// Matrix3x3 matScaleAroundPoint(double scaleX, double scaleY, double pointX, double pointY);

Matrix3x3 matSkewXY(double skewX, double skewY, bool skewOrderYX);

// matrix transform from destination to source, in canonical coordinates
Matrix3x3 matInverseTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

// matrix transform from source to destination in canonical coordinates
Matrix3x3 matTransformCanonical(double translateX, double translateY, double scaleX, double scaleY, double skewX, double skewY, bool skewOrderYX, double rads, double centerX, double centerY);

/// transform from pixel coordinates to canonical coordinates
Matrix3x3 matPixelToCanonical(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                              double renderscaleX,            //!< 0.5 for a half-resolution image
                              double renderscaleY,
                              bool fielded);            //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5 field scale in Y
/// transform from canonical coordinates to pixel coordinates
Matrix3x3 matCanonicalToPixel(double pixelaspectratio, //!< 1.067 for PAL, where 720x576 pixels occupy 768x576 in canonical coords
                              double renderscaleX,   //!< 0.5 for a half-resolution image
                              double renderscaleY,
                              bool fielded);   //!< true if the image property kOfxImagePropField is kOfxImageFieldLower or kOfxImageFieldUpper (apply 0.5field scale in Y

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

Point3D matApply(const Matrix3x3 & m, const Point3D & p);

void matApply(const Matrix3x3 & m, double* x, double *y, double *z);

struct Matrix4x4
{
    double data[16];

    Matrix4x4();

    Matrix4x4(const double d[16]);

    Matrix4x4(const Matrix4x4 & o);
    double & operator()(int row, int col);

    double operator()(int row, int col) const;
};

Matrix4x4 matMul(const Matrix4x4 & m1, const Matrix4x4 & m2);

Point4D matApply(const Matrix4x4 & m, const Point4D & p);

// compute the bounding box of the transform of a rectangle
void transformRegionFromRoD(const RectD &srcRect, const Matrix3x3 &transform, RectD &dstRect);

// Matrix4x4 matrix4x4FromMatrix3x3(const Matrix3x3& m);
} // namespace Transform
NATRON_NAMESPACE_EXIT

#endif // TRANSFORM_H
