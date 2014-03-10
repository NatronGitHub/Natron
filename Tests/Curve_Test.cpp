//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <gtest/gtest.h>

#include <QString>
#include <QDir>

#include "Engine/Curve.h"

TEST(KeyFrame,Basic)
{
    KeyFrame k;
    k.setValue(10.);
    EXPECT_EQ(10.,k.getValue());
    k.setTime(50.);
    EXPECT_EQ(50.,k.getTime());
    k.setLeftDerivative(-1.);
    EXPECT_EQ(-1.,k.getLeftDerivative());
    k.setRightDerivative(-2.);
    EXPECT_EQ(-2.,k.getRightDerivative());
    k.setInterpolation(Natron::KEYFRAME_CATMULL_ROM);
    EXPECT_EQ(Natron::KEYFRAME_CATMULL_ROM, k.getInterpolation());

    KeyFrame k1(10.,20.);
    EXPECT_NE(k,k1);
    EXPECT_TRUE(KeyFrame_compare_time()(k1,k));
    EXPECT_TRUE(k1.getTime() < k.getTime());
    k1.setValue(10.);
    k1.setTime(50.);
    k1.setLeftDerivative(-1.);
    k1.setRightDerivative(-2.);
    k1.setInterpolation(Natron::KEYFRAME_CATMULL_ROM);
    EXPECT_EQ(k,k1);

}

TEST(Curve,Basic)
{
    Curve c;
    // empty curve
    EXPECT_FALSE(c.isAnimated());

    // one keyframe
    EXPECT_TRUE(c.addKeyFrame(KeyFrame(0.,5.)));
    EXPECT_EQ(5., c.getValueAt(0.));
    EXPECT_EQ(5., c.getValueAt(10.));
    EXPECT_EQ(5., c.getValueAt(-10.));
    EXPECT_TRUE(c.isAnimated());

    // two keyframes
    EXPECT_FALSE(c.addKeyFrame(KeyFrame(0.,10.))); // keyframe already exists, replacing it
    EXPECT_TRUE(c.addKeyFrame(KeyFrame(1.,20.)));
    EXPECT_EQ(10., c.getValueAt(0.));
    EXPECT_EQ(20., c.getValueAt(1.));
    EXPECT_EQ(10., c.getValueAt(-10.)); // before first keyframe
    EXPECT_EQ(20., c.getValueAt(10.)); // after last keyframe
    EXPECT_EQ(15., c.getValueAt(0.5)); // middle

    // derivative
    EXPECT_EQ(10., c.getDerivativeAt(0.));
    EXPECT_EQ(10., c.getDerivativeAt(0.5)); // middle
    EXPECT_EQ(10., c.getDerivativeAt(0.99));
    EXPECT_EQ(0., c.getDerivativeAt(1.)); // from last keyframe, it's constant

    // integrate
    EXPECT_EQ(100., c.getIntegrateFromTo(-10., 0.));
    EXPECT_EQ(15., c.getIntegrateFromTo(0., 1.));
    EXPECT_EQ(200., c.getIntegrateFromTo(1., 11.));
    EXPECT_EQ(315., c.getIntegrateFromTo(-10., 11.));

    KeyFrame k2(1., 20.);

}


