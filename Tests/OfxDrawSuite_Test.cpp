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


// The OFX 1.5 DrawSuite of Natron's interacts (Engine/OfxDrawSuiteGL.h, an adapter over HostSupport's
// OFX::Host::Interact::OpenGLDrawSuite and the openfx OpenGL DrawSuite core, ofxDrawSuiteOpenGL.h): the
// choice of the overlay entry point (kOfxImageEffectPluginPropOverlayInteractV1 or V2), the state, the
// core's line widths, stipples and ellipses as Natron uses them, Natron's colours, text and OpenGL
// functions, the drawSuiteShadows preference, and the OpenGL drawing, rendered into
// an offscreen framebuffer.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "Global/GLIncludes.h"

#include <ofxDrawSuite.h>
#include <ofxDrawSuiteOpenGL.h>
#include <ofxImageEffect.h>
#include <ofxhImageEffect.h>
#include <ofxhPropertySuite.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/EffectInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/OSGLContext.h"
#include "Engine/OfxDrawSuiteGL.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Project.h"
#include "Engine/RenderScale.h"
#include "Engine/Settings.h"

#include "OfxTestPlugin.h"

NATRON_NAMESPACE_USING

namespace GL = OFX::DrawSuiteOpenGL;

TEST(OfxDrawSuite, HostSupportsDrawSuite)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    EXPECT_TRUE( OFX::Host::ImageEffect::gImageEffectHost->supportsDrawSuite() );
    EXPECT_TRUE( OFX::Host::ImageEffect::gImageEffectHost->fetchSuite(kOfxDrawSuite, 1) != NULL );
    // Natron prefers the OpenGL overlay of a plug-in that registers both
    EXPECT_TRUE( OFX::Host::ImageEffect::gImageEffectHost->preferOverlayInteractV1() );
}

namespace {
OfxStatus overlayV1(const char*, const void*, OfxPropertySetHandle, OfxPropertySetHandle) { return kOfxStatOK; }
OfxStatus overlayV2(const char*, const void*, OfxPropertySetHandle, OfxPropertySetHandle) { return kOfxStatOK; }

// the overlay entry points of a plug-in descriptor, as HostSupport declares them
const OFX::Host::Property::PropSpec overlayProps[] = {
    { kOfxImageEffectPluginPropOverlayInteractV1, OFX::Host::Property::ePointer, 1, false, NULL },
    { kOfxImageEffectPluginPropOverlayInteractV2, OFX::Host::Property::ePointer, 1, false, NULL },
    OFX::Host::Property::propSpecEnd
};

OfxPluginEntryPoint*
overlayEntry(OfxPluginEntryPoint* v1, OfxPluginEntryPoint* v2)
{
    OFX::Host::Property::Set props(overlayProps);
    props.setPointerProperty(kOfxImageEffectPluginPropOverlayInteractV1, (void*)v1);
    props.setPointerProperty(kOfxImageEffectPluginPropOverlayInteractV2, (void*)v2);
    OFX::Host::ImageEffect::Base desc(props);

    return desc.getOverlayInteractMainEntry();
}
}

// Natron supports both kinds of overlays: an OpenGL-only (V1) plug-in gets its V1 overlay, a
// DrawSuite-only (V2) plug-in gets its V2 overlay, and a plug-in that has both gets the V1 one
// (OfxHost::preferOverlayInteractV1()).
TEST(OfxDrawSuite, OverlayEntryPoint)
{
    ASSERT_TRUE(OFX::Host::ImageEffect::gImageEffectHost != NULL);
    EXPECT_TRUE( overlayEntry(NULL, NULL) == NULL );
    EXPECT_TRUE( overlayEntry(&overlayV1, NULL) == &overlayV1 );
    EXPECT_TRUE( overlayEntry(NULL, &overlayV2) == &overlayV2 );
    EXPECT_TRUE( overlayEntry(&overlayV1, &overlayV2) == &overlayV1 );
}

