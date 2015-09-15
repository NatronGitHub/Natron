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

#ifndef _Engine_RotoPoint_h_
#define _Engine_RotoPoint_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <list>
#include <set>
#include <string>

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include "Global/GlobalDefines.h"
#include "Engine/FitCurve.h"

CLANG_DIAG_OFF(deprecated-declarations)
#include <QObject>
#include <QMutex>
#include <QMetaType>
CLANG_DIAG_ON(deprecated-declarations)

#define kRotoLayerBaseName "Layer"
#define kRotoBezierBaseName "Bezier"
#define kRotoOpenBezierBaseName "Pencil"
#define kRotoEllipseBaseName "Ellipse"
#define kRotoRectangleBaseName "Rectangle"
#define kRotoPaintBrushBaseName "Brush"
#define kRotoPaintEraserBaseName "Eraser"
#define kRotoPaintBlurBaseName "Blur"
#define kRotoPaintSmearBaseName "Smear"
#define kRotoPaintSharpenBaseName "Sharpen"
#define kRotoPaintCloneBaseName "Clone"
#define kRotoPaintRevealBaseName "Reveal"
#define kRotoPaintDodgeBaseName "Dodge"
#define kRotoPaintBurnBaseName "Burn"

namespace Natron {
class Image;
class ImageComponents;
class Node;
}
namespace boost { namespace serialization { class access; } }

class RectI;
class RectD;
class KnobI;
class KnobBool;
class KnobDouble;
class KnobInt;
class KnobChoice;
class KnobColor;
typedef struct _cairo_pattern cairo_pattern_t;

class Curve;
class Bezier;
class RotoItemSerialization;
class BezierCP;

/**
 * @class A base class for all items made by the roto context
 **/
class RotoContext;
class RotoLayer;

namespace Transform {
struct Matrix3x3;
}



struct RotoPoint
{
    Natron::Point pos;
    double pressure;
    double timestamp;
    
    RotoPoint() : pos(), pressure(0), timestamp(0) {}

    RotoPoint(const Natron::Point &pos_, double pressure_, double timestamp_)
    : pos(pos_), pressure(pressure_), timestamp(timestamp_) {}

    RotoPoint(double x, double y, double pressure_, double timestamp_)
    : pressure(pressure_), timestamp(timestamp_) { pos.x = x; pos.y = y; }
};

#endif // _Engine_RotoPoint_h_
