/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

#ifndef Engine_KnobGui_h
#define Engine_KnobGui_h

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Engine/OverlaySupport.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

class KnobGuiI
    : public OverlaySupport
{
public:

    KnobGuiI()
    {
    }

    virtual void swapOpenGLBuffers() OVERRIDE = 0;
    virtual void redraw() OVERRIDE = 0;
    virtual void getViewportSize(double &width, double &height) const OVERRIDE = 0;
    virtual void getPixelScale(double & xScale, double & yScale) const OVERRIDE = 0;
#ifdef OFX_EXTENSIONS_NATRON
    virtual double getScreenPixelRatio() const OVERRIDE = 0;
#endif
    virtual void getBackgroundColour(double &r, double &g, double &b) const OVERRIDE = 0;
    virtual void copyAnimationToClipboard(int /* dimension = -1*/) const {}

    virtual void copyValuesToClipboard(int /*dimension = -1*/) const {}

    virtual void copyLinkToClipboard(int /*dimension = -1*/) const {}

    virtual bool isGuiFrozenForPlayback() const = 0;
    virtual void saveOpenGLContext() OVERRIDE = 0;
    virtual void restoreOpenGLContext() OVERRIDE = 0;
    virtual unsigned int getCurrentRenderScale() const OVERRIDE { return 0; }

    virtual CurvePtr getCurve(ViewSpec view, int dimension) const = 0;
    virtual bool getAllDimensionsVisible() const = 0;
    virtual RectD getViewportRect() const OVERRIDE = 0;
    virtual void getCursorPosition(double& x, double& y) const OVERRIDE = 0;

    /**
     * @brief Converts the given (x,y) coordinates which are in OpenGL canonical coordinates to widget coordinates.
     **/
    virtual void toWidgetCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Converts the given (x,y) coordinates which are in widget coordinates to OpenGL canonical coordinates
     **/
    virtual void toCanonicalCoordinates(double *x, double *y) const OVERRIDE = 0;

    /**
     * @brief Returns the font height, i.e: the height of the highest letter for this font
     **/
    virtual int getWidgetFontHeight() const OVERRIDE = 0;

    /**
     * @brief Returns for a string the estimated pixel size it would take on the widget
     **/
    virtual int getStringWidthForCurrentFont(const std::string& string) const OVERRIDE = 0;

protected:

    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() = 0;
};

NATRON_NAMESPACE_EXIT

#endif // Engine_KnobGui_h
