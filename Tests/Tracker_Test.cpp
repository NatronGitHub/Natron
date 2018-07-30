/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <vector>
#include <cmath>
#include <cstdlib>

#include <gtest/gtest.h>

#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_OFF(maybe-uninitialized)
#endif
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <openMVG/robust_estimation/robust_estimator_Prosac.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_ON(maybe-uninitialized)
#endif

#include "Engine/EngineFwd.h"
#include "Engine/Transform.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_USING

using namespace openMVG::robust;

static void
padd(std::vector<Point>& v,
     double x,
     double y)
{
    v.resize(v.size() + 1);
    Point& p = v.back();
    p.x = x;
    p.y = y;
}

static void
throwProsacError(ProsacReturnCodeEnum c,
                 int nMinSamples)
{
    switch (c) {
    case openMVG::robust::eProsacReturnCodeFoundModel:
        break;
    case openMVG::robust::eProsacReturnCodeInliersIsMinSamples:
        break;
    case openMVG::robust::eProsacReturnCodeNoModelFound:
        throw std::runtime_error("Could not find a model for the given correspondences.");
        break;
    case openMVG::robust::eProsacReturnCodeNotEnoughPoints: {
        std::stringstream ss;
        ss << "This model requires a minimum of ";
        ss << nMinSamples;
        ss << " correspondences.";
        throw std::runtime_error( ss.str() );
        break;
    }
    case openMVG::robust::eProsacReturnCodeMaxIterationsFromProportionParamReached:
        throw std::runtime_error("Maximum iterations computed from outliers proportion reached");
        break;
    case openMVG::robust::eProsacReturnCodeMaxIterationsParamReached:
        throw std::runtime_error("Maximum solver iterations reached");
        break;
    }
}

static void
makeMatFromVectorOfPoints(const std::vector<Point>& x,
                          openMVG::Mat& M)
{
    M.resize( 2, x.size() );
    for (std::size_t i = 0; i < x.size(); ++i) {
        M(0, i) = x[i].x;
        M(1, i) = x[i].y;
    }
}

template <typename MODELTYPE>
void
runProsacForModel(const std::vector<Point>& x1,
                  const std::vector<Point>& x2,
                  int w1,
                  int h1,
                  int w2,
                  int h2,
                  typename MODELTYPE::Model* foundModel,
                  InliersVec* inliers)
{
    typedef ProsacKernelAdaptor<MODELTYPE> KernelType;

    assert( x1.size() == x2.size() );
    openMVG::Mat M1( 2, x1.size() ), M2( 2, x2.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        M1(0, i) = x1[i].x;
        M1(1, i) = x1[i].y;

        M2(0, i) = x2[i].x;
        M2(1, i) = x2[i].y;
    }
    KernelType kernel(M1, w1, h1, M2, w2, h2);
    ProsacReturnCodeEnum ret = prosac(kernel, foundModel, inliers);
    throwProsacError( ret, KernelType::MinimumSamples() );
}

template <typename MODELTYPE>
int
Score(const typename MODELTYPE::Model & model,
      const openMVG::Mat& x1,
      const openMVG::Mat& x2,
      double normalizedSigma,
      InliersVec* isInlier)
{
    assert( (int)isInlier->size() == x1.cols() );

    double inlierThreshold = InlierThreshold<MODELTYPE::CODIMENSION>(normalizedSigma);
    int nbInliers = 0;
    for (std::size_t j = 0; j < isInlier->size(); ++j) {
        double error = MODELTYPE::Error( model, x1.col(j), x2.col(j) );
        if (error < inlierThreshold) {
            ++nbInliers;
            (*isInlier)[j] = true;
        } else {
            (*isInlier)[j] = false;
        }
    }

    return nbInliers;
}

TEST(ModelSearch, TranslationMinimal)
{
    const int w1 = 500;
    const int h1 = 500;
    const int w2 = 600;
    const int h2 = 600;
    const double precision = 0.01;
    std::vector<Point> x1, x2;

    padd(x1, 50, 50);
    padd(x2, 300, 100);

    openMVG::Vec2 model;
    try {
        runProsacForModel<openMVG::robust::Translation2DSolver>(x1, x2, w1, h1, w2, h2, &model, 0);
    } catch (...) {
        ASSERT_TRUE(false);
    }

    EXPECT_NEAR(model(0), 250, precision);
    EXPECT_NEAR(model(1), 50, precision);
}