namespace {
// A viewport with the given pixel scale and screen pixel ratio. The font renderText() draws with is
// 10 widget (screen) pixels high (ascent 8, descent 2), 6 widget pixels per character.
class TestOverlaySupport
    : public OverlaySupport
{
public:
    TestOverlaySupport(int w, int h, double pixelScale = 1., double ratio = 1.)
        : _w(w), _h(h), _scaleX(pixelScale), _scaleY(pixelScale), _ratio(ratio), canText(true), textOK(true) {}

    virtual void swapOpenGLBuffers() OVERRIDE FINAL {}
    virtual void redraw() OVERRIDE FINAL {}
    virtual void getViewportSize(double &width, double &height) const OVERRIDE FINAL { width = _w; height = _h; }
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE FINAL { xScale = _scaleX; yScale = _scaleY; }
    virtual double getScreenPixelRatio() const OVERRIDE FINAL { return _ratio; }
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE FINAL { r = g = b = 0.5; }
    virtual void saveOpenGLContext() OVERRIDE FINAL {}
    virtual void restoreOpenGLContext() OVERRIDE FINAL {}
    virtual RectD getViewportRect() const OVERRIDE FINAL { return RectD(0, 0, _w, _h); }
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE FINAL { x = y = 0.; }
    virtual void toWidgetCoordinates(double * /*x*/, double * /*y*/) const OVERRIDE FINAL {}
    virtual void toCanonicalCoordinates(double * /*x*/, double * /*y*/) const OVERRIDE FINAL {}
    virtual int getWidgetFontHeight() const OVERRIDE FINAL { return 10; }
    virtual int getWidgetFontAscent() const OVERRIDE FINAL { return 8; }
    virtual int getWidgetFontDescent() const OVERRIDE FINAL { return 2; }
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE FINAL { return 6 * (int)string.size(); }
    virtual bool canRenderText() const OVERRIDE FINAL { return canText; }
    virtual bool renderText(double x, double y, const std::string &string, double r, double g, double b, int flags, double a) OVERRIDE FINAL
    {
        Text t = { x, y, r, g, b, a, flags, string };
        texts.push_back(t);

        return textOK;
    }

    struct Text
    {
        double x, y, r, g, b, a;
        int flags;
        std::string text;
    };

    int _w, _h;
    double _scaleX, _scaleY, _ratio;
    bool canText; // canRenderText()
    bool textOK; // what renderText() returns
    std::vector<Text> texts;
};

// A GLFunctions table that records the calls, to check what the DrawSuite asks OpenGL to draw
// without an OpenGL context.
struct GLRecord
{
    std::map<GL::GLenumT, bool> enabled;
    std::vector<GL::GLenumT> begins; // the mode of each glBegin
    std::vector<int> vertexCounts; // the vertices of each glBegin/glEnd block
    std::vector<OfxPointD> vertices;
    struct LineWidth { float width; bool smooth; };
    std::vector<LineWidth> lineWidths;
    struct Stipple { int factor; unsigned short pattern; };
    std::vector<Stipple> stipples;
    std::vector<OfxRGBAColourF> colours;
    int pushes, pops;

    void reset() { *this = GLRecord(); }
    GLRecord() : pushes(0), pops(0) {}
};

GLRecord gRec;

void OFX_DRAWGL_APIENTRY recBegin(GL::GLenumT mode) { gRec.begins.push_back(mode); gRec.vertexCounts.push_back(0); }
void OFX_DRAWGL_APIENTRY recEnd(void) {}
void OFX_DRAWGL_APIENTRY recVertex2d(GL::GLdoubleT x, GL::GLdoubleT y) { OfxPointD p = { x, y }; gRec.vertices.push_back(p); if ( !gRec.vertexCounts.empty() ) { ++gRec.vertexCounts.back(); } }
void OFX_DRAWGL_APIENTRY recColor4f(GL::GLfloatT r, GL::GLfloatT g, GL::GLfloatT b, GL::GLfloatT a) { OfxRGBAColourF c = { r, g, b, a }; gRec.colours.push_back(c); }
void OFX_DRAWGL_APIENTRY recEnable(GL::GLenumT cap) { gRec.enabled[cap] = true; }
void OFX_DRAWGL_APIENTRY recDisable(GL::GLenumT cap) { gRec.enabled[cap] = false; }
void OFX_DRAWGL_APIENTRY recHint(GL::GLenumT, GL::GLenumT) {}
void OFX_DRAWGL_APIENTRY recLineWidth(GL::GLfloatT w) { GLRecord::LineWidth l = { w, gRec.enabled[GL::kGLLineSmooth] }; gRec.lineWidths.push_back(l); }
void OFX_DRAWGL_APIENTRY recLineStipple(GL::GLintT factor, GL::GLushortT pattern) { GLRecord::Stipple s = { factor, pattern }; gRec.stipples.push_back(s); }
void OFX_DRAWGL_APIENTRY recPolygonMode(GL::GLenumT, GL::GLenumT) {}
void OFX_DRAWGL_APIENTRY recBlendFunc(GL::GLenumT, GL::GLenumT) {}
void OFX_DRAWGL_APIENTRY recPushAttrib(GL::GLbitfieldT) { ++gRec.pushes; }
void OFX_DRAWGL_APIENTRY recPopAttrib(void) { ++gRec.pops; }

const GL::GLFunctions recordingGL = {
    recBegin, recEnd, recVertex2d, recColor4f, recEnable, recDisable, recHint, recLineWidth, recLineStipple,
    recPolygonMode, recBlendFunc, recPushAttrib, recPopAttrib
};

// An OfxDrawSuiteGL that draws with recordingGL
class RecordingDrawSuite
    : public OfxDrawSuiteGL
{
public:
    void beginDraw(OverlaySupport* viewport, bool shadows)
    {
        OfxDrawSuiteGL::beginDraw(viewport, shadows);
        setGLFunctions(&recordingGL);
        gRec.reset();
    }
};

OfxStatus
drawLine(OfxDrawSuiteGL& d, OfxDrawPrimitive primitive = kOfxDrawPrimitiveLines)
{
    const OfxPointD pts[2] = { { 0., 0. }, { 10., 0. } };

    return d.drawPrimitive(primitive, pts, 2);
}
} // anon namespace

