/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "StrUtils.h"

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#else
#include <cstdlib>
#endif

NATRON_NAMESPACE_ENTER

namespace StrUtils {

    /*Converts a std::string to wide string*/
    std::wstring
            utf8_to_utf16(const std::string & s)
    {
#ifdef __NATRON_WIN32__
        std::wstring native;


        native.resize(MultiByteToWideChar (CP_UTF8, 0, s.c_str(), -1, NULL, 0) - 1);
        MultiByteToWideChar ( CP_UTF8, 0, s.c_str(), s.size(), &native[0], (int)native.size() );

        return native;

#else
        std::wstring dest;
        size_t max = s.size() * 4;
        mbtowc (NULL, NULL, max);  /* reset mbtowc */

        const char* cstr = s.c_str();

        while (max > 0) {
            wchar_t w;
            size_t length = mbtowc(&w, cstr, max);
            if (length < 1) {
                break;
            }
            dest.push_back(w);
            cstr += length;
            max -= length;
        }

        return dest;
#endif
    } // utf8_to_utf16

    std::string
            utf16_to_utf8 (const std::wstring& str)
    {
#ifdef __NATRON_WIN32__
        std::string utf8;

        utf8.resize(WideCharToMultiByte (CP_UTF8, 0, str.data(), str.length(), NULL, 0, NULL, NULL));
        WideCharToMultiByte (CP_UTF8, 0, str.data(), str.length(), &utf8[0], (int)utf8.size(), NULL, NULL);

        return utf8;
#else
        std::string utf8;
        for (std::size_t i = 0; i < str.size(); ++i) {
            char c[MB_LEN_MAX];
            int nbBytes = wctomb(c, str[i]);
            if (nbBytes > 0) {
                for (int j = 0; j < nbBytes; ++j) {
                    utf8.push_back(c[j]);
                }
            } else {
                break;
            }
        }
        return utf8;
#endif
    } // utf16_to_utf8

    void ensureLastPathSeparator(QString& path)
    {
        static const QChar separator( QLatin1Char('/') );

        if ( !path.endsWith(separator) ) {
            path += separator;
        }
    }

#ifdef __NATRON_WIN32__


    //Returns the last Win32 error, in string format. Returns an empty string if there is no error.
    std::string
            GetLastErrorAsString()
    {
        //Get the error message, if any.
        DWORD errorMessageID = ::GetLastError();

        if (errorMessageID == 0) {
            return std::string(); //No error message has been recorded
        }
        LPSTR messageBuffer = 0;
        size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
        std::string message(messageBuffer, size);

        //Free the buffer.
        LocalFree(messageBuffer);

        return message;
    } // GetLastErrorAsString

#endif // __NATRON_WIN32__
}

NATRON_NAMESPACE_EXIT
