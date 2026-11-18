/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// Degenerate inputs that used to produce NaN or infinity.
// Finiteness is checked with MathUtils::isFinite(), which tests the bits, so that
// these tests also work when the code is compiled with -ffast-math, where
// std::isnan() and std::isfinite() are folded to constants.

#include "Global/Macros.h"

#include <cmath>
#include <limits>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include "Global/MathUtils.h"
#include "Engine/Curve.h"
#include "Engine/CurveSerialization.h"
#include "Engine/Format.h"
#include "Engine/Interpolation.h"
#include "Engine/Lut.h"
#include "Engine/Transform.h"
#include "Gui/ZoomContext.h"
#include "NaNGuards_FastMath.h"

NATRON_NAMESPACE_USING

static void
expectFiniteDerivatives(const Curve& c)
{
    KeyFrameSet keys = c.getKeyFrames_mt_safe();
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        EXPECT_TRUE( MathUtils::isFinite( it->getLeftDerivative() ) ) << "at x=" << it->getTime();
        EXPECT_TRUE( MathUtils::isFinite( it->getRightDerivative() ) ) << "at x=" << it->getTime();
    }
}

// #1267: a periodic curve (e.g. HueCorrect's) with keyframes at both ends of the period
TEST(NaNGuards, PeriodicCurveKeysAtBothEnds)
{
    const KeyframeTypeEnum types[] = { eKeyframeTypeSmooth, eKeyframeTypeCubic, eKeyframeTypeCatmullRom, eKeyframeTypeLinear, eKeyframeTypeHorizontal };

    for (KeyframeTypeEnum type : types) {
        Curve c;
        c.setXRange(0., 6.);
        c.setPeriodic(true);
        KeyFrame k0(0., 0.5);
        k0.setInterpolation(type);
        KeyFrame k6(6., 0.5);
        k6.setInterpolation(type);
        c.addKeyFrame(k0);
        c.addKeyFrame(k6);
        expectFiniteDerivatives(c);

        const double xs[] = { 0., 1e-9, 3., 5.999, 6., 6. + 1e-9, 12., -6., -1e-9, 1000. };
        for (double x : xs) {
            double v = c.getValueAt(x);
            EXPECT_TRUE( MathUtils::isFinite(v) ) << "type " << (int)type << " x=" << x;
            EXPECT_DOUBLE_EQ(0.5, v) << "type " << (int)type << " x=" << x;
        }
    }
}

TEST(NaNGuards, PeriodicCurveThreeKeys)
{
    const KeyframeTypeEnum types[] = { eKeyframeTypeSmooth, eKeyframeTypeCubic, eKeyframeTypeCatmullRom, eKeyframeTypeLinear };

    for (KeyframeTypeEnum type : types) {
        // with a key at the period
        Curve c;
        c.setXRange(0., 6.);
        c.setPeriodic(true);
        KeyFrame k[3] = { KeyFrame(0., 0.), KeyFrame(3., 1.), KeyFrame(6., 0.) };
        for (int i = 0; i < 3; ++i) {
            k[i].setInterpolation(type);
            c.addKeyFrame(k[i]);
        }
        expectFiniteDerivatives(c);

        // without a key at the period: must give the same curve
        Curve c2;
        c2.setXRange(0., 6.);
        c2.setPeriodic(true);
        for (int i = 0; i < 2; ++i) {
            c2.addKeyFrame(k[i]);
        }
        expectFiniteDerivatives(c2);

        for (double x = -6.; x <= 12.; x += 0.25) {
            double v = c.getValueAt(x);
            double v2 = c2.getValueAt(x);
            EXPECT_TRUE( MathUtils::isFinite(v) ) << "type " << (int)type << " x=" << x;
            EXPECT_TRUE( MathUtils::isFinite(v2) ) << "type " << (int)type << " x=" << x;
            // periodicity
            EXPECT_NEAR(v, c.getValueAt(x + 6.), 1e-9) << "type " << (int)type << " x=" << x;
            EXPECT_NEAR(v2, c2.getValueAt(x + 6.), 1e-9) << "type " << (int)type << " x=" << x;
            // Cubic derivatives depend on the neighbours' derivatives, which are refreshed
            // incrementally as keys are added, so the extra key changes them slightly.
            if (type != eKeyframeTypeCubic) {
                EXPECT_NEAR(v, v2, 1e-9) << "type " << (int)type << " x=" << x;
            }
        }
        EXPECT_DOUBLE_EQ( 0., c.getValueAt(6.) );
        EXPECT_DOUBLE_EQ( 1., c.getValueAt(3.) );
        EXPECT_DOUBLE_EQ( 1., c.getValueAt(9.) );
    }
}