namespace {
// true if the DrawSuite can draw with OpenGL: OpenGL is loaded and its function table is complete
// (glad may load an OpenGL without some of the OpenGL 1.1 compatibility functions)
bool
drawSuiteGLAvailable()
{
    if ( !appPTR->isOpenGLLoaded() ) {
        // TODO: Convert to GTEST_SKIP() when gtest updated.
        std::cerr << "Skipping test because OpenGL loading failed." << std::endl;

        return false;
    }
    if ( !GL::isComplete( OfxDrawSuiteGL::natronGLFunctions() ) ) {
        std::cerr << "Skipping test because the OpenGL function table of the DrawSuite is incomplete "
                  << "(an OpenGL without the 1.1 compatibility functions)." << std::endl;

        return false;
    }

    return true;
}
}

// The OpenGL functions come from glad, once OpenGL is loaded (AppManager loads it at startup), and
// each draw action uses them.
TEST(OfxDrawSuite, GLFunctionsAfterGLInit)
{
    if ( !drawSuiteGLAvailable() ) {
        return;
    }
    const OfxDrawSuiteGL::GLFunctions& t = OfxDrawSuiteGL::natronGLFunctions();
    EXPECT_TRUE(t.Begin == glBegin);
    EXPECT_TRUE(t.End == glEnd);
    EXPECT_TRUE(t.Vertex2d == glVertex2d);
    EXPECT_TRUE(t.LineStipple == glLineStipple);
    EXPECT_TRUE(t.PopAttrib == glPopAttrib);

    TestOverlaySupport viewport(64, 64);
    OfxDrawSuiteGL d;
    EXPECT_TRUE(d.glFunctions() == NULL); // before the first draw
    d.beginDraw(&viewport, false);
    EXPECT_TRUE(d.glFunctions() == &t);
}

TEST(OfxDrawSuite, State)
{
    TestOverlaySupport viewport(64, 64, 0.5, 2.);
    OfxDrawSuiteGL d;
    const OfxRGBAColourF red = { 1.f, 0.f, 0.f, 0.5f };

    d.beginDraw(&viewport, true);
    // the viewport's pixel scale and screen pixel ratio, and the shadows flag
    EXPECT_EQ(0.5, d.state().pixelScale.x);
    EXPECT_EQ(0.5, d.state().pixelScale.y);
    EXPECT_EQ(2., d.state().screenPixelRatio);
    EXPECT_TRUE(d.state().shadows);

    EXPECT_EQ( kOfxStatOK, d.setColour(&red) );
    EXPECT_EQ( kOfxStatOK, d.setLineWidth(2.5f) );
    EXPECT_EQ( kOfxStatOK, d.setLineStipple(kOfxDrawLineStipplePatternDash) );
    EXPECT_EQ( 0.5f, d.state().colour.a );
    EXPECT_EQ( 2.5f, d.state().lineWidth );
    EXPECT_EQ( kOfxDrawLineStipplePatternDash, d.state().stipple );
    // invalid values are rejected and leave the state unchanged
    EXPECT_EQ( kOfxStatErrValue, d.setColour(NULL) );
    EXPECT_EQ( kOfxStatFailed, d.setLineWidth(-1.f) );
    EXPECT_EQ( kOfxStatFailed, d.setLineWidth( std::numeric_limits<float>::quiet_NaN() ) );
    EXPECT_EQ( kOfxStatFailed, d.setLineWidth( std::numeric_limits<float>::infinity() ) );
    EXPECT_EQ( kOfxStatErrValue, d.setLineStipple( (OfxDrawLineStipplePattern)1000 ) );
    EXPECT_EQ( 2.5f, d.state().lineWidth );
    EXPECT_EQ( kOfxDrawLineStipplePatternDash, d.state().stipple );

    // each draw action starts with opaque white, single-pixel solid lines
    d.beginDraw(&viewport, false);
    EXPECT_EQ( 1.f, d.state().colour.r );
    EXPECT_EQ( 1.f, d.state().colour.g );
    EXPECT_EQ( 1.f, d.state().colour.b );
    EXPECT_EQ( 1.f, d.state().colour.a );
    EXPECT_EQ( 0.f, d.state().lineWidth );
    EXPECT_EQ( kOfxDrawLineStipplePatternSolid, d.state().stipple );
    EXPECT_FALSE(d.state().shadows);
}

