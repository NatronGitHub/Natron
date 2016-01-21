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

#include <gtest/gtest.h>

#include <QString>
#include <QDir>

#include "Engine/Curve.h"

NATRON_NAMESPACE_USING

TEST(KeyFrame,Basic)
{
    KeyFrame k;

    k.setValue(10.);
    EXPECT_EQ( 10.,k.getValue() );
    k.setTime(50.);
    EXPECT_EQ( 50.,k.getTime() );
    k.setLeftDerivative(-1.);
    EXPECT_EQ( -1.,k.getLeftDerivative() );
    k.setRightDerivative(-2.);
    EXPECT_EQ( -2.,k.getRightDerivative() );
    k.setInterpolation(eKeyframeTypeCatmullRom);
    EXPECT_EQ( eKeyframeTypeCatmullRom, k.getInterpolation() );

    KeyFrame k1(10.,20.);
    EXPECT_NE(k,k1);
    EXPECT_TRUE( KeyFrame_compare_time() (k1,k) );
    EXPECT_TRUE( k1.getTime() < k.getTime() );
    k1.setValue(10.);
    k1.setTime(50.);
    k1.setLeftDerivative(-1.);
    k1.setRightDerivative(-2.);
    k1.setInterpolation(eKeyframeTypeCatmullRom);
    EXPECT_EQ(k,k1);
}

TEST(Curve,Basic)
{
    Curve c;

    // empty curve
    EXPECT_FALSE( c.isAnimated() );

    // one keyframe
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(0.,5.) ) );
    EXPECT_EQ( 5., c.getValueAt(0.) );
    EXPECT_EQ( 5., c.getValueAt(10.) );
    EXPECT_EQ( 5., c.getValueAt(-10.) );
    EXPECT_TRUE( c.isAnimated() );

    // two keyframes
    EXPECT_FALSE( c.addKeyFrame( KeyFrame(0.,10.) ) ); // keyframe already exists, replacing it
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(1.,20.) ) );
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
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(0.,10.,0.,0.,eKeyframeTypeConstant) ) ); // keyframe already exists, replacing it
    EXPECT_TRUE( c.addKeyFrame( KeyFrame(1.,20.,0.,0.,eKeyframeTypeConstant) ) );
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