TEST(ModelSearch, TranslationNPoints)
{
    const int w1 = 500;
    const int h1 = 500;
    const int w2 = 600;
    const int h2 = 600;
    const double precision = 0.01;
    std::vector<Point> x1, x2;

    padd(x1, 50, 50);
    padd(x1, 50, 60);
    padd(x1, 50, 70);
    padd(x1, 50, 80);

    padd(x2, 300, 50);
    padd(x2, 300, 60);
    padd(x2, 300, 70);
    padd(x2, 300, 80);

    openMVG::Vec2 model;
    try {
        runProsacForModel<openMVG::robust::Translation2DSolver>(x1, x2, w1, h1, w2, h2, &model, 0);
    } catch (...) {
        ASSERT_TRUE(false);
    }

    EXPECT_NEAR(model(0), 250, precision);
    EXPECT_NEAR(model(1), 0, precision);
}

static openMVG::Vec4
makeSimilarity(double rotationDegrees,
               double scale,
               double tx,
               double ty)
{
    openMVG::Vec4 r;
    double rotRadi = Transform::toRadians(rotationDegrees);
    double sinAlpha = std::sin(rotRadi);
    double cosAlpha = std::cos(rotRadi);

    r(0) = tx;
    r(1) = ty;
    r(2) = scale * sinAlpha;
    r(3) = scale * cosAlpha;

    return r;
}

static void
similarityApply(const openMVG::Vec4& S,
                const openMVG::Vec2& X1,
                openMVG::Vec2* X2)
{
    openMVG::Mat2 m;

    m(0, 0) = S(3); m(0, 1) = -S(2);
    m(1, 0) = S(2); m(1, 1) = S(3);
    openMVG::Vec2 t;
    t(0) = S(0);
    t(1) = S(1);
    *X2 = (m * X1) + t;
}

static Point
similarityApply(const openMVG::Vec4& S,
                const Point& x1)
{
    openMVG::Vec2 p1;

    p1(0) = x1.x;
    p1(1) = x1.y;

    openMVG::Vec2 p2;
    similarityApply(S, p1, &p2);
    openMVG::Mat2 m;
    Point ret;
    ret.x = p2(0);
    ret.y = p2(1);

    return ret;
}

static void
computeSimilarity(int w1,
                  int h1,
                  int w2,
                  int h2,
                  const openMVG::Vec4& S,
                  const std::vector<Point>& x1,
                  std::vector<Point>* x2,
                  openMVG::Vec4* model,
                  InliersVec* inliers)
{
    x2->resize( x1.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        (*x2)[i] = similarityApply(S, x1[i]);
        {
            openMVG::Vec2 x2_tmp;
            x2_tmp(0) = (*x2)[i].x;
            x2_tmp(1) = (*x2)[i].y;


            // Check that the similarity applies correctly
            openMVG::Vec2 t;
            t(0) = S(0);
            t(1) = S(1);
            openMVG::Vec2 cRot;
            double ccosalpha = S(3);
            double csinalpha = S(2);
            cRot(0) = ccosalpha * x1[i].x - csinalpha * x1[i].y;
            cRot(1) = csinalpha * x1[i].x + ccosalpha * x1[i].y;
            openMVG::Vec2 x2err = x2_tmp - t - cRot;
            EXPECT_NEAR(x2err(0), 0., 5e-8);
            EXPECT_NEAR(x2err(1), 0., 5e-8);
        }
    }

    runProsacForModel<openMVG::robust::Similarity2DSolver>(x1, *x2, w1, h1, w2, h2, model, inliers);
}

static void
testSimilarity(const std::vector<Point>& x1)
{
    const int w1 = 500;
    const int h1 = 500;
    const int w2 = 600;
    const int h2 = 600;
    const Point iTranslation = {50, 100};
    const double iScale = 1.2;
    const double iRotation = 45.;
    const openMVG::Vec4 S = makeSimilarity(iRotation, iScale, iTranslation.x, iTranslation.y);
    std::vector<Point> x2;
    openMVG::Vec4 foundSimilarity;
    InliersVec inliers;

    try {
        computeSimilarity(w1, h1, w2, h2, S, x1, &x2, &foundSimilarity, &inliers);
    } catch (...) {
        ASSERT_TRUE(false);
    }

    ASSERT_TRUE( x1.size() == x2.size() );


    int originalInliers = std::accumulate(inliers.begin(), inliers.end(), 0);
    EXPECT_EQ( originalInliers, (int)x1.size() );

    openMVG::Mat M1, M2;
    makeMatFromVectorOfPoints(x1, M1);
    makeMatFromVectorOfPoints(x2, M2);

    // We don't normalize when using minimal points
    double sigma = x1.size() == openMVG::robust::Similarity2DSolver::MinimumSamples() ? 1. : 1. / (double)(w1 * h1);
    std::vector<bool> isInlier( x1.size() );
    int nbInliers = Score<openMVG::robust::Similarity2DSolver>(foundSimilarity, M1, M2, sigma, &isInlier);
    EXPECT_TRUE(originalInliers == nbInliers);
    for (std::size_t i = 0; i < isInlier.size(); ++i) {
        EXPECT_TRUE(isInlier[i]);
    }
}

