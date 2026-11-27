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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <stdexcept>

#include <QString>
#include <QDir>

#include "Engine/Curve.h"
#include "Engine/Interpolation.h"

NATRON_NAMESPACE_USING

TEST(KeyFrame,
     Basic)
{
    KeyFrame k;

    k.setValue(10.);
    EXPECT_EQ( 10., k.getValue() );
    k.setTime(50.);
    EXPECT_EQ( 50., k.getTime() );
    k.setLeftDerivative(-1.);
    EXPECT_EQ( -1., k.getLeftDerivative() );
    k.setRightDerivative(-2.);
    EXPECT_EQ( -2., k.getRightDerivative() );
    k.setInterpolation(eKeyframeTypeCatmullRom);
    EXPECT_EQ( eKeyframeTypeCatmullRom, k.getInterpolation() );

    KeyFrame k1(10., 20.);
    EXPECT_NE(k, k1);
    EXPECT_TRUE( KeyFrame_compare_time() (k1, k) );
    EXPECT_TRUE( k1.getTime() < k.getTime() );
    k1.setValue(10.);
    k1.setTime(50.);
    k1.setLeftDerivative(-1.);
    k1.setRightDerivative(-2.);
    k1.setInterpolation(eKeyframeTypeCatmullRom);
    EXPECT_EQ(k, k1);
}

TEST(Curve, Basic)
{
    Curve c;

    // empty curve
    EXPECT_FALSE( c.isAnimated() );

    // one keyframe
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(0., 5.) ) );
    EXPECT_EQ( 5., c.getValueAt(0.) );
    EXPECT_EQ( 5., c.getValueAt(10.) );
    EXPECT_EQ( 5., c.getValueAt(-10.) );
    EXPECT_TRUE( c.isAnimated() );

    // two keyframes
    EXPECT_FALSE( c.addKeyFrame( KeyFrame(0., 10.) ) ); // keyframe already exists, replacing it
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(1., 20.) ) );
    EXPECT_EQ( 10., c.getValueAt(0.) );
    EXPECT_EQ( 20., c.getValueAt(1.) );
    EXPECT_EQ( 10., c.getValueAt(-10.) ); // before first keyframe
    EXPECT_EQ( 20., c.getValueAt(10.) ); // after last keyframe
    EXPECT_EQ( 15., c.getValueAt(0.5) ); // middle
    // derivative
    EXPECT_EQ( 10., c.getDerivativeAt(0.) );
    EXPECT_EQ( 10., c.getDerivativeAt(0.5) ); // middle
    EXPECT_EQ( 10., c.getDerivativeAt(0.99) );
    EXPECT_EQ( 0., c.getDerivativeAt(1.) ); // from last keyframe, it's constant

    // integrate
    EXPECT_EQ( 100., c.getIntegrateFromTo(-10., 0.) );
    EXPECT_EQ( 15., c.getIntegrateFromTo(0., 1.) );
    EXPECT_EQ( 200., c.getIntegrateFromTo(1., 11.) );
    EXPECT_EQ( 315., c.getIntegrateFromTo(-10., 11.) );

    KeyFrameSet ks = c.getKeyFrames_mt_safe();

    c.clearKeyFrames();
    // empty curve
    EXPECT_FALSE( c.isAnimated() );

    // two keyframes, constant interpolation
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(0., 10., 0., 0., eKeyframeTypeConstant) ) ); // keyframe already exists, replacing it
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(1., 20., 0., 0., eKeyframeTypeConstant) ) );
    EXPECT_EQ( 10., c.getValueAt(0.) );
    EXPECT_EQ( 20., c.getValueAt(1.) );
    EXPECT_EQ( 10., c.getValueAt(-10.) ); // before first keyframe
    EXPECT_EQ( 20., c.getValueAt(10.) ); // after last keyframe
    EXPECT_EQ( 10., c.getValueAt(0.5) ); // middle
    // derivative
    EXPECT_EQ( 0., c.getDerivativeAt(0.) );
    EXPECT_EQ( 0., c.getDerivativeAt(0.5) ); // middle
    EXPECT_EQ( 0., c.getDerivativeAt(0.99) );
    EXPECT_EQ( 0., c.getDerivativeAt(1.) ); // from last keyframe, it's constant
    // integrate
    EXPECT_EQ( 100., c.getIntegrateFromTo(-10., 0.) );
    EXPECT_EQ( 10., c.getIntegrateFromTo(0., 1.) );
    EXPECT_EQ( 200., c.getIntegrateFromTo(1., 11.) );
    EXPECT_EQ( 310., c.getIntegrateFromTo(-10., 11.) );

    KeyFrame k2(1., 20.);
}