// A curve without an owner and without a Y range is not clamped, and its range is +/-DBL_MAX
TEST(NaNGuards, CurveUnboundedRange)
{
    Curve c;

    c.addKeyFrame( KeyFrame(0., -1e300) );
    c.addKeyFrame( KeyFrame(1., 1e300) );
    Curve::YRange r = c.getCurveYRange();
    EXPECT_EQ(std::numeric_limits<double>::lowest(), r.min);
    EXPECT_EQ(std::numeric_limits<double>::max(), r.max);
    EXPECT_DOUBLE_EQ( -1e300, c.getValueAt(0.) );
    EXPECT_DOUBLE_EQ( 1e300, c.getValueAt(1.) );
    EXPECT_TRUE( MathUtils::isFinite( c.getIntegrateFromTo(0., 1.) ) );
}

TEST(NaNGuards, InterpolationDegenerateSegment)
{
    // zero-width segment
    double v = Interpolation::interpolate(1., 2., 0.5, 0.5, 1., 3., 1., eKeyframeTypeCubic, eKeyframeTypeCubic);
    EXPECT_TRUE( MathUtils::isFinite(v) );
    EXPECT_EQ(2., v);
    // (Interpolation::derive() asserts that currentTime < tnext, so it can't be tested here)
    double i = Interpolation::integrate(1., 2., 0.5, 0.5, 1., 3., 1., 1., eKeyframeTypeCubic, eKeyframeTypeCubic);
    EXPECT_EQ(0., i);

    // coincident previous key
    const KeyframeTypeEnum types[] = { eKeyframeTypeSmooth, eKeyframeTypeCubic, eKeyframeTypeCatmullRom, eKeyframeTypeLinear };
    for (KeyframeTypeEnum type : types) {
        double l, r;
        Interpolation::autoComputeDerivatives(type, type, type,
                                              0., 1.,
                                              0., 1.,
                                              3., 2.,
                                              0., 0.,
                                              &l, &r);
        EXPECT_TRUE( MathUtils::isFinite(l) ) << "type " << (int)type;
        EXPECT_TRUE( MathUtils::isFinite(r) ) << "type " << (int)type;
        // coincident next key
        Interpolation::autoComputeDerivatives(type, type, type,
                                              -3., 2.,
                                              0., 1.,
                                              0., 1.,
                                              0., 0.,
                                              &l, &r);
        EXPECT_TRUE( MathUtils::isFinite(l) ) << "type " << (int)type;
        EXPECT_TRUE( MathUtils::isFinite(r) ) << "type " << (int)type;
    }
}

TEST(NaNGuards, SolveCubicNearTripleRoot)
{
    // (x-1)^2 (x-1-e): the discriminant is slightly negative, and the argument of acos() is close to +/-1
    for (double e = 1e-3; e > 1e-12; e /= 10.) {
        double a = 1., b = 1. + e;
        // (x-a)^2 (x-b) = x^3 - (2a+b) x^2 + (a^2+2ab) x - a^2 b
        double s[3];
        int o[3];
        int n = Interpolation::solveCubic(-a * a * b, a * a + 2 * a * b, -(2 * a + b), 1., s, o);
        EXPECT_GE(n, 1);
        for (int i = 0; i < n; ++i) {
            EXPECT_TRUE( MathUtils::isFinite(s[i]) ) << "e=" << e;
            EXPECT_NEAR(1., s[i], 1e-3) << "e=" << e;
        }
    }
}

static bool
isIdentity(const Transform::Matrix3x3& m)
{
    return m.a == 1. && m.b == 0. && m.c == 0. &&
           m.d == 0. && m.e == 1. && m.f == 0. &&
           m.g == 0. && m.h == 0. && m.i == 1.;
}

