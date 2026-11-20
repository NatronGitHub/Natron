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

#include "OfxDrawSuiteGL.h"

#include <string>

#include "Global/GLIncludes.h"

#include "Engine/AppManager.h"
#include "Engine/OverlaySupport.h"
#include "Engine/Settings.h"

NATRON_NAMESPACE_ENTER

namespace {
// The OpenGL functions of all the DrawSuite instances (zero-initialized: no function before
// loadGLFunctions()). Written once, by AppManagerPrivate::initGl() with the OpenGL functions mutex
// held, and read in the main thread after AppManager::isOpenGLLoaded(), which takes that mutex.
OFX::DrawSuiteOpenGL::GLFunctions gNatronGLFunctions;
}

OfxDrawSuiteGL::OfxDrawSuiteGL()
    : OFX::Host::Interact::OpenGLDrawSuite(NULL)
    , _viewport(NULL)
{
}

bool
OfxDrawSuiteGL::loadGLFunctions()
{
    // glad's function pointers (the GLAD_DEBUG wrappers in debug builds), which have the types of
    // the GLFunctions members. They are only valid once glad has loaded OpenGL: a table filled
    // earlier would hold NULL or the not-yet-set-up GLAD_DEBUG wrappers.
    OFX::DrawSuiteOpenGL::GLFunctions& t = gNatronGLFunctions;

    t.Begin = glBegin;
    t.End = glEnd;
    t.Vertex2d = glVertex2d;
    t.Color4f = glColor4f;
    t.Enable = glEnable;
    t.Disable = glDisable;
    t.Hint = glHint;
    t.LineWidth = glLineWidth;
    t.LineStipple = glLineStipple;
    t.PolygonMode = glPolygonMode;
    t.BlendFunc = glBlendFunc;
    t.PushAttrib = glPushAttrib;
    t.PopAttrib = glPopAttrib;

    return OFX::DrawSuiteOpenGL::isComplete(t);
}

const OfxDrawSuiteGL::GLFunctions&
OfxDrawSuiteGL::natronGLFunctions()
{
    return gNatronGLFunctions;
}

bool
OfxDrawSuiteGL::shadowsPreference()
{
    SettingsPtr settings = appPTR ? appPTR->getCurrentSettings() : SettingsPtr();

    return settings ? settings->drawSuiteShadows() : false;
}

void
OfxDrawSuiteGL::beginDraw(OverlaySupport* viewport,
                          bool shadows)
{
    _viewport = viewport;
    OfxPointD pixelScale = { 1., 1. };
    double ratio = 1.;
    if (viewport) {
        viewport->getPixelScale(pixelScale.x, pixelScale.y);
        ratio = viewport->getScreenPixelRatio();
    }
    // (sanitised by the core: positive, finite)
    OFX::Host::Interact::OpenGLDrawSuite::beginDraw(pixelScale, ratio, shadows);

    const bool glLoaded = appPTR && appPTR->isOpenGLLoaded() && OFX::DrawSuiteOpenGL::isComplete(gNatronGLFunctions);
    setGLFunctions(glLoaded ? &gNatronGLFunctions : NULL);
}

OfxStatus
OfxDrawSuiteGL::getColour(OfxStandardColour stdColour,
                          OfxRGBAColourF *colour) const
{
    if (!colour) {
        return kOfxStatErrValue;
    }
    // The colours of Natron's own overlays: the selection colour of the preferences for selected
    // items and the marquee, white for the rest, green for the item under the mouse (as the
    // openfx-supportext overlays do), and black for the shadow drawn behind the overlay.
    double sr = 1., sg = 1., sb = 1.;
    if ( appPTR && appPTR->getCurrentSettings() ) {
        appPTR->getCurrentSettings()->getSelectionColor(&sr, &sg, &sb);
    }
    switch (stdColour) {
    case kOfxStandardColourOverlayBackground:
        *colour = { 0.f, 0.f, 0.f, 1.f };
        break;
    case kOfxStandardColourOverlayActive:
        *colour = { 0.f, 1.f, 0.f, 1.f };
        break;
    case kOfxStandardColourOverlaySelected:
    case kOfxStandardColourOverlayMarqueeFG:
        *colour = { (float)sr, (float)sg, (float)sb, 1.f };
        break;
    case kOfxStandardColourOverlayMarqueeBG:
        *colour = { (float)sr, (float)sg, (float)sb, 0.25f };
        break;
    case kOfxStandardColourOverlayDeselected:
    case kOfxStandardColourOverlayText:
        *colour = { 1.f, 1.f, 1.f, 1.f };
        break;
    default:
        return kOfxStatErrValue;
    }

    return kOfxStatOK;
}

bool
OfxDrawSuiteGL::measureText(const char *utf8,
                            TextMetrics &metrics)
{
    if ( !_viewport || !_viewport->canRenderText() ) {
        return false;
    }
    // "The text font face and size are determined by the host": the viewport's text font, whose
    // metrics the viewport gives in widget pixels, i.e. screen pixels, for the font that
    // OverlaySupport::renderText() draws with (on a HiDPI screen, the font scaled by the screen
    // pixel ratio, as the viewport's own labels).
    const std::string str(utf8);
    metrics.width = _viewport->getStringWidthForCurrentFont(str);
    metrics.ascent = _viewport->getWidgetFontAscent();
    metrics.descent = _viewport->getWidgetFontDescent();

    return true;
}

OfxStatus
OfxDrawSuiteGL::renderText(const char *utf8,
                           const OfxPointD &baselineLeft,
                           const TextMetrics &metrics)
{
    if (!_viewport) {
        return kOfxStatReplyDefault;
    }
    const State& s = state();
    if ( !_viewport->renderText(baselineLeft.x, baselineLeft.y - metrics.descent * s.pixelScale.y, std::string(utf8),
                                s.colour.r, s.colour.g, s.colour.b, 0, s.colour.a) ) {
        return kOfxStatReplyDefault; // this viewport can't draw text: the built-in font
    }

    return kOfxStatOK;
}

NATRON_NAMESPACE_EXIT
