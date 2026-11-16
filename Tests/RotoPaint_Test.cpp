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

// Regression tests for https://github.com/NatronGitHub/Natron/issues/510
// ("Rotopaint doesn't apply stroke to pencil at the first frame it is created").
//
// The RotoPaint overlay interact is driven headless, with a mock viewport. Every user edit that changes what the
// node renders must evaluate the node (RotoContext::evaluateChange()), which increments the node hash: that is
// what invalidates the image cached for the current frame and makes the viewer render it again.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <iostream>
#include <string>

#include <gtest/gtest.h>

#include <QtCore/QPointF>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/Bezier.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/RenderScale.h"
#include "Engine/RotoContext.h"
#include "Engine/RotoPaintInteract.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_USING

namespace {
// A viewport that does not draw anything: only what the RotoPaint overlay interact queries during pen events.
class MockViewport
    : public OverlaySupport
{
public:
    virtual void swapOpenGLBuffers() OVERRIDE FINAL {}

    virtual void redraw() OVERRIDE FINAL {}

    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        width = 1000.;
        height = 1000.;
    }

    virtual void getPixelScale(double & xScale,
                               double & yScale) const OVERRIDE FINAL
    {
        xScale = 1.;
        yScale = 1.;
    }

#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE FINAL
    {
        return 1.;
    }

#endif

    virtual void getBackgroundColour(double &r,
                                     double &g,
                                     double &b) const OVERRIDE FINAL
    {
        r = g = b = 0.;
    }

    virtual void saveOpenGLContext() OVERRIDE FINAL {}

    virtual void restoreOpenGLContext() OVERRIDE FINAL {}

    virtual RectD getViewportRect() const OVERRIDE FINAL
    {
        return RectD(0., 0., 1000., 1000.);
    }

    virtual void getCursorPosition(double& x,
                                   double& y) const OVERRIDE FINAL
    {
        x = y = 0.;
    }

    virtual void toWidgetCoordinates(double * /*x*/,
                                     double * /*y*/) const OVERRIDE FINAL {}

    virtual void toCanonicalCoordinates(double * /*x*/,
                                        double * /*y*/) const OVERRIDE FINAL {}

    virtual int getWidgetFontHeight() const OVERRIDE FINAL
    {
        return 10;
    }

    virtual int getStringWidthForCurrentFont(const std::string& /*string*/) const OVERRIDE FINAL
    {
        return 10;
    }
};

bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

// The overlay actions are protected in EffectInstance (and private in RotoPaint), and the public
// EffectInstance::onOverlayPen*_public() wrappers only forward them when the node has a GUI (hasHostOverlay()).
// A pointer to a protected member can be formed from a derived class: calling through it still dispatches
// to RotoPaint's override, without needing a GUI.
class OverlayActions
    : public EffectInstance
{
public:
    typedef bool (EffectInstance::*PenDownFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double, PenType);
    typedef bool (EffectInstance::*PenMotionFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double);
    typedef bool (EffectInstance::*PenUpFn)(double, const RenderScale &, ViewIdx, const QPointF &, const QPointF &, double, double);

    static PenDownFn penDown() { return &OverlayActions::onOverlayPenDown; }

    static PenMotionFn penMotion() { return &OverlayActions::onOverlayPenMotion; }

    static PenUpFn penUp() { return &OverlayActions::onOverlayPenUp; }
};

const double kTime = 1.;
} // anonymous namespace

class RotoPaintOverlayTest
    : public testing::Test
{
protected:

    virtual void SetUp() OVERRIDE FINAL
    {
        _app = appPTR->getTopLevelInstance();

        // Every shape is backed by OpenFX nodes (see RotoDrawableItem::createNodes()).
        _havePlugins = isPluginAvailable(PLUGINID_OFX_MERGE) && isPluginAvailable(PLUGINID_OFX_ROTO);
        if (!_havePlugins) {
            std::cout << "Skipping: the RotoPaint tests need the " PLUGINID_OFX_MERGE " and " PLUGINID_OFX_ROTO
                " plug-ins (openfx-misc), found through OFX_PLUGIN_PATH" << std::endl;

            return;
        }

        CreateNodeArgs args( PLUGINID_NATRON_ROTOPAINT, _app->getProject() );
        _node = _app->createNode(args);
        ASSERT_TRUE( bool(_node) );
        _effect = _node->getEffectInstance();
        ASSERT_TRUE( bool(_effect) );
        _context = _node->getRotoContext();
        ASSERT_TRUE( bool(_context) );
        _effect->setCurrentViewportForOverlays_public(&_viewport);
    }

    virtual void TearDown() OVERRIDE FINAL
    {
        if (_effect) {
            _effect->setCurrentViewportForOverlays_public(NULL);
        }
        if (_node) {
            _node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
        }
        _context.reset();
        _effect.reset();
        _node.reset();
    }

    // Activates a toolbar knob like NodeViewerContext does: when it is already on (e.g. the default tool)
    // setting its value is not a change, so at least a knobChanged() call must be issued.
    template <typename KNOB>
    void activate(const std::shared_ptr<KNOB>& knob)
    {
        if (knob->getValue() != true) {
            knob->setValue(true);
        } else {
            _effect->onKnobValueChanged_public(knob.get(), eValueChangedReasonUserEdited, _effect->getCurrentTime(), ViewSpec(0), true);
        }
    }

    // Selects a tool like the toolbar does: its group (role) first, then the tool itself.
    void selectTool(const char* groupName,
                    const char* toolName)
    {
        KnobGroupPtr group = std::dynamic_pointer_cast<KnobGroup>( _node->getKnobByName(groupName) );
        KnobButtonPtr tool = std::dynamic_pointer_cast<KnobButton>( _node->getKnobByName(toolName) );

        ASSERT_TRUE( bool(group) );
        ASSERT_TRUE( bool(tool) );
        activate(group);
        activate(tool);
    }

    void penDown(double x,
                 double y)
    {
        EXPECT_TRUE( ( (*_effect).*OverlayActions::penDown() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0., ePenTypeLMB) );
    }

    void penMotion(double x,
                   double y)
    {
        EXPECT_TRUE( ( (*_effect).*OverlayActions::penMotion() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0.) );
    }

    void penUp(double x,
               double y)
    {
        // The return value only tells whether the interact needs a redraw
        const bool redraw = ( (*_effect).*OverlayActions::penUp() )(kTime, RenderScale(), ViewIdx(0), QPointF(x, y), QPointF(x, y), 1., 0.);

        Q_UNUSED(redraw);
    }

    BezierPtr getDrawnBezier() const
    {
        // onlyActivated = false: a closed bezier that is not finished yet is not listed otherwise
        std::list<RotoDrawableItemPtr> items = _context->getCurvesByRenderOrder(false);

        return items.empty() ? BezierPtr() : std::dynamic_pointer_cast<Bezier>( items.front() );
    }

    AppInstancePtr _app;
    bool _havePlugins;
    NodePtr _node;
    EffectInstancePtr _effect;
    RotoContextPtr _context;
    MockViewport _viewport;
};

