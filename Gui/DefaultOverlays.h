
//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */


#ifndef DEFAULTOVERLAYS_H
#define DEFAULTOVERLAYS_H

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/OfxOverlayInteract.h"

class Double_Knob;
class KnobI;
class QPoint;
class QPointF;
class NodeGui;
struct DefaultOverlayPrivate;
class DefaultOverlay : public Natron::NatronOverlayInteractSupport
{
    
public:
    
    DefaultOverlay(const boost::shared_ptr<NodeGui>& node);
    
    ~DefaultOverlay();
    
    boost::shared_ptr<NodeGui> getNode() const;
    
    bool addPositionParam(const boost::shared_ptr<Double_Knob>& position);
    
    void draw(double time,const RenderScale& renderScale);
    
    bool penMotion(double time,
                   const RenderScale &renderScale,
                   const QPointF &penPos,
                   const QPoint &penPosViewport,
                   double pressure) ;
    
    
    bool penUp(double time,
               const RenderScale &renderScale,
               const QPointF &penPos,
               const QPoint &penPosViewport,
               double  pressure)  ;
    
    
    bool penDown(double time,
                 const RenderScale &renderScale,
                 const QPointF &penPos,
                 const QPoint &penPosViewport,
                 double  pressure)  ;
    
    
    bool keyDown(double time,
                 const RenderScale &renderScale,
                 int     key,
                 char*   keyString)  ;
    
    
    bool keyUp(double time,
               const RenderScale &renderScale,
               int     key,
               char*   keyString)  ;
    
    
    bool keyRepeat(double time,
                   const RenderScale &renderScale,
                   int     key,
                   char*   keyString)  ;
    
    
    bool gainFocus(double time,
                   const RenderScale &renderScale)  ;
    
    
    bool loseFocus(double  time,
                   const RenderScale &renderScale)  ;
    
    bool hasDefaultOverlayForParam(const KnobI* param) ;
    
private:
    
    boost::scoped_ptr<DefaultOverlayPrivate> _imp;
};

#endif // DEFAULTOVERLAYS_H
