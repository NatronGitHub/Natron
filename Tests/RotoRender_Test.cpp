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

// What the Roto node renders, compared with the expected alpha:
// - the feather ramp of a shape, outward and inward (#157);
// - overlapping feathers (#147);
// - the Mask input, with and without Bg input (#367).
//
// With a feather fall-off of 1, the alpha of a feather of width w at the distance d from the opaque edge is
// (1 - d / w)^2 (the feather patches interpolate 1 - d / w, and RotoContextPrivate::renderBezier() squares it).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Bezier.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/ParallelRenderArgs.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoItem.h"
#include "Engine/TimeLine.h"

#include "TestUtils.h"

NATRON_NAMESPACE_USING

namespace {
const double kTime = 1.;
const int kMaskInput = 10; // the Mask input of Roto and RotoPaint (ROTOPAINT_MASK_INPUT_INDEX)
// The rasterization of the feather patches (8 bits, sampled at pixel centers or close)
const double kRampTolerance = 0.02;

// The alpha channel of a rendered image (0 outside its bounds)
class Alpha
{
public:
    explicit Alpha(const ImagePtr& image)
        : _image(image)
    {
    }

    double at(int x,
              int y) const
    {
        if ( !_image || !_image->getBounds().contains(x, y) ) {
            return 0.;
        }
        Image::ReadAccess access = _image->getReadRights();
        const float* pixel = reinterpret_cast<const float*>( access.pixelAt(x, y) );
        const int nComps = (int)_image->getComponentsCount();

        return pixel[nComps - 1];
    }

private:
    ImagePtr _image;
};

// Renders the output of node at kTime, view 0 and scale 1, as an interactive render: a null image if the render
// failed.
ImagePtr
renderFrame(const NodePtr& node)
{
    EffectInstancePtr effect = node->getEffectInstance();
    const ViewIdx view(0);
    const unsigned int mipmapLevel = 0;
    const RenderScale scale = RenderScale::fromMipmapLevel(mipmapLevel);
    RectD rod;
    bool isProjectFormat;
    StatusEnum stat = effect->getRegionOfDefinition_public(node->getHashValue(), kTime, scale, view, &rod, &isProjectFormat);

    if ( (stat == eStatusFailed) || rod.isNull() ) {
        return ImagePtr();
    }
    const RectI renderWindow = rod.toPixelEnclosing( mipmapLevel, effect->getAspectRatio(-1) );
    RenderingFlagSetter flagIsRendering(node);
    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    ParallelRenderArgsSetter frameRenderArgs( kTime, view,
                                              true, // isRenderUserInteraction
                                              false, // isSequential
                                              abortInfo,
                                              node, // tree root
                                              0, // texture index
                                              node->getApp()->getTimeLine().get(),
                                              NodePtr(), // rotoPaint node
                                              false, // isAnalysis
                                              false, // isDraft
                                              RenderStatsPtr() );
    FrameRequestMap request;
    stat = EffectInstance::computeRequestPass(kTime, view, mipmapLevel, rod, node, request);
    if (stat == eStatusFailed) {
        return ImagePtr();
    }
    frameRenderArgs.updateNodesRequest(request);

    std::list<ImagePlaneDesc> planes;
    {
        ImagePlaneDesc plane, pairedPlane;
        effect->getMetadataComponents(-1, &plane, &pairedPlane);
        planes.push_back(plane);
    }
    const EffectInstance::RenderRoIArgs renderArgs( kTime, scale, mipmapLevel, view,
                                                    true, // byPassCache
                                                    renderWindow, rod, planes,
                                                    effect->getBitDepth(-1),
                                                    false, // calledFromGetImage
                                                    effect.get(),
                                                    eStorageModeRAM, // returnStorage
                                                    kTime ); // callerRenderTime
    std::map<ImagePlaneDesc, ImagePtr> outputPlanes;
    try {
        if ( (effect->renderRoI(renderArgs, &outputPlanes) != EffectInstance::eRenderRoIRetCodeOk) || outputPlanes.empty() ) {
            return ImagePtr();
        }
    } catch (const std::exception& e) {
        std::cerr << "renderFrame: " << e.what() << std::endl;

        return ImagePtr();
    }

    return outputPlanes.begin()->second;
} // renderFrame

struct XY
{
    double x;
    double y;
};

// A closed polygonal shape with the given feather distance. Its feather points are the control points, or feather
// if it is not empty.
BezierPtr
makePolygon(const RotoContextPtr& context,
            const std::vector<XY>& points,
            double featherDistance,
            const std::vector<XY>& feather = std::vector<XY>())
{
    BezierPtr shape = context->makeBezier(points[0].x, points[0].y, kRotoBezierBaseName, kTime, false);

    if (!shape) {
        return shape;
    }
    for (std::size_t i = 1; i < points.size(); ++i) {
        shape->addControlPoint(points[i].x, points[i].y, kTime);
    }
    shape->setCurveFinished(true);
    // the shape is animated (auto-keying): set the value of its keyframe
    shape->getFeatherKnob()->setValueAtTime(kTime, featherDistance, ViewSpec::all(), 0);
    EXPECT_EQ( featherDistance, shape->getFeatherDistance(kTime) );
    for (std::size_t i = 0; i < feather.size(); ++i) {
        shape->setPointAtIndex(true /*feather*/, (int)i, kTime, feather[i].x, feather[i].y, feather[i].x, feather[i].y, feather[i].x, feather[i].y);
    }

    return shape;
}

// The alpha of a feather of the given width at the distance d from its opaque edge (see the top of this file)
double
featherRamp(double d,
            double width)
{
    const double v = std::min( std::max(1. - d / width, 0.), 1. );

    return v * v;
}

class RotoRenderTest
    : public testing::Test
{
protected:
    virtual void SetUp() OVERRIDE FINAL
    {
        // Every shape is rendered by OpenFX nodes (see RotoDrawableItem::createNodes()).
        _havePlugins = NatronTest::requirePlugins({ PLUGINID_OFX_MERGE, PLUGINID_OFX_ROTO, PLUGINID_OFX_CONSTANT }, "openfx-misc");
    }

    // A Roto node, destroyed at the end of the test
    NodePtr makeRoto()
    {
        return _nodes.create(PLUGINID_NATRON_ROTO);
    }

    NatronTest::ProjectResetScope _projectReset;
    NatronTest::NodeScope _nodes;
    bool _havePlugins = false;
};

// The 300 px square from (100, 100) to (400, 400)
const std::vector<XY> kSquare = { {100., 400.}, {400., 400.}, {400., 100.}, {100., 100.} };
} // anonymous namespace