// Natron's overlay colours, from the preferences
TEST(OfxDrawSuite, StandardColours)
{
    const OfxStandardColour colours[] = {
        kOfxStandardColourOverlayBackground, kOfxStandardColourOverlayActive, kOfxStandardColourOverlaySelected,
        kOfxStandardColourOverlayDeselected, kOfxStandardColourOverlayMarqueeFG, kOfxStandardColourOverlayMarqueeBG,
        kOfxStandardColourOverlayText
    };
    OfxDrawSuiteGL d;

    for (std::size_t i = 0; i < sizeof(colours) / sizeof(colours[0]); ++i) {
        OfxRGBAColourF c = { -1.f, -1.f, -1.f, -1.f };
        EXPECT_EQ( kOfxStatOK, d.getColour(colours[i], &c) );
        EXPECT_TRUE(c.r >= 0.f && c.r <= 1.f && c.g >= 0.f && c.g <= 1.f && c.b >= 0.f && c.b <= 1.f && c.a > 0.f && c.a <= 1.f);
    }
    // the selected items and the marquee use the selection colour of the preferences
    double r, g, b;
    appPTR->getCurrentSettings()->getSelectionColor(&r, &g, &b);
    OfxRGBAColourF c;
    ASSERT_EQ( kOfxStatOK, d.getColour(kOfxStandardColourOverlaySelected, &c) );
    EXPECT_FLOAT_EQ( (float)r, c.r );
    EXPECT_FLOAT_EQ( (float)g, c.g );
    EXPECT_FLOAT_EQ( (float)b, c.b );
    ASSERT_EQ( kOfxStatOK, d.getColour(kOfxStandardColourOverlayMarqueeFG, &c) );
    EXPECT_FLOAT_EQ( (float)r, c.r );
    // the deselected items and the text are white
    ASSERT_EQ( kOfxStatOK, d.getColour(kOfxStandardColourOverlayDeselected, &c) );
    EXPECT_EQ( 3.f, c.r + c.g + c.b );
    // the background (the colour of the shadows) is black
    ASSERT_EQ( kOfxStatOK, d.getColour(kOfxStandardColourOverlayBackground, &c) );
    EXPECT_EQ( 0.f, c.r + c.g + c.b );
    EXPECT_EQ( kOfxStatErrValue, d.getColour( (OfxStandardColour)1000, &c ) );
    EXPECT_EQ( kOfxStatErrValue, d.getColour(kOfxStandardColourOverlayText, NULL) );
}

// The stipple patterns of the core, in screen pixels (bit 0 first)
TEST(OfxDrawSuite, StipplePatterns)
{
    EXPECT_EQ( 0xFFFF, GL::stipplePattern(kOfxDrawLineStipplePatternSolid) );
    EXPECT_EQ( 0x3333, GL::stipplePattern(kOfxDrawLineStipplePatternDot) );
    EXPECT_EQ( 0x00FF, GL::stipplePattern(kOfxDrawLineStipplePatternDash) );
    EXPECT_EQ( 0xFF00, GL::stipplePattern(kOfxDrawLineStipplePatternAltDash) );
    EXPECT_EQ( 0x1FE3, GL::stipplePattern(kOfxDrawLineStipplePatternDotDash) );
}

// Line widths and stipple factors follow the viewport's screen pixel ratio: width 0 is an unsmoothed
// line of max(1, round(ratio)) device pixels, another width a smooth line of width * ratio device
// pixels, and the stipple factor is round(ratio).
TEST(OfxDrawSuite, LineWidthAndStipple)
{
    const double ratios[] = { 1., 1.4, 1.6, 2., 3. };

    for (std::size_t i = 0; i < sizeof(ratios) / sizeof(ratios[0]); ++i) {
        const double ratio = ratios[i];
        const float rounded = (float)std::max(1., std::floor(ratio + 0.5));
        TestOverlaySupport viewport(64, 64, 1., ratio);
        RecordingDrawSuite d;
        d.beginDraw(&viewport, false);

        ASSERT_EQ( kOfxStatOK, drawLine(d) );
        ASSERT_EQ( 1u, gRec.lineWidths.size() );
        EXPECT_EQ( rounded, gRec.lineWidths[0].width ) << ratio;
        EXPECT_FALSE( gRec.lineWidths[0].smooth ) << ratio;
        EXPECT_TRUE( gRec.stipples.empty() );
        EXPECT_EQ(gRec.pushes, gRec.pops);

        ASSERT_EQ( kOfxStatOK, d.setLineWidth(1.5f) );
        ASSERT_EQ( kOfxStatOK, d.setLineStipple(kOfxDrawLineStipplePatternDash) );
        ASSERT_EQ( kOfxStatOK, drawLine(d, kOfxDrawPrimitiveLineStrip) );
        ASSERT_EQ( 2u, gRec.lineWidths.size() );
        EXPECT_FLOAT_EQ( (float)(1.5 * ratio), gRec.lineWidths[1].width ) << ratio;
        EXPECT_TRUE( gRec.lineWidths[1].smooth ) << ratio;
        ASSERT_EQ( 1u, gRec.stipples.size() );
        EXPECT_EQ( (int)rounded, gRec.stipples[0].factor ) << ratio;
        EXPECT_EQ( 0x00FF, gRec.stipples[0].pattern );
    }
}