// Drawing a Pencil stroke (an open bezier) must re-render the node when the pen is released. Otherwise the viewer
// keeps showing the image of the current frame cached before the stroke existed (the other frames are not cached
// yet, so they show it): issue 510.
TEST_F(RotoPaintOverlayTest, PencilStrokeEvaluatesNodeOnPenUp)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamPencilToolButtonAction);

    penDown(100., 100.);
    penMotion(140., 120.);
    const U64 ageBeforePenUp = _context->getAge();
    const U64 hashBeforePenUp = _node->getHashValue();
    penUp(140., 120.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_TRUE( bezier->isOpenBezier() );
    EXPECT_GT(_context->getAge(), ageBeforePenUp);
    EXPECT_NE(_node->getHashValue(), hashBeforePenUp);

    // Extending the stroke with a second point must re-render it too.
    penDown(200., 150.);
    const U64 ageBeforeSecondPenUp = _context->getAge();
    const U64 hashBeforeSecondPenUp = _node->getHashValue();
    penUp(200., 150.);
    EXPECT_EQ(2, bezier->getControlPointsCount());
    EXPECT_GT(_context->getAge(), ageBeforeSecondPenUp);
    EXPECT_NE(_node->getHashValue(), hashBeforeSecondPenUp);
}

// A closed bezier is not rendered before it is finished (see RotoContextPrivate::renderBezier()): adding a point
// to it must not re-render the node. FinishingBezierByChangingToolEvaluatesNode checks that finishing it does.
TEST_F(RotoPaintOverlayTest, UnfinishedBezierPointDoesNotEvaluateNodeOnPenUp)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamDrawBezierToolButtonAction);

    penDown(100., 100.);
    penUp(100., 100.);
    penDown(300., 100.);
    const U64 ageBeforePenUp = _context->getAge();
    const U64 hashBeforePenUp = _node->getHashValue();
    penUp(300., 100.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_FALSE( bezier->isOpenBezier() );
    EXPECT_FALSE( bezier->isCurveFinished() );
    EXPECT_EQ(2, bezier->getControlPointsCount());
    EXPECT_EQ(_context->getAge(), ageBeforePenUp);
    EXPECT_EQ(_node->getHashValue(), hashBeforePenUp);
}

// Choosing another tool of the same group finishes the closed bezier being drawn, which makes it renderable
// (closed beziers are not rendered before they are finished): that must evaluate the node as well.
TEST_F(RotoPaintOverlayTest, FinishingBezierByChangingToolEvaluatesNode)
{
    if (!_havePlugins) {
        return;
    }
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamDrawBezierToolButtonAction);

    penDown(100., 100.);
    penUp(100., 100.);
    penDown(300., 100.);
    penUp(300., 100.);
    penDown(200., 250.);
    penUp(200., 250.);

    BezierPtr bezier = getDrawnBezier();
    ASSERT_TRUE( bool(bezier) );
    EXPECT_EQ(3, bezier->getControlPointsCount());
    EXPECT_FALSE( bezier->isCurveFinished() );

    const U64 ageBeforeToolChange = _context->getAge();
    const U64 hashBeforeToolChange = _node->getHashValue();
    selectTool(kRotoUIParamBezierEditionToolButton, kRotoUIParamPencilToolButtonAction);

    EXPECT_TRUE( bezier->isCurveFinished() );
    EXPECT_GT(_context->getAge(), ageBeforeToolChange);
    EXPECT_NE(_node->getHashValue(), hashBeforeToolChange);
}
