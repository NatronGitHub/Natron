//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#ifndef POWITER_GLOBAL_GLOBALDEFINES_H_
#define POWITER_GLOBAL_GLOBALDEFINES_H_

#include <utility>
#if defined(_WIN32)
#include <string>
#define NOMINMAX ///< Qt5 bug workaround with qdatetime.h
#include <windows.h>
#define OPENEXR_DLL
#endif

#ifndef Q_MOC_RUN
#include <boost/cstdint.hpp>
#endif
#include <QtCore/QForeachContainer>

#include "Global/Enums.h"

#undef foreach
#define foreach Q_FOREACH

#ifdef __APPLE__
#define __POWITER_OSX__
#define __POWITER_UNIX__
#elif  defined(_WIN32)
#define __POWITER_WIN32__
#define OPENEXR_DLL
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#define __POWITER_UNIX__
#define __POWITER_LINUX__
#endif

#define POWITER_ROOT "/"
#define POWITER_CACHE_ROOT_PATH "./"
#define POWITER_IMAGES_PATH ":/Resources/Images/"
#define POWITER_PLUGINS_PATH POWITER_ROOT"Plugins"

// debug flag
#define POWITER_DEBUG

typedef boost::uint32_t U32;
typedef boost::uint64_t U64;
typedef boost::uint8_t U8;
typedef boost::uint16_t U16;

#ifdef __POWITER_WIN32__
namespace PowiterWindows{
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
