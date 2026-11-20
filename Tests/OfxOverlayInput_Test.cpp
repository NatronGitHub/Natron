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

// The inputs of the OFX overlay interact actions (#1378, #1379):
// - kOfxInteractPropPenViewportPosition: "in the interact's openGL viewport coordinates, with 0,0 being
//   at the bottom left" (the viewers give widget coordinates, with 0,0 at the top left);
// - kOfxPropKeyString: "UTF8 string representing a character key that was pressed, some keys have no
//   UTF8 encoding, in which case this is """;
// - kOfxInteractPropPenPressure is passed through (the viewer gives 0 with no button down, see
//   ViewerGL::mouseMoveEvent()).
// The overlay is driven headless, with a mock viewport, through the EffectInstance::*_public() functions
// the viewer calls (see ViewerTab::notifyOverlaysPenDown_internal()).

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>

#include <gtest/gtest.h>

#include <QtCore/QPointF>
#include <QtCore/QString>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/OfxOverlayInteract.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Project.h"
#include "Engine/RenderScale.h"
#include "Engine/ViewIdx.h"

#include "OfxTestPlugin.h"

// Tests/OfxTestPlugin/NatronTestPluginOverlayInput.cpp
#define kNatronTestPluginOverlayInput "net.natron.test.OverlayInput"

NATRON_NAMESPACE_USING

namespace {
const double kViewportHeight = 600.;

// A viewport that does not draw anything: only what an OFX overlay interact queries.
class MockViewport
    : public OverlaySupport
{
public:
    virtual void swapOpenGLBuffers() OVERRIDE FINAL {}

    virtual void redraw() OVERRIDE FINAL {}

    virtual void getViewportSize(double &width,
                                 double &height) const OVERRIDE FINAL
    {
        width = 800.;
        height = kViewportHeight;
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
        return RectD(0., 0., 800., kViewportHeight);
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

const double kTime = 1.;

int
counter(const std::string& name)
{
    return NatronTestPlugin::counter("OverlayInput." + name);
}
} // anonymous namespace

TEST(OfxOverlayInput, KeyStringFromText)
{
    EXPECT_EQ( QByteArray("a"), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("a") ) );
    EXPECT_EQ( QByteArray("A"), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("A") ) );
    EXPECT_EQ( QByteArray("\xc3\xa9"), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\xc3\xa9") ) ); // é
    EXPECT_EQ( QByteArray(" "), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8(" ") ) );
    // no character: no text, or a control character
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString() ) );
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\r") ) ); // Return
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\t") ) ); // Tab
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\x1b") ) ); // Escape
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\x7f") ) ); // Delete
    EXPECT_EQ( QByteArray(), NatronOverlayInteractSupport::keyStringFromText( QString::fromUtf8("\x01") ) ); // Ctrl+A
}

TEST(OfxOverlayInput, ViewerOverlay)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginOverlayInput) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginOverlayInput);

        return;
    }
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( kNatronTestPluginOverlayInput, app->getProject() );
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    NodePtr node = app->createNode(args);
    ASSERT_TRUE( bool(node) );
    EffectInstancePtr effect = node->getEffectInstance();
    ASSERT_TRUE( effect->hasOverlay() );
    MockViewport viewport;
    effect->setCurrentViewportForOverlays_public(&viewport);

    // pen actions: the viewer gives the position in widget coordinates (0,0 at the top left)
    const int penDownCount = counter("penDown.count");
    const int penDownX = counter("penDown.viewportX");
    const int penDownY = counter("penDown.viewportY");
    const int penDownPressure = counter("penDown.pressure1000");
    ignore_result( effect->onOverlayPenDown_public( kTime, RenderScale(), ViewIdx(0), QPointF(10., 20.), QPointF(0., 0.), 1., 0., ePenTypeLMB, KeyboardModifiers() ) );
    EXPECT_EQ( penDownCount + 1, counter("penDown.count") );
    EXPECT_EQ( penDownX + 10, counter("penDown.viewportX") );
    // bottom-left origin: the point 20 pixels below the top is at y = height - 1 - 20
    EXPECT_EQ( penDownY + (int)kViewportHeight - 1 - 20, counter("penDown.viewportY") );
    EXPECT_EQ( penDownPressure + 1000, counter("penDown.pressure1000") );

    const int penUpY = counter("penUp.viewportY");
    const int penUpPressure = counter("penUp.pressure1000");
    ignore_result( effect->onOverlayPenUp_public( kTime, RenderScale(), ViewIdx(0), QPointF(10., kViewportHeight - 1.), QPointF(0., 0.), 0., 0., KeyboardModifiers() ) );
    // the bottom row of the widget is y = 0
    EXPECT_EQ( penUpY, counter("penUp.viewportY") );
    EXPECT_EQ( penUpPressure, counter("penUp.pressure1000") );

    const int penMotionY = counter("penMotion.viewportY");
    ignore_result( effect->onOverlayPenMotion_public( kTime, RenderScale(), ViewIdx(0), QPointF(0., 0.), QPointF(0., 0.), 0., 0., KeyboardModifiers() ) );
    EXPECT_EQ( penMotionY + (int)kViewportHeight - 1, counter("penMotion.viewportY") );

    // key actions: kOfxPropKeyString
    const int keyDownA = counter("keyDown.string.a");
    const int keyDownE = counter("keyDown.string.\xc3\xa9");
    const int keyDownNone = counter("keyDown.string.");
    const int keyUpA = counter("keyUp.string.a");
    const int keyRepeatA = counter("keyRepeat.string.a");
    ignore_result( effect->onOverlayKeyDown_public( kTime, RenderScale(), ViewIdx(0), Key_a, KeyboardModifiers(), QString::fromUtf8("a") ) );
    ignore_result( effect->onOverlayKeyRepeat_public( kTime, RenderScale(), ViewIdx(0), Key_a, KeyboardModifiers(), QString::fromUtf8("a") ) );
    ignore_result( effect->onOverlayKeyUp_public( kTime, RenderScale(), ViewIdx(0), Key_a, KeyboardModifiers(), QString::fromUtf8("a") ) );
    ignore_result( effect->onOverlayKeyDown_public( kTime, RenderScale(), ViewIdx(0), Key_eacute, KeyboardModifiers(), QString::fromUtf8("\xc3\xa9") ) );
    ignore_result( effect->onOverlayKeyDown_public( kTime, RenderScale(), ViewIdx(0), Key_Return, KeyboardModifiers(), QString::fromUtf8("\r") ) );
    EXPECT_EQ( keyDownA + 1, counter("keyDown.string.a") );
    EXPECT_EQ( keyRepeatA + 1, counter("keyRepeat.string.a") );
    EXPECT_EQ( keyUpA + 1, counter("keyUp.string.a") );
    EXPECT_EQ( keyDownE + 1, counter("keyDown.string.\xc3\xa9") );
    EXPECT_EQ( keyDownNone + 1, counter("keyDown.string.") );

    effect->setCurrentViewportForOverlays_public(NULL);
    node->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
