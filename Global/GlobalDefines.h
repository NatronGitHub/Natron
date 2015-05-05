//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_GLOBAL_GLOBALDEFINES_H_
#define NATRON_GLOBAL_GLOBALDEFINES_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#else
#include <cstdlib>
#endif

#include "Global/Macros.h"

#include <ofxhImageEffect.h>
#include <ofxPixels.h>

#undef toupper
#undef tolower
#undef isalpha
#undef isalnum

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/cstdint.hpp>
#endif
#include <QtCore/QForeachContainer>
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




typedef int SequenceTime;

Q_DECLARE_METATYPE(SequenceTime)

typedef OfxPointD RenderScale;

namespace Natron {
typedef OfxPointD Point;
}

typedef OfxRGBAColourF RGBAColourF;
typedef OfxRangeD RangeD;

///these are used between process to communicate via the pipes
#define kRenderingStartedLong "Rendering started"
#define kRenderingStartedShort "-b"

#define kFrameRenderedStringLong "Frame rendered: "
#define kFrameRenderedStringShort "-r"

#define kProgressChangedStringLong "Progress changed: "
#define kProgressChangedStringShort "-p"

#define kRenderingFinishedStringLong "Rendering finished"
#define kRenderingFinishedStringShort "-e"

#define kAbortRenderingStringLong "Abort rendering"
#define kAbortRenderingStringShort "-a"

#define kBgProcessServerCreatedShort "--bg_server_created"


#define kNodeGraphObjectName "nodeGraph"
#define kCurveEditorObjectName "curveEditor"
#define kDopeSheetEditorObjectName "dopeSheetEditor"

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
#define kNodeGraphMoveNodeBackDropCommandCompressionID 14
#define kNodeGraphResizeNodeBackDropCommandCompressionID 15
#define kCurveEditorMoveTangentsCommandCompressionID 16
#define kCurveEditorTransformKeysCommandCompressionID 17
#define kDopeSheetEditorMoveKeysCommandCompressionID 18
#define kDopeSheetEditorLeftTrimCommandCompressionID 19
#define kDopeSheetEditorRightTrimCommandCompressionID 20
#define kDopeSheetEditorMoveClipCommandCompressionID 21
#define kDopeSheetEditorMoveGroupCommandCompressionID 22

namespace Natron {
/*Converts a std::string to wide string*/
inline std::wstring
s2ws(const std::string & s)
{


#ifdef __NATRON_WIN32__
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
#else
    std::wstring dest;

    size_t max = s.size() * 4;
    mbtowc (NULL, NULL, max);  /* reset mbtowc */

    const char* cstr = s.c_str();

    while (max > 0) {
        wchar_t w;
        size_t length = mbtowc(&w,cstr,max);
        if (length < 1) {
            break;
        }
        dest.push_back(w);
        cstr += length;
        max -= length;
    }
    return dest;
#endif

}
}

#endif // ifndef NATRON_GLOBAL_GLOBALDEFINES_H_