TEST(NaNGuards, SingularMatrixInverse)
{
    Transform::Matrix3x3 zero(0., 0., 0., 0., 0., 0., 0., 0., 0.);
    Transform::Matrix3x3 inv = Transform::matInverse(zero);

    EXPECT_TRUE( isIdentity(inv) );

    // a zero scale is not invertible
    inv = Transform::matInverseTransformCanonical(1., 2., 0., 1., 0.1, 0.2, false, 0.3, 4., 5.);
    EXPECT_TRUE( isIdentity(inv) );

    // non-degenerate inputs are unchanged
    Transform::Matrix3x3 m = Transform::matTransformCanonical(1., 2., 2., 3., 0.1, 0.2, false, 0.3, 4., 5.);
    inv = Transform::matInverseTransformCanonical(1., 2., 2., 3., 0.1, 0.2, false, 0.3, 4., 5.);
    Transform::Matrix3x3 p = Transform::matMul(m, inv);
    EXPECT_NEAR(1., p.a, 1e-12);
    EXPECT_NEAR(0., p.b, 1e-12);
    EXPECT_NEAR(0., p.c, 1e-12);
    EXPECT_NEAR(0., p.d, 1e-12);
    EXPECT_NEAR(1., p.e, 1e-12);
    EXPECT_NEAR(0., p.f, 1e-12);
    inv = Transform::matInverse(m);
    p = Transform::matMul(m, inv);
    EXPECT_NEAR(1., p.a, 1e-12);
    EXPECT_NEAR(0., p.b, 1e-12);
    EXPECT_NEAR(1., p.e, 1e-12);
    EXPECT_NEAR(1., p.i, 1e-12);
}

TEST(NaNGuards, LutOutOfDomain)
{
    const Color::Lut* bt1886 = Color::LutManager::BT1886Lut();

    EXPECT_EQ( 0.f, bt1886->toColorSpaceFloatFromLinearFloat(-0.5f) );
    EXPECT_EQ( 0.f, bt1886->fromColorSpaceFloatToLinearFloat(-0.5f) );
    EXPECT_FLOAT_EQ( std::pow(0.5f, 1.f / 2.4f), bt1886->toColorSpaceFloatFromLinearFloat(0.5f) );

    const Color::Lut* luts[] = { Color::LutManager::CineonLut(), Color::LutManager::PanalogLut(), Color::LutManager::REDLogLut(), Color::LutManager::ViperLogLut(), Color::LutManager::BT1886Lut() };
    for (const Color::Lut* lut : luts) {
        const float vs[] = { -1e10f, -1.f, -0.5f, -0.05f, -0.01f, 0.f };
        for (float v : vs) {
            EXPECT_TRUE( MathUtils::isFinite( lut->toColorSpaceFloatFromLinearFloat(v) ) ) << lut->getName() << " v=" << v;
        }
        // the 8-bit tables are filled from the whole float range, including negative values
        lut->validate();
        EXPECT_EQ( 0, lut->toColorSpaceUint8FromLinearFloatFast(-1.f) ) << lut->getName();
    }
}

TEST(NaNGuards, FormatPixelAspectRatio)
{
    Format f(0, 0, 10, 10, "f", 0.);

    EXPECT_EQ( 1., f.getPixelAspectRatio() );
    f.setPixelAspectRatio(-2.);
    EXPECT_EQ( 1., f.getPixelAspectRatio() );
    f.setPixelAspectRatio( std::numeric_limits<double>::quiet_NaN() );
    EXPECT_EQ( 1., f.getPixelAspectRatio() );
    f.setPixelAspectRatio(2.);
    EXPECT_EQ( 2., f.getPixelAspectRatio() );
    Format g(RectI(0, 0, 10, 10), 0.);
    EXPECT_EQ( 1., g.getPixelAspectRatio() );
}

TEST(NaNGuards, ZoomContextEmptyArea)
{
    ZoomContext z(0.01, 1024.);

    z.setScreenSize(100., 100.);
    z.fill(0., 10., 0., 10.);
    const double left = z.left();
    const double bottom = z.bottom();
    const double factor = z.factor();
    const double aspect = z.aspectRatio();

    // empty areas are ignored
    z.fill(1., 1., 0., 10.);
    z.fill(0., 10., 2., 2.);
    z.fit(1., 1., 0., 10.);
    z.fit(0., 10., 2., 2.);
    EXPECT_EQ( left, z.left() );
    EXPECT_EQ( bottom, z.bottom() );
    EXPECT_EQ( factor, z.factor() );
    EXPECT_EQ( aspect, z.aspectRatio() );
}