// #1308: a periodic curve is integrated across the ends of its period
TEST(Curve, IntegratePeriodicTriangle)
{
    // a triangle wave of period 6: 0 at 0 (and 6, 12...), 1 at 3
    Curve c;

    c.setXRange(0., 6.);
    c.setPeriodic(true);
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(0., 0., 0., 0., eKeyframeTypeLinear) ) );
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(3., 1., 0., 0., eKeyframeTypeLinear) ) );

    const double eps = 1e-12;
    // within a period
    EXPECT_NEAR( 1.5, c.getIntegrateFromTo(0., 3.), eps );
    EXPECT_NEAR( 1.5, c.getIntegrateFromTo(3., 6.), eps ); // from the last keyframe to the end of the period
    EXPECT_NEAR( 3., c.getIntegrateFromTo(0., 6.), eps );
    EXPECT_NEAR( 0.5, c.getIntegrateFromTo(1., 2.), eps );
    EXPECT_NEAR( 0., c.getIntegrateFromTo(6., 6.), eps );
    // in other periods
    EXPECT_NEAR( 0.5, c.getIntegrateFromTo(7., 8.), eps );
    EXPECT_NEAR( 0.5, c.getIntegrateFromTo(-5., -4.), eps );
    EXPECT_NEAR( 1. / 6., c.getIntegrateFromTo(6., 7.), eps );
    // across the end of a period
    EXPECT_NEAR( 1. / 3., c.getIntegrateFromTo(5., 7.), eps );
    EXPECT_NEAR( 1. / 12., c.getIntegrateFromTo(-0.5, 0.5), eps );
    EXPECT_NEAR( 3., c.getIntegrateFromTo(2., 8.), eps );
    // several periods
    EXPECT_NEAR( 6., c.getIntegrateFromTo(-6., 6.), eps );
    EXPECT_NEAR( 6. + 1. / 3., c.getIntegrateFromTo(5., 19.), eps );
    EXPECT_NEAR( -6. - 1. / 3., c.getIntegrateFromTo(19., 5.), eps );
}

// #1308: the integral of a periodic curve matches the numerical integral of its values,
// whatever the interpolation, with the first keyframe after the start of the range, and with
// or without a keyframe at the end of the period
TEST(Curve, IntegratePeriodicMatchesValues)
{
    const KeyframeTypeEnum types[] = { eKeyframeTypeSmooth, eKeyframeTypeCubic, eKeyframeTypeCatmullRom, eKeyframeTypeLinear };
    const double windows[][2] = { { -7.3, 15.1 }, { 0.5, 1.5 }, { 5.5, 7.5 }, { 6.9, 7.1 }, { -2., -1. }, { 3., 9. } };

    for (KeyframeTypeEnum type : types) {
        for (int keyAtPeriod = 0; keyAtPeriod < 2; ++keyAtPeriod) {
            Curve c;
            c.setXRange(0., 6.);
            c.setPeriodic(true);
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(1., 0.2, 0., 0., type) ) );
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(2.5, 1., 0., 0., type) ) );
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(4., 0.3, 0., 0., type) ) );
            if (keyAtPeriod) {
                EXPECT_TRUE( c.addKeyFrame( KeyFrame(7., 0.2, 0., 0., type) ) );
            }
            for (const double* w : windows) {
                // midpoint rule
                const int n = 20000;
                const double h = (w[1] - w[0]) / n;
                double expected = 0.;
                for (int i = 0; i < n; ++i) {
                    expected += c.getValueAt(w[0] + (i + 0.5) * h);
                }
                expected *= h;
                const double integral = c.getIntegrateFromTo(w[0], w[1]);
                EXPECT_NEAR(expected, integral, 1e-4) << "type " << (int)type << " keyAtPeriod " << keyAtPeriod << " [" << w[0] << ", " << w[1] << "]";
                EXPECT_NEAR(-expected, c.getIntegrateFromTo(w[1], w[0]), 1e-4);
            }
        }
    }
}

