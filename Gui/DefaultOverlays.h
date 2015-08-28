
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


#ifndef DEFAULTOVERLAYS_H
#define DEFAULTOVERLAYS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

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
    
    void removeDefaultOverlay(KnobI* knob);
    
    bool isEmpty() const;
    
private:
    
    boost::scoped_ptr<DefaultOverlayPrivate> _imp;
};

#endif // DEFAULTOVERLAYS_H