// The vertex count of the ellipses: within 1/4 screen pixel of the ellipse
TEST(OfxDrawSuite, EllipseSegments)
{
    EXPECT_EQ( 8, GL::ellipseSegmentCount(0., 0.) );
    EXPECT_EQ( 15, GL::ellipseSegmentCount(10., 0.) );
    EXPECT_EQ( 32, GL::ellipseSegmentCount(50., 0.) );
    EXPECT_EQ( 45, GL::ellipseSegmentCount(100., 10.) );
    EXPECT_EQ( GL::ellipseSegmentCount(100., 10.), GL::ellipseSegmentCount(10., -100.) );
    EXPECT_EQ( 141, GL::ellipseSegmentCount(1000., 0.) );
    EXPECT_EQ( 445, GL::ellipseSegmentCount(1e4, 0.) );
    EXPECT_EQ( 1405, GL::ellipseSegmentCount(1e5, 0.) );
    EXPECT_EQ( 4096, GL::ellipseSegmentCount(1e6, 0.) );
    EXPECT_EQ( 4096, GL::ellipseSegmentCount(1e9, 1e9) );
    EXPECT_EQ( 8, GL::ellipseSegmentCount( std::numeric_limits<double>::quiet_NaN(), 1. ) );
    EXPECT_EQ( 8, GL::ellipseSegmentCount( std::numeric_limits<double>::infinity(), 1. ) );

    // the radii are measured in screen pixels of the viewport: 50 canonical units at a pixel scale
    // of 0.5 are 100 screen pixels
    TestOverlaySupport viewport(64, 64, 0.5, 2.);
    RecordingDrawSuite d;
    d.beginDraw(&viewport, false);
    const OfxPointD pts[2] = { { 0., 0. }, { 100., 20. } };
    ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveEllipse, pts, 2) );
    ASSERT_EQ( 1u, gRec.begins.size() );
    EXPECT_EQ( GL::kGLLineLoop, gRec.begins[0] );
    EXPECT_EQ( 45, gRec.vertexCounts[0] );
}

// With the shadows on, the lines and the ellipse outline are drawn twice, first one device pixel
// right and down in black with the current alpha; the filled primitives once.
TEST(OfxDrawSuite, Shadows)
{
    TestOverlaySupport viewport(64, 64, 0.5, 2.);
    RecordingDrawSuite d;
    const OfxRGBAColourF red = { 1.f, 0.f, 0.f, 0.5f };

    d.beginDraw(&viewport, true);
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    ASSERT_EQ( kOfxStatOK, drawLine(d) );
    ASSERT_EQ( 2u, gRec.begins.size() );
    ASSERT_EQ( 4u, gRec.vertices.size() );
    // one device pixel: pixelScale / ratio = 0.25 canonical units
    EXPECT_EQ(0.25, gRec.vertices[0].x);
    EXPECT_EQ(-0.25, gRec.vertices[0].y);
    EXPECT_EQ(0., gRec.vertices[2].x);
    EXPECT_EQ(0., gRec.vertices[2].y);
    ASSERT_EQ( 2u, gRec.colours.size() );
    EXPECT_EQ( 0.f, gRec.colours[0].r );
    EXPECT_EQ( 0.5f, gRec.colours[0].a );
    EXPECT_EQ( 1.f, gRec.colours[1].r );
    EXPECT_EQ( 0.5f, gRec.colours[1].a );

    gRec.reset();
    const OfxPointD rect[2] = { { 0., 0. }, { 10., 10. } };
    ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveRectangle, rect, 2) );
    EXPECT_EQ( 1u, gRec.begins.size() );

    // shadows off: once
    d.beginDraw(&viewport, false);
    ASSERT_EQ( kOfxStatOK, drawLine(d) );
    EXPECT_EQ( 1u, gRec.begins.size() );
}

// Text is drawn in the viewport's font by OverlaySupport::renderText(), aligned with the viewport's
// font metrics (in widget, i.e. screen, pixels, whatever the screen pixel ratio), with the colour's alpha; with the shadows on, a
// shadow first. Viewports that can't draw text get the built-in font of the core.
TEST(OfxDrawSuite, Text)
{
    TestOverlaySupport viewport(64, 64, 0.5, 2.);
    RecordingDrawSuite d;
    const OfxRGBAColourF red = { 1.f, 0.f, 0.f, 0.5f };
    const OfxPointD pos = { 100., 50. };

    d.beginDraw(&viewport, false);
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    // "abcd": 24 screen pixels, so 12 canonical units; descent 2 screen pixels, so 1 canonical unit
    ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, kOfxDrawTextAlignmentCenterH | kOfxDrawTextAlignmentBaseline) );
    ASSERT_EQ( 1u, viewport.texts.size() );
    EXPECT_EQ( std::string("abcd"), viewport.texts[0].text );
    EXPECT_EQ( 100. - 6., viewport.texts[0].x );
    EXPECT_EQ( 50. - 1., viewport.texts[0].y ); // the bottom of the line box
    EXPECT_EQ( 1., viewport.texts[0].r );
    EXPECT_EQ( 0., viewport.texts[0].g );
    EXPECT_EQ( 0.5, viewport.texts[0].a );
    EXPECT_TRUE( gRec.begins.empty() );

    // right, top: the ascent (8 screen pixels) is above the baseline
    viewport.texts.clear();
    ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, kOfxDrawTextAlignmentRight | kOfxDrawTextAlignmentTop) );
    ASSERT_EQ( 1u, viewport.texts.size() );
    EXPECT_EQ( 100. - 12., viewport.texts[0].x );
    EXPECT_EQ( 50. - 4. - 1., viewport.texts[0].y );

    // shadows: first one device pixel right and down, in black with the alpha of the colour
    d.beginDraw(&viewport, true);
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    viewport.texts.clear();
    ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, 0) );
    ASSERT_EQ( 2u, viewport.texts.size() );
    EXPECT_EQ( 100. + 0.25, viewport.texts[0].x );
    EXPECT_EQ( 50. - 1. - 0.25, viewport.texts[0].y );
    EXPECT_EQ( 0., viewport.texts[0].r );
    EXPECT_EQ( 0.5, viewport.texts[0].a );
    EXPECT_EQ( 100., viewport.texts[1].x );
    EXPECT_EQ( 50. - 1., viewport.texts[1].y );
    EXPECT_EQ( 1., viewport.texts[1].r );

    // a viewport that can't draw text (e.g. a parameter interact): the built-in font
    d.beginDraw(&viewport, false);
    viewport.texts.clear();
    viewport.canText = false;
    ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, 0) );
    EXPECT_TRUE( viewport.texts.empty() );
    ASSERT_EQ( 1u, gRec.begins.size() );
    EXPECT_EQ( GL::kGLQuads, gRec.begins[0] );

    // a viewport whose renderText() fails: the built-in font too
    d.beginDraw(&viewport, false);
    viewport.canText = true;
    viewport.textOK = false;
    ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, 0) );
    EXPECT_EQ( 1u, viewport.texts.size() );
    ASSERT_EQ( 1u, gRec.begins.size() );
    EXPECT_EQ( GL::kGLQuads, gRec.begins[0] );

    // NULL text or position
    EXPECT_EQ( kOfxStatErrValue, d.drawText(NULL, &pos, 0) );
    EXPECT_EQ( kOfxStatErrValue, d.drawText("a", NULL, 0) );
}

