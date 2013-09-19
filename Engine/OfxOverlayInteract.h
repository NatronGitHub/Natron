//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */
#ifndef OFXOVERLAYINTERACT_H
#define OFXOVERLAYINTERACT_H

#include <QObject>
#include <ofxhImageEffect.h>

class OfxNode;
namespace Powiter {
   
    
class OfxImageEffectInstance;
class OfxOverlayInteract :  public OFX::Host::ImageEffect::OverlayInteract
{
    
    OfxNode* _node;
    
public:
    
    OfxOverlayInteract(OfxImageEffectInstance &v, int bitDepthPerComponent = 8, bool hasAlpha = false);
    
    virtual ~OfxOverlayInteract(){}
    
    /*Swaps the buffer of the attached viewer*/
    virtual OfxStatus swapBuffers();
    
    /*Calls updateGL() on the attached viewer*/
    virtual OfxStatus redraw();
    
    /// hooks to kOfxInteractPropViewportSize in the property set
    /// this is actually redundant and is to be deprecated
    virtual void getViewportSize(double &width, double &height) const ;
    
    // hooks to live kOfxInteractPropPixelScale in the property set
    virtual void getPixelScale(double& xScale, double& yScale) const ;
    
    // hooks to kOfxInteractPropBackgroundColour in the property set
    virtual void getBackgroundColour(double &r, double &g, double &b) const ;


};

}

#endif // OFXOVERLAYINTERACT_H