TEST(NaNGuards, MathUtilsIsFinite)
{
    EXPECT_TRUE( MathUtils::isFinite(0.) );
    EXPECT_TRUE( MathUtils::isFinite( std::numeric_limits<double>::max() ) );
    EXPECT_TRUE( MathUtils::isFinite( std::numeric_limits<double>::denorm_min() ) );
    EXPECT_FALSE( MathUtils::isFinite( std::numeric_limits<double>::infinity() ) );
    EXPECT_FALSE( MathUtils::isFinite( -std::numeric_limits<double>::infinity() ) );
    EXPECT_FALSE( MathUtils::isFinite( std::numeric_limits<double>::quiet_NaN() ) );
    EXPECT_TRUE( MathUtils::isFinite( std::numeric_limits<float>::max() ) );
    EXPECT_FALSE( MathUtils::isFinite( std::numeric_limits<float>::infinity() ) );
    EXPECT_FALSE( MathUtils::isFinite( std::numeric_limits<float>::quiet_NaN() ) );
    EXPECT_FALSE( MathUtils::isNaN(0.f) );
    EXPECT_FALSE( MathUtils::isNaN( std::numeric_limits<float>::infinity() ) );
    EXPECT_TRUE( MathUtils::isNaN( std::numeric_limits<float>::quiet_NaN() ) );
    EXPECT_TRUE( MathUtils::isNaN( -std::numeric_limits<float>::quiet_NaN() ) );
}

// Values computed at run time, so that the compiler can't know they are NaN or infinite.
static volatile double gZero = 0.;
static volatile double gBig = std::numeric_limits<double>::max();

// MathUtils::isFinite()/isNaN() called from a TU compiled with -ffast-math (see
// NaNGuards_FastMath.cpp): without the optimization barrier, clang folds them to constants.
TEST(NaNGuards, MathUtilsWithFastMath)
{
    const double zero = gZero;
    const double nanD = zero / zero;
    const double infD = 1. / zero;
    const float nanF = (float)nanD;
    const float infF = (float)infD;

    if ( !NaNGuardsFastMath::compiledWithFastMath() ) {
        std::cout << "note: NaNGuards_FastMath.cpp was not compiled with -ffast-math" << std::endl;
    }
    EXPECT_TRUE( NaNGuardsFastMath::isFiniteD(1.) );
    EXPECT_TRUE( NaNGuardsFastMath::isFiniteD( std::numeric_limits<double>::max() ) );
    EXPECT_FALSE( NaNGuardsFastMath::isFiniteD(nanD) );
    EXPECT_FALSE( NaNGuardsFastMath::isFiniteD(infD) );
    EXPECT_FALSE( NaNGuardsFastMath::isFiniteD(-infD) );
    EXPECT_TRUE( NaNGuardsFastMath::isFiniteF(1.f) );
    EXPECT_FALSE( NaNGuardsFastMath::isFiniteF(nanF) );
    EXPECT_FALSE( NaNGuardsFastMath::isFiniteF(infF) );
    EXPECT_FALSE( NaNGuardsFastMath::isNaNF(1.f) );
    EXPECT_FALSE( NaNGuardsFastMath::isNaNF(infF) );
    EXPECT_TRUE( NaNGuardsFastMath::isNaNF(nanF) );
    const float pix[5] = { 0.f, nanF, 1.f, infF, nanF };
    EXPECT_EQ( 2, NaNGuardsFastMath::countNaN(pix, 5) );
    EXPECT_TRUE( NaNGuardsFastMath::isSumFinite(1., 2.) );
    EXPECT_FALSE( NaNGuardsFastMath::isSumFinite(gBig, gBig) ); // overflows to infinity
}

// With large times, tcur - period rounds to tcur: the search for the previous/next
// keyframe across the period must not run past the end of the keyframes (it crashed).
TEST(NaNGuards, PeriodicCurveHugeTimes)
{
    Curve c;

    c.setXRange(0., 6.);
    c.setPeriodic(true);
    c.addKeyFrame( KeyFrame(1e17, 0.25) );
    c.addKeyFrame( KeyFrame(1e17 + 64, 0.75) );
    c.addKeyFrame( KeyFrame(1e17 + 128, 0.5) );
    expectFiniteDerivatives(c);
    const double xs[] = { 0., 3., 1e17, 1e17 + 32, 1e17 + 64, 1e17 + 1e3 };
    for (double x : xs) {
        EXPECT_TRUE( MathUtils::isFinite( c.getValueAt(x) ) ) << "x=" << x;
    }
}