// #1308: the curve editor draws a curve segment by segment with Curve::segmentAt(): for a periodic
// curve, the segment must contain the time brought into the period, and its keyframes (shifted by a
// period at the ends) must be the ones getValueAt() interpolates between.
TEST(Curve, SegmentAtPeriodic)
{
    const double period = 6.;
    const KeyframeTypeEnum types[] = { eKeyframeTypeSmooth, eKeyframeTypeLinear };

    for (KeyframeTypeEnum type : types) {
        for (int keyAtPeriod = 0; keyAtPeriod < 2; ++keyAtPeriod) {
            // the first keyframe is after the start of the X range [0, 6]
            Curve c;
            c.setXRange(0., period);
            c.setPeriodic(true);
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(1., 0.2, 0., 0., type) ) );
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(2.5, 1., 0., 0., type) ) );
            EXPECT_TRUE( c.addKeyFrame( KeyFrame(4., 0.3, 0., 0., type) ) );
            if (keyAtPeriod) {
                EXPECT_TRUE( c.addKeyFrame( KeyFrame(1. + period, 0.2, 0., 0., type) ) );
            }
            const KeyFrameSet keys = c.getKeyFrames_mt_safe();
            for (double x = -13.; x <= 13.; x += 0.125) {
                const Curve::Segment s = Curve::segmentAt(keys, true, 0., period, x);
                // x brought into the period that starts at the first keyframe
                EXPECT_GE(s.t, 1.) << "x=" << x;
                EXPECT_LT(s.t, 1. + period) << "x=" << x;
                EXPECT_NEAR( 0., std::remainder(s.t - x, period), 1e-9 ) << "x=" << x;
                // the segment contains it
                EXPECT_LE(s.tcur, s.t) << "x=" << x;
                EXPECT_LT(s.t, s.tnext) << "x=" << x;
                // and gives the curve's value
                EXPECT_NEAR( c.getValueAt(x),
                             Interpolation::interpolate(s.tcur, s.vcur, s.vcurDerivRight, s.vnextDerivLeft, s.tnext, s.vnext, s.t, s.interp, s.interpNext),
                             1e-9 ) << "x=" << x;
                // the next keyframe, as the curve editor places it (x + (tnext - t)), is a keyframe of the curve
                const double xNext = x + (s.tnext - s.t);
                EXPECT_NEAR( s.vnext, c.getValueAt(xNext), 1e-9 ) << "x=" << x;
                // the keyframe before, too
                const double xPrev = x + (s.tcur - s.t);
                EXPECT_NEAR( s.vcur, c.getValueAt(xPrev), 1e-9 ) << "x=" << x;
            }
        }
    }
}

// The segments of a non-periodic curve are the keyframes around the time, unchanged.
TEST(Curve, SegmentAtNonPeriodic)
{
    Curve c;

    EXPECT_TRUE( c.addKeyFrame( KeyFrame(1., 0.2, 0., 0., eKeyframeTypeLinear) ) );
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(3., 1., 0., 0., eKeyframeTypeLinear) ) );
    const KeyFrameSet keys = c.getKeyFrames_mt_safe();
    const Curve::Segment s = Curve::segmentAt(keys, false, 0., 6., 2.);
    EXPECT_EQ(2., s.t);
    EXPECT_EQ(1., s.tcur);
    EXPECT_EQ(0.2, s.vcur);
    EXPECT_EQ(3., s.tnext);
    EXPECT_EQ(1., s.vnext);
    // a periodic curve with an unbounded X range is not wrapped
    const Curve::Segment u = Curve::segmentAt(keys, true, std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(), 13.);
    EXPECT_EQ(13., u.t);
    EXPECT_EQ(3., u.tcur);
    EXPECT_THROW( (void)Curve::segmentAt(KeyFrameSet(), false, 0., 6., 2.), std::invalid_argument );
}
