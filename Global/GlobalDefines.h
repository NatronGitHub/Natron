//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef NATRON_GLOBAL_GLOBALDEFINES_H_
#define NATRON_GLOBAL_GLOBALDEFINES_H_

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#endif

#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif
#include <QtCore/QForeachContainer>
#include <QtCore/QMetaType>
#include "Global/Macros.h"
#include "Global/Enums.h"

#undef foreach
#define foreach Q_FOREACH


typedef boost::uint32_t U32;
typedef boost::uint64_t U64;
typedef boost::uint8_t U8;
typedef boost::uint16_t U16;

#include <ofxhImageEffect.h>

typedef int SequenceTime;

Q_DECLARE_METATYPE(SequenceTime)

typedef OfxPointD RenderScale;

#define kFrameRenderedString "Frame rendered: "
#define kProgressChangedString "Progress changed: "
#define kRenderingFinishedString "Rendering finished"
#define kAbortRenderingString "Abort rendering"

#define kNodeGraphObjectName "NodeGraph"
#define kCurveEditorObjectName "CurveEditor"

#define kCurveEditorMoveKeyCommandCompressionID 1
#define kCurveEditorMoveMultipleKeysCommandCompressionID 2
#define kKnobUndoChangeCommandCompressionID 3
#define kNodeGraphMoveNodeCommandCompressionID 4


#ifdef __NATRON_WIN32__
namespace NatronWindows{
    /*Converts a std::string to wide string*/
    inline std::wstring s2ws(const std::string& s)
    {
        int len;
        int slength = (int)s.length() + 1;
        len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
        wchar_t* buf = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
        std::wstring r(buf);
        delete[] buf;
        return r;
    }
}
#endif

#endif
