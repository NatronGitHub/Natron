//  Powiter
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef PowiterOsX_powiterFn_h
#define PowiterOsX_powiterFn_h
#include "Superviser/Enums.h"
#include <string>
#include <utility>
#ifdef __APPLE__
#define __POWITER_OSX__
#define __POWITER_UNIX__
#elif  defined(_WIN32)
#define __POWITER_WIN32__
#define NOMINMAX // < Qt5 bug workaround with qdatetime.h
#include <windows.h>
#define OPENEXR_DLL
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#define __POWITER_UNIX__
#define __POWITER_LINUX__
#endif


#define ROOT "./"
#define CACHE_ROOT_PATH "./"
#define IMAGES_PATH ":/Resources/Images/"
#define PLUGINS_PATH ROOT"Plugins"


// debug flag
#define PW_DEBUG

#include <boost/cstdint.hpp>
typedef boost::uint32_t U32;
typedef boost::uint64_t U64;
typedef boost::uint8_t U8;
typedef boost::uint16_t U16;


/*see LRUcache.h for an explanation of these cache related defines*/
//#define USE_VARIADIC_TEMPLATES
#define CACHE_USE_HASH
#define CACHE_USE_BOOST


namespace PowiterWindows{
    
#ifdef __POWITER_WIN32__

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

#endif

}
#ifdef __POWITER_WIN32__
#undef strlcpy
#undef strcpy
#undef strlen
/*Windows fix to have safe c string handling functions*/

inline size_t strlen(const char* src){
	const char *eos = src;
	while( *eos++ ) ;
	return( eos - src - 1 );
}

inline size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t res = strlen(src);
	if (size > 0) {
		size_t length = (res >= size) ? size - 1 : res;
		memcpy(dest, src, length);
		dest[length]='\0';
	}
	return res;
}

inline char* strcpy(char* dest,const char* src){
	size_t res = strlen(src);
	if (res > 0) {
		memcpy(dest, src, res);
		dest[res]='\0';
	}
	return dest;
}

#endif
#endif