static std::string
saveCurve(const Curve& c)
{
    std::stringstream ss;
    {
        boost::archive::xml_oarchive oa(ss);
        oa << boost::serialization::make_nvp( "Curve", const_cast<Curve&>(c) );
    }

    return ss.str();
}

static void
loadCurve(const std::string& s,
          Curve* c)
{
    std::stringstream ss(s);
    boost::archive::xml_iarchive ia(ss);

    ia >> boost::serialization::make_nvp("Curve", *c);
}

// Projects saved before the fix for #1267 contain NaN derivatives, which Boost's
// xml_iarchive can't read: they are read as 0, and the derivatives are recomputed.
TEST(NaNGuards, CurveLoadNaNDerivatives)
{
    Curve c;

    c.addKeyFrame( KeyFrame(0., 0.) );
    c.addKeyFrame( KeyFrame(3., 1.) );
    c.addKeyFrame( KeyFrame(6., 0.) );
    const std::string good = saveCurve(c);

    // a normal curve loads unchanged
    {
        Curve c2;
        loadCurve(good, &c2);
        KeyFrameSet k1 = c.getKeyFrames_mt_safe();
        KeyFrameSet k2 = c2.getKeyFrames_mt_safe();
        ASSERT_EQ( k1.size(), k2.size() );
        for (KeyFrameSet::const_iterator i1 = k1.begin(), i2 = k2.begin(); i1 != k1.end(); ++i1, ++i2) {
            EXPECT_EQ( i1->getTime(), i2->getTime() );
            EXPECT_EQ( i1->getValue(), i2->getValue() );
            EXPECT_EQ( i1->getLeftDerivative(), i2->getLeftDerivative() );
            EXPECT_EQ( i1->getRightDerivative(), i2->getRightDerivative() );
        }
    }

    // NaN and infinite derivatives, as written by older versions
    std::string bad = std::regex_replace( good, std::regex("<LeftDerivative>[^<]*</LeftDerivative>"), "<LeftDerivative>nan</LeftDerivative>" );
    bad = std::regex_replace( bad, std::regex("<RightDerivative>[^<]*</RightDerivative>"), "<RightDerivative>-inf</RightDerivative>" );
    ASSERT_NE(good, bad);
    {
        Curve c2;
        ASSERT_NO_THROW( loadCurve(bad, &c2) );
        expectFiniteDerivatives(c2);
        // the automatic derivatives are recomputed (they depend on the order in which they are
        // refreshed, so compare with the same refresh on the original curve)
        Curve ref(c);
        ref.refreshAutomaticDerivatives();
        KeyFrameSet k1 = ref.getKeyFrames_mt_safe();
        KeyFrameSet k2 = c2.getKeyFrames_mt_safe();
        ASSERT_EQ( k1.size(), k2.size() );
        for (KeyFrameSet::const_iterator i1 = k1.begin(), i2 = k2.begin(); i1 != k1.end(); ++i1, ++i2) {
            EXPECT_NEAR( i1->getLeftDerivative(), i2->getLeftDerivative(), 1e-12 );
            EXPECT_NEAR( i1->getRightDerivative(), i2->getRightDerivative(), 1e-12 );
        }

        // the curve is usually loaded in a temporary, then cloned into the knob's curve,
        // which recomputes them with its own periodicity
        Curve p;
        p.setXRange(0., 6.);
        p.setPeriodic(true);
        p.clone(c2);
        expectFiniteDerivatives(p);
        EXPECT_DOUBLE_EQ( 0., p.getValueAt(6.) );
        EXPECT_DOUBLE_EQ( 1., p.getValueAt(3.) );
    }

#ifdef NDEBUG // KeyFrame asserts that its values are finite
    // NaN values are not written
    {
        KeyFrameSet keys = c.getKeyFrames_mt_safe();
        KeyFrame k = *keys.begin();
        k.setLeftDerivative( gZero / gZero );
        Curve c3;
        c3.setKeyframes(KeyFrameSet{ k }, false);
        const std::string s = saveCurve(c3);
        EXPECT_EQ( std::string::npos, s.find("nan") ) << s;
        Curve c4;
        ASSERT_NO_THROW( loadCurve(s, &c4) );
        expectFiniteDerivatives(c4);
    }
#endif
}
