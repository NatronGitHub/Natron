/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_GLOBAL_GLOBALDEFINES_H
#define NATRON_GLOBAL_GLOBALDEFINES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#else
#include <cstdlib>
#endif
#include <fstream>

#include "Global/Macros.h"

#include <ofxCore.h>
#include <ofxPixels.h>

#undef toupper
#undef tolower
#undef isalpha
#undef isalnum

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/cstdint.hpp>
#endif
#include <QtCore/QForeachContainer>
#include <QtCore/QString>
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMetaType>
CLANG_DIAG_ON(deprecated)
#include "Global/Enums.h"


// boost and C++11 also have a foreach. this breaks it. DON'T UNCOMMENT THIS.
//#undef foreach
//#define foreach Q_FOREACH


typedef boost::uint32_t U32;
typedef boost::uint64_t U64;
typedef boost::uint8_t U8;
typedef boost::uint16_t U16;


NATRON_NAMESPACE_ENTER

typedef int SequenceTime;

struct RenderScale
    : public OfxPointD
{
    RenderScale() { x = y = 1.; }

    RenderScale(double scale) { assert(scale != 0.); x = y = scale; }

    RenderScale(double scaleX,
                double scaleY) { x = scaleX; y = scaleY; }
};

typedef OfxPointD Point;
typedef OfxRGBAColourF RGBAColourF;
typedef OfxRGBAColourD RGBAColourD;
typedef OfxRangeD RangeD;


///these are used between process to communicate via the pipes
#define kRenderingStartedShort "-b"

#define kFrameRenderedStringShort "-r"

#define kProgressChangedStringShort "-p"

#define kRenderingFinishedStringShort "-e"

#define kAbortRenderingStringShort "-a"

#define kBgProcessServerCreatedShort "--bg_server_created"

#define kNodeGraphObjectName "nodeGraph"
#define kAnimationModuleEditorObjectName "animationModule"

#define kCurveEditorMoveMultipleKeysCommandCompressionID 2
#define kKnobUndoChangeCommandCompressionID 3
#define kNodeGraphMoveNodeCommandCompressionID 4
#define kRotoMoveControlPointsCompressionID 5
#define kRotoMoveTangentCompressionID 6
#define kRotoCuspSmoothCompressionID 7
#define kRotoMoveFeatherBarCompressionID 8
#define kRotoMakeBezierCompressionID 9
#define kRotoMakeEllipseCompressionID 10
#define kRotoMakeRectangleCompressionID 11
#define kRotoTransformCompressionID 12
#define kMultipleKnobsUndoChangeCommandCompressionID 13
#define kNodeGraphMoveNodeBackdropCommandCompressionID 14
#define kNodeGraphResizeNodeBackdropCommandCompressionID 15
#define kCurveEditorMoveTangentsCommandCompressionID 16
#define kCurveEditorTransformKeysCommandCompressionID 17
#define kAnimationModuleEditorMoveKeysCommandCompressionID 18
#define kAnimationModuleEditorLeftTrimCommandCompressionID 19
#define kAnimationModuleEditorRightTrimCommandCompressionID 20
#define kAnimationModuleEditorSlipReaderCommandCompressionID 23
#define kNodeUndoChangeCommandCompressionID 24


NATRON_NAMESPACE_EXIT

Q_DECLARE_METATYPE(NATRON_NAMESPACE::SequenceTime)

#endif // ifndef NATRON_GLOBAL_GLOBALDEFINES_H