// A feather pulled outward, 50 px around the square: opaque inside, fading out to the feather (the reference for the
// inward feather below).
TEST_F(RotoRenderTest, OutwardFeatherRamp)
{
    if (!_havePlugins) {
        return;
    }
    NodePtr roto = makeRoto();
    ASSERT_TRUE( bool(roto) );
    ASSERT_TRUE( bool( makePolygon(roto->getRotoContext(), kSquare, 0., { {50., 450.}, {450., 450.}, {450., 50.}, {50., 50.} }) ) );
    const Alpha alpha( renderFrame(roto) );

    const int y = 250;
    EXPECT_EQ(1., alpha.at(250, y));
    EXPECT_EQ(1., alpha.at(101, y));
    EXPECT_EQ(0., alpha.at(45, y));
    double previous = 0.;
    for (int x = 50; x < 100; ++x) {
        const double a = alpha.at(x, y);
        EXPECT_NEAR(featherRamp(100. - (x + 0.5), 50.), a, kRampTolerance) << "x=" << x;
        EXPECT_GE(a, previous) << "x=" << x;
        previous = a;
    }
}

// #157: a feather pulled inward, 50 px inside the square. The shape is opaque inside the feather, and fades out from
// there to the shape, which is transparent; it was opaque up to the shape.
TEST_F(RotoRenderTest, InwardFeatherRamp)
{
    if (!_havePlugins) {
        return;
    }
    NodePtr roto = makeRoto();
    ASSERT_TRUE( bool(roto) );
    ASSERT_TRUE( bool( makePolygon(roto->getRotoContext(), kSquare, 0., { {150., 350.}, {350., 350.}, {350., 150.}, {150., 150.} }) ) );
    const Alpha alpha( renderFrame(roto) );

    const int y = 250;
    EXPECT_EQ(1., alpha.at(250, y));
    EXPECT_EQ(1., alpha.at(151, y));
    EXPECT_EQ(0., alpha.at(95, y));
    double previous = 0.;
    for (int x = 100; x < 150; ++x) {
        const double a = alpha.at(x, y);
        EXPECT_NEAR(featherRamp(150. - (x + 0.5), 50.), a, kRampTolerance) << "x=" << x;
        EXPECT_GE(a, previous) << "x=" << x;
        previous = a;
    }
    // the same on the other sides
    EXPECT_NEAR( featherRamp(25., 50.), alpha.at(375, y), kRampTolerance );
    EXPECT_NEAR( featherRamp(25., 50.), alpha.at(250, 125), kRampTolerance );
    EXPECT_NEAR( featherRamp(25., 50.), alpha.at(250, 375), kRampTolerance );
}

