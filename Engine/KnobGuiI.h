/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#ifndef KNOBGUII_H
#define KNOBGUII_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#endif
#include "Engine/OverlaySupport.h"
#include "Engine/EngineFwd.h"


class KnobGuiI
    : public OverlaySupport
{
public:

    KnobGuiI()
    {
    }

    virtual void swapOpenGLBuffers() = 0;
    virtual void redraw() = 0;
    virtual void getViewportSize(double &width, double &height) const = 0;
    virtual void getPixelScale(double & xScale, double & yScale) const  = 0;
    virtual void getBackgroundColour(double &r, double &g, double &b) const = 0;
    virtual void copyAnimationToClipboard() const = 0;
    virtual bool isGuiFrozenForPlayback() const = 0;
    virtual void saveOpenGLContext() = 0;
    virtual void restoreOpenGLContext() = 0;
    virtual unsigned int getCurrentRenderScale() const { return 0; }
    virtual boost::shared_ptr<Curve> getCurve(int dimension) const = 0;
protected:

    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() = 0;
};

#endif // KNOBGUII_H