TEST(ModelSearch, SimilarityMinimal)
{
    std::vector<Point> x1;

    padd(x1, 50, 50);
    padd(x1, 70, 100);
    testSimilarity(x1);
}

TEST(ModelSearch, SimilarityNPoints)
{
    std::vector<Point> x1;

    padd(x1, 50, 50);
    padd(x1, 70, 100);
    padd(x1, 20, 87);
    padd(x1, 18, 10);
    padd(x1, 23, 32);
    padd(x1, 44, 75);
    testSimilarity(x1);
}


static void
computeHomography(int w1,
                  int h1,
                  int w2,
                  int h2,
                  const openMVG::Mat3& H,
                  const double outliersProp,
                  std::vector<Point>& x1,
                  std::vector<Point>* x2,
                  int* nbOutliers,
                  openMVG::Mat3* model,
                  InliersVec* inliers)
{
    x2->resize( x1.size() );

    // Compute the x2 points from the ground truth homography using homogeneous coords.
    for (std::size_t i = 0; i < x1.size(); ++i) {
        openMVG::Vec3 v;
        v(0) = x1[i].x;
        v(1) = x1[i].y;
        v(2) = 1.;
        openMVG::Vec3 u = H * v;
        (*x2)[i].x = u(0);
        (*x2)[i].y = u(1);
    }

    assert(outliersProp >= 0 && outliersProp <= 1.);
    *nbOutliers = x1.size() * outliersProp;
    assert( *nbOutliers < (int)x1.size() );

    // Introduce outliers in x1, set them so that there is a better probability to have inliers in the start of the point set so that Prosac works well

    // This is the probability that a sample is a outliers, the more we reach the end, the more it increases
    *nbOutliers = 0;
    for (std::size_t i = 0; i < x1.size(); ++i) {
        double prob = (outliersProp * i) / (double)x1.size();
        double s = std::rand() % 100;
        bool isOutlier = s <= (prob * 100);
        if (isOutlier) {
            x1[i].x = x1[i].x + i * 5.5;
            x1[i].y = x1[i].y + 7.8;
            *nbOutliers = *nbOutliers + 1;
        }
    }

    runProsacForModel<openMVG::robust::Homography2DSolver>(x1, *x2, w1, h1, w2, h2, model, inliers);
}

static void
testHomography(std::vector<Point>& x1)
{
    const int w1 = 500;
    const int h1 = 500;
    const int w2 = 600;
    const int h2 = 600;
    openMVG::Mat3 H;

    H << 1, 0, -4,
        0, 1,  5,
        0, 0,  1;

    const double outliersProp = x1.size() == 4 ? 0. : 0.2;
    int nbOutliers;
    std::vector<Point> x2;
    openMVG::Mat3 foundModel;
    InliersVec inliers;
    try {
        computeHomography(w1, h1, w2, h2, H, outliersProp, x1, &x2, &nbOutliers, &foundModel, &inliers);
    } catch (...) {
        ASSERT_TRUE(false);
    }

    ASSERT_TRUE( x1.size() == x2.size() );


    int nbInliers = std::accumulate(inliers.begin(), inliers.end(), 0);
    EXPECT_EQ(nbInliers, (int)x1.size() - nbOutliers);

    // Check the found model
    for (int i = 0; i < H.rows(); ++i) {
        for (int j = 0; j < H.cols(); ++j) {
            EXPECT_NEAR(H(i, j), foundModel(i, j), 1e-5);
        }
    }
}

TEST(ModelSearch, HomographyMinimal)
{
    std::vector<Point> x1;

    padd(x1, 50, 50);
    padd(x1, 240, 60);
    padd(x1, 223, 342);
    padd(x1, 13, 310);
    testHomography(x1);
}

TEST(ModelSearch, HomographyNPoints)
{
    std::vector<Point> x1;
    const int n = 1000;

    x1.resize(n);

    std::srand(2000);
    for (int i = 0; i < n; ++i) {
        x1[i].x = std::rand() % 500 + 1;
        x1[i].y = std::rand() % 500 + 1;
    }
    testHomography(x1);
}