// Without OpenGL functions (OpenGL not loaded), the DrawSuite fails instead of calling NULL
TEST(OfxDrawSuite, NoGLFunctions)
{
    TestOverlaySupport viewport(64, 64);
    OfxDrawSuiteGL d;

    d.beginDraw(&viewport, false);
    d.setGLFunctions(NULL);
    EXPECT_EQ( kOfxStatFailed, drawLine(d) );
    viewport.canText = false;
    const OfxPointD pos = { 1., 1. };
    EXPECT_EQ( kOfxStatFailed, d.drawText("a", &pos, 0) );
}

namespace {
// An offscreen RGBA framebuffer with a projection whose canonical coordinates are its pixels.
class OffscreenFramebuffer
{
public:
    OffscreenFramebuffer(int w, int h)
        : _w(w), _h(h), _fbo(0), _rbo(0), ok(false)
    {
        _context = std::make_shared<OSGLContext>( FramebufferConfig(), nullptr, GLVersion.major, GLVersion.minor,
                                                  appPTR->getCurrentSettings()->getActiveOpenGLRendererID() );
        _context->setContextCurrentNoRender();
        if (!glGenFramebuffers || !glBegin) {
            return;
        }
        glGenFramebuffers(1, &_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glGenRenderbuffers(1, &_rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, _rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _rbo);
        ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, 0, h, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        clear();
    }

    ~OffscreenFramebuffer()
    {
        if (_fbo) {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteRenderbuffers(1, &_rbo);
            glDeleteFramebuffers(1, &_fbo);
        }
        OSGLContext::unsetCurrentContextNoRender();
    }

    void clear()
    {
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // the pixel whose center is (x + 0.5, y + 0.5), as 0-255 RGBA
    std::vector<int> pixel(int x, int y) const
    {
        glFinish();
        unsigned char p[4] = { 0, 0, 0, 0 };
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p);

        return std::vector<int>(p, p + 4);
    }

    // the number of pixels whose red component is non-zero
    int countRed() const
    {
        glFinish();
        std::vector<unsigned char> buf(_w * _h * 4);
        glReadPixels(0, 0, _w, _h, GL_RGBA, GL_UNSIGNED_BYTE, &buf.front());
        int n = 0;
        for (int i = 0; i < _w * _h; ++i) {
            n += buf[4 * i] != 0;
        }

        return n;
    }

private:
    int _w, _h;
    OSGLContextPtr _context;
    GLuint _fbo, _rbo;

public:
    bool ok;
};

std::vector<int> rgba(int r, int g, int b, int a) { std::vector<int> v(4); v[0] = r; v[1] = g; v[2] = b; v[3] = a; return v; }
} // anon namespace

TEST(OfxDrawSuite, OpenGLDrawing)
{
    if ( !drawSuiteGLAvailable() ) {
        return;
    }
    const int W = 64, H = 64;
    OffscreenFramebuffer fb(W, H);
    if (!fb.ok) {
        std::cerr << "Skipping test because no offscreen framebuffer could be created." << std::endl;
        return;
    }
    // canonical coordinates are the framebuffer's pixels, as Natron's viewer at zoom 1
    TestOverlaySupport viewport(W, H);
    OfxDrawSuiteGL d;
    d.beginDraw(&viewport, false);
    ASSERT_TRUE( d.glFunctions() != NULL );

    const OfxRGBAColourF red = { 1.f, 0.f, 0.f, 1.f };
    const OfxRGBAColourF halfGreen = { 0.f, 1.f, 0.f, 0.5f };

    // filled rectangle, given by its top-right and bottom-left corners
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    {
        const OfxPointD pts[2] = { { 40., 40. }, { 8., 8. } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveRectangle, pts, 2) );
    }
    EXPECT_EQ( rgba(255, 0, 0, 255), fb.pixel(20, 20) );
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(4, 4) );
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(44, 44) );

    // non-opaque colours are composited "over"
    ASSERT_EQ( kOfxStatOK, d.setColour(&halfGreen) );
    {
        const OfxPointD pts[3] = { { 16., 16. }, { 32., 16. }, { 24., 32. } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitivePolygon, pts, 3) );
    }
    std::vector<int> p = fb.pixel(24, 20);
    EXPECT_NEAR(127, p[0], 2);
    EXPECT_NEAR(127, p[1], 2);
    EXPECT_EQ(0, p[2]);
    EXPECT_EQ( rgba(255, 0, 0, 255), fb.pixel(10, 10) );

    // a horizontal line and an ellipse outline, single-pixel and solid
    fb.clear();
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    {
        const OfxPointD pts[2] = { { 4., 4.5 }, { 60., 4.5 } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveLines, pts, 2) );
    }
    EXPECT_EQ( rgba(255, 0, 0, 255), fb.pixel(30, 4) );
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(30, 6) );
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(30, 3) ); // no shadow
    const int lineCount = fb.countRed();
    EXPECT_NEAR(56, lineCount, 2);

    fb.clear();
    {
        const OfxPointD pts[2] = { { 8., 8. }, { 56., 56. } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveEllipse, pts, 2) );
    }
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(32, 32) ); // not filled
    const int ellipseCount = fb.countRed();
    EXPECT_NEAR(2 * M_PI * 24, ellipseCount, 30); // the circumference, in pixels

    // a stippled line draws fewer pixels than a solid one
    fb.clear();
    ASSERT_EQ( kOfxStatOK, d.setLineStipple(kOfxDrawLineStipplePatternDash) );
    {
        const OfxPointD pts[2] = { { 4., 4.5 }, { 60., 4.5 } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveLines, pts, 2) );
    }
    EXPECT_NEAR(lineCount / 2, fb.countRed(), 5);

    // a wider line covers more rows
    fb.clear();
    ASSERT_EQ( kOfxStatOK, d.setLineStipple(kOfxDrawLineStipplePatternSolid) );
    ASSERT_EQ( kOfxStatOK, d.setLineWidth(5.f) );
    {
        const OfxPointD pts[2] = { { 4., 32. }, { 60., 32. } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveLineStrip, pts, 2) );
    }
    EXPECT_GE( fb.countRed(), 3 * lineCount );

    // The DrawSuite calls leave the OpenGL state of the plug-in unchanged
    EXPECT_FALSE( glIsEnabled(GL_BLEND) );
    EXPECT_FALSE( glIsEnabled(GL_LINE_STIPPLE) );
    EXPECT_FALSE( glIsEnabled(GL_LINE_SMOOTH) );
    GLfloat lineWidth = 0.f;
    glGetFloatv(GL_LINE_WIDTH, &lineWidth);
    EXPECT_EQ(1.f, lineWidth);
    EXPECT_EQ( GL_NO_ERROR, glGetError() );

    // the shadow of a line: one pixel right and down, in black
    fb.clear();
    d.beginDraw(&viewport, true);
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    {
        const OfxPointD pts[2] = { { 4., 4.5 }, { 60., 4.5 } };
        ASSERT_EQ( kOfxStatOK, d.drawPrimitive(kOfxDrawPrimitiveLines, pts, 2) );
    }
    EXPECT_EQ( rgba(255, 0, 0, 255), fb.pixel(30, 4) );
    EXPECT_EQ( rgba(0, 0, 0, 255), fb.pixel(30, 3) );

    // text goes through the viewport, aligned with its font metrics
    d.beginDraw(&viewport, false);
    ASSERT_EQ( kOfxStatOK, d.setColour(&red) );
    {
        const OfxPointD pos = { 32., 32. };
        ASSERT_EQ( kOfxStatOK, d.drawText("abcd", &pos, kOfxDrawTextAlignmentCenterH | kOfxDrawTextAlignmentBaseline) );
    }
    ASSERT_EQ( 1u, viewport.texts.size() );
    EXPECT_EQ( std::string("abcd"), viewport.texts[0].text );
    EXPECT_EQ( 32. - 12., viewport.texts[0].x ); // 4 characters of 6 pixels, centered
    EXPECT_EQ( 32. - 2., viewport.texts[0].y ); // the descent is below the baseline

    // the built-in font, in a viewport that can't draw text
    fb.clear();
    viewport.canText = false;
    {
        const OfxPointD pos = { 8., 8. };
        ASSERT_EQ( kOfxStatOK, d.drawText("H", &pos, 0) );
    }
    EXPECT_GT( fb.countRed(), 7 ); // at least the two stems of the H
    EXPECT_EQ( rgba(0, 0, 0, 0), fb.pixel(40, 40) );
    EXPECT_EQ( GL_NO_ERROR, glGetError() );
} // TEST(OfxDrawSuite, OpenGLDrawing)

