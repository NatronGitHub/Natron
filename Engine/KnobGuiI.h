//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef KNOBGUII_H
#define KNOBGUII_H

#include "Engine/OverlaySupport.h"

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
    virtual void saveContext() = 0;
    virtual void restoreContext() = 0;
    
protected:

    ///Should set to the underlying knob the gui ptr
    virtual void setKnobGuiPointer() = 0;
};

#endif // KNOBGUII_H
