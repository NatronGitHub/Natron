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

#ifndef Engine_OfxDrawSuiteGL_h
#define Engine_OfxDrawSuiteGL_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <ofxCore.h>
#include <ofxDrawSuite.h>
#include <ofxhDrawSuiteOpenGL.h>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief The OFX 1.5 DrawSuite (OfxDrawSuiteV1, ofxDrawSuite.h) of a Natron interact, drawn with
 * OpenGL: a thin adapter over HostSupport's OFX::Host::Interact::OpenGLDrawSuite
 * (ofxhDrawSuiteOpenGL.h), which draws with the OpenGL DrawSuite core of openfx
 * (ofxDrawSuiteOpenGL.h), the same code as the Support library's DrawSuite emulation in OpenGL
 * overlays. Natron only provides:
 * - the OpenGL functions, from glad (loadGLFunctions(), once OpenGL is loaded);
 * - the viewport's pixel scale and screen pixel ratio, and the drawSuiteShadows preference
 *   (beginDraw());
 * - its standard overlay colours (getColour());
 * - its text, in the viewport's font (OverlaySupport::renderText()), when the viewport can draw
 *   text (OverlaySupport::canRenderText()); otherwise the core's built-in font.
 *
 * Natron draws all overlays in a GL context that is current during kOfxInteractActionDraw, with the
 * projection of the interact's canonical view: the DrawSuite draws in the same view as the OpenGL
 * overlays. One object per interact instance holds the draw state of the current draw action.
 **/
class OfxDrawSuiteGL
    : public OFX::Host::Interact::OpenGLDrawSuite
{
public:
    OfxDrawSuiteGL();

    virtual ~OfxDrawSuiteGL() {}

    /// Fill the OpenGL function table of all the DrawSuite instances from glad. Called by
    /// AppManager once glad has loaded OpenGL (after the GLAD_DEBUG wrappers are set up); until
    /// then the table is empty and the DrawSuite draws nothing (kOfxStatFailed). Returns
    /// isComplete() of the table.
    static bool loadGLFunctions();

    /// The OpenGL function table filled by loadGLFunctions() (all NULL before)
    static const GLFunctions& natronGLFunctions();

    /// The drawSuiteShadows preference (Settings::drawSuiteShadows()), false
    /// without settings. Natron's interacts answer drawSuiteShadows() with it.
    static bool shadowsPreference();

    /// Start a draw action (kOfxInteractActionDraw) in viewport: reset the state with the
    /// viewport's pixel scale and screen pixel ratio, and the shadows flag, and use the OpenGL
    /// functions if OpenGL is loaded.
    void beginDraw(OverlaySupport* viewport, bool shadows);

    /// Natron's overlay colours: the selection colour of the preferences for the selected items
    /// and the marquee, green for the active item, white for the deselected items and the text,
    /// and black for the background (the colour of the shadows).
    virtual OfxStatus getColour(OfxStandardColour stdColour, OfxRGBAColourF *colour) const OVERRIDE;

protected:
    /// The text metrics in the viewport's text font, in screen pixels (the viewport's metrics, in
    /// widget pixels, of the font that OverlaySupport::renderText() draws with). false (the
    /// built-in font) if the viewport can't draw text.
    virtual bool measureText(const char *utf8, TextMetrics &metrics) OVERRIDE;

    /// OverlaySupport::renderText(), whose (x, y) is the bottom-left corner of the line box, so
    /// one descent below the baseline. kOfxStatReplyDefault (the built-in font) if it fails.
    virtual OfxStatus renderText(const char *utf8, const OfxPointD &baselineLeft, const TextMetrics &metrics) OVERRIDE;

private:
    OverlaySupport* _viewport;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_OfxDrawSuiteGL_h