// #147: a U shape with a feather distance of 80 px; the feathers of the two sides of its 100 px wide gap overlap.
// The alpha in the gap is the largest of the two feathers, a continuous and symmetric profile; one feather used to
// cover the other, with a jump from 0 to 0.66 where it ended.
TEST_F(RotoRenderTest, OverlappingFeathers)
{
    if (!_havePlugins) {
        return;
    }
    NodePtr roto = makeRoto();
    ASSERT_TRUE( bool(roto) );
    const std::vector<XY> u = { {100., 100.}, {400., 100.}, {400., 400.}, {300., 400.}, {300., 200.}, {200., 200.}, {200., 400.}, {100., 400.} };
    const double width = 80.;
    ASSERT_TRUE( bool( makePolygon(roto->getRotoContext(), u, width) ) );
    const Alpha alpha( renderFrame(roto) );

    // away from the corners
    const int y = 350;
    for (int x = 200; x < 300; ++x) {
        const double center = x + 0.5;
        const double expected = std::max( featherRamp(center - 200., width), featherRamp(300. - center, width) );
        EXPECT_NEAR(expected, alpha.at(x, y), 0.02) << "x=" << x;
        EXPECT_NEAR(alpha.at(x, y), alpha.at(499 - x, y), 0.02) << "x=" << x;
    }
    // the outer side is not changed
    for (int x = 20; x < 100; ++x) {
        EXPECT_NEAR(featherRamp(100. - (x + 0.5), width), alpha.at(x, 250), 0.02) << "x=" << x;
    }
}

// #367: the Mask input of Roto limits the shapes to the mask; outside the mask, the output is the Bg input.
TEST_F(RotoRenderTest, MaskInput)
{
    if (!_havePlugins) {
        return;
    }
    // Bg: transparent gray
    NodePtr bg = _nodes.create(PLUGINID_OFX_CONSTANT);
    ASSERT_TRUE( bool(bg) );
    KnobColorPtr color = std::dynamic_pointer_cast<KnobColor>( bg->getKnobByName("color") );
    ASSERT_TRUE( bool(color) );
    color->setValues(0.5, 0.5, 0.5, 0., ViewSpec::all(), eValueChangedReasonNatronInternalEdited);

    NodePtr roto = makeRoto();
    ASSERT_TRUE( bool(roto) );
    ASSERT_TRUE( bool( makePolygon(roto->getRotoContext(), kSquare, 0.) ) );
    ASSERT_TRUE( roto->connectInput(bg, 0) );

    NodePtr mask = makeRoto();
    ASSERT_TRUE( bool(mask) );
    ASSERT_TRUE( bool( makePolygon(mask->getRotoContext(), { {250., 550.}, {550., 550.}, {550., 250.}, {250., 250.} }, 0.) ) );

    {
        const Alpha alpha( renderFrame(roto) );
        EXPECT_EQ(1., alpha.at(150, 150));
        EXPECT_EQ(1., alpha.at(300, 300));
    }

    ASSERT_TRUE( roto->connectInput(mask, kMaskInput) );
    ASSERT_TRUE( roto->isMaskEnabled(kMaskInput) );
    {
        const Alpha alpha( renderFrame(roto) );
        EXPECT_EQ(1., alpha.at(300, 300)); // in the shape and in the mask
        EXPECT_EQ(0., alpha.at(150, 150)); // in the shape, not in the mask: Bg
        EXPECT_EQ(0., alpha.at(500, 500)); // in the mask, not in the shape: Bg
    }
}

// #367: without Bg input, outside the mask the output is transparent black.
TEST_F(RotoRenderTest, MaskInputWithoutBg)
{
    if (!_havePlugins) {
        return;
    }
    NodePtr roto = makeRoto();
    ASSERT_TRUE( bool(roto) );
    ASSERT_TRUE( bool( makePolygon(roto->getRotoContext(), kSquare, 0.) ) );
    NodePtr mask = makeRoto();
    ASSERT_TRUE( bool(mask) );
    ASSERT_TRUE( bool( makePolygon(mask->getRotoContext(), { {250., 550.}, {550., 550.}, {550., 250.}, {250., 250.} }, 0.) ) );
    ASSERT_TRUE( roto->connectInput(mask, kMaskInput) );
    ASSERT_TRUE( roto->isMaskEnabled(kMaskInput) );
    const Alpha alpha( renderFrame(roto) );
    EXPECT_EQ(1., alpha.at(300, 300)); // in the shape and in the mask
    EXPECT_EQ(0., alpha.at(150, 150)); // in the shape, not in the mask
}