namespace {
// The counters of the overlays of the test plug-ins (Tests/OfxTestPlugin/NatronTestPlugin.cpp)
struct OverlayCounters
{
    int drawV1, drawV2, shadowsOn, shadowsOff, shadowsMissing, drawSuiteOK;

    explicit OverlayCounters(const std::string& id)
        : drawV1( NatronTestPlugin::counter(id + ".drawV1") )
        , drawV2( NatronTestPlugin::counter(id + ".drawV2") )
        , shadowsOn( NatronTestPlugin::counter(id + ".shadowsOn") )
        , shadowsOff( NatronTestPlugin::counter(id + ".shadowsOff") )
        , shadowsMissing( NatronTestPlugin::counter(id + ".shadowsMissing") )
        , drawSuiteOK( NatronTestPlugin::counter(id + ".drawSuiteOK") )
    {
    }
};

NodePtr
createOverlayNode(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);

    return app->createNode(args);
}

void
drawOverlay(const NodePtr& node,
            OverlaySupport* viewport)
{
    EffectInstancePtr effect = node->getEffectInstance();

    effect->setCurrentViewportForOverlays_public(viewport);
    effect->drawOverlay_public( 0., RenderScale::identity, ViewIdx(0) );
}
} // anon namespace

// A plug-in that registers both overlay entries gets its V1 overlay, a plug-in that registers only a
// V2 overlay gets it, and the drawSuiteShadows preference reaches the inArgs of
// kOfxInteractActionDraw (kNatronOfxInteractPropDrawSuiteShadows) of both.
TEST(OfxDrawSuite, OverlayPlugins)
{
    if ( !NatronTestPlugin::isAvailable(kNatronTestPluginOverlayV1V2) || !NatronTestPlugin::isAvailable(kNatronTestPluginOverlayV2) ) {
        NatronTestPlugin::reportSkipped(kNatronTestPluginOverlayV1V2);

        return;
    }
    if ( !drawSuiteGLAvailable() ) {
        return;
    }
    KnobBoolPtr shadowsPref = std::dynamic_pointer_cast<KnobBool>( appPTR->getCurrentSettings()->getKnobByName("drawSuiteShadows") );
    ASSERT_TRUE( bool(shadowsPref) );
    EXPECT_TRUE( shadowsPref->getDefaultValue(0) ); // on by default, like Natron's own overlay shadows
    const bool oldPref = shadowsPref->getValue();

    const int W = 64, H = 64;
    OffscreenFramebuffer fb(W, H); // the overlays draw in a current OpenGL context
    if (!fb.ok) {
        std::cerr << "Skipping test because no offscreen framebuffer could be created." << std::endl;

        return;
    }
    TestOverlaySupport viewport(W, H);
    NodePtr both = createOverlayNode(kNatronTestPluginOverlayV1V2);
    NodePtr v2Only = createOverlayNode(kNatronTestPluginOverlayV2);
    ASSERT_TRUE( bool(both) );
    ASSERT_TRUE( bool(v2Only) );
    EXPECT_TRUE( both->getEffectInstance()->hasOverlay() );
    EXPECT_TRUE( v2Only->getEffectInstance()->hasOverlay() );

    for (int on = 1; on >= 0; --on) {
        shadowsPref->setValue( (bool)on );
        EXPECT_EQ( (bool)on, appPTR->getCurrentSettings()->drawSuiteShadows() );

        const OverlayCounters bothBefore("OverlayV1V2");
        drawOverlay(both, &viewport);
        const OverlayCounters bothAfter("OverlayV1V2");
        EXPECT_EQ( bothBefore.drawV1 + 1, bothAfter.drawV1 );
        EXPECT_EQ( bothBefore.drawV2, bothAfter.drawV2 );
        EXPECT_EQ( bothBefore.shadowsOn + on, bothAfter.shadowsOn );
        EXPECT_EQ( bothBefore.shadowsOff + 1 - on, bothAfter.shadowsOff );
        EXPECT_EQ( bothBefore.shadowsMissing, bothAfter.shadowsMissing );

        fb.clear();
        const OverlayCounters v2Before("OverlayV2");
        drawOverlay(v2Only, &viewport);
        const OverlayCounters v2After("OverlayV2");
        EXPECT_EQ( v2Before.drawV1, v2After.drawV1 );
        EXPECT_EQ( v2Before.drawV2 + 1, v2After.drawV2 );
        EXPECT_EQ( v2Before.shadowsOn + on, v2After.shadowsOn );
        EXPECT_EQ( v2Before.shadowsOff + 1 - on, v2After.shadowsOff );
        EXPECT_EQ( v2Before.drawSuiteOK + 2, v2After.drawSuiteOK ); // the line and the text
        // the DrawSuite line from (4, 4.5) to (20, 4.5), in white; its shadow below it with the preference
        EXPECT_EQ( rgba(255, 255, 255, 255), fb.pixel(10, 4) );
        EXPECT_EQ( on ? rgba(0, 0, 0, 255) : rgba(0, 0, 0, 0), fb.pixel(10, 3) );
        // the text, in the viewport's font
        ASSERT_FALSE( viewport.texts.empty() );
        EXPECT_EQ( std::string("ab"), viewport.texts.back().text );
        EXPECT_EQ( on ? 2u : 1u, viewport.texts.size() );
        viewport.texts.clear();
    }
    shadowsPref->setValue(oldPref);

    both->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
    v2Only->destroyNode(true /*blockingDestroy*/, false /*autoReconnect*/);
}
