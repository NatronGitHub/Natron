/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "StrUtils.h"

#include <utility>
#if defined(_WIN32)
#include <string>
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#else
#include <cstdlib>
#include <climits>
#endif

#include <vector>
#include <algorithm>



static char separator()
{
#if defined (__NATRON_WIN32__)
    return '\\';
#else
    return '/';
#endif
}

static bool
endsWith(const std::string& str,
         const std::string& suffix)
{
    return ( ( str.size() >= suffix.size() ) &&
            (str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) );
}

NATRON_NAMESPACE_ENTER

namespace StrUtils {

    bool is_utf8(const char * string)
    {
        if(!string)
            return false;

        const unsigned char * bytes = (const unsigned char *)string;
        while(*bytes)
        {
            if( (// ASCII
                 // use bytes[0] <= 0x7F to allow ASCII control characters
                 bytes[0] == 0x09 ||
                 bytes[0] == 0x0A ||
                 bytes[0] == 0x0D ||
                 (0x20 <= bytes[0] && bytes[0] <= 0x7E)
                 )
               ) {
                bytes += 1;
                continue;
            }

            if( (// non-overlong 2-byte
                 (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
                 (0x80 <= bytes[1] && bytes[1] <= 0xBF)
                 )
               ) {
                bytes += 2;
                continue;
            }

            if( (// excluding overlongs
                 bytes[0] == 0xE0 &&
                 (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
                 (0x80 <= bytes[2] && bytes[2] <= 0xBF)
                 ) ||
               (// straight 3-byte
                ((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
                 bytes[0] == 0xEE ||
                 bytes[0] == 0xEF) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
                ) ||
               (// excluding surrogates
                bytes[0] == 0xED &&
                (0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF)
                )
               ) {
                bytes += 3;
                continue;
            }

            if( (// planes 1-3
                 bytes[0] == 0xF0 &&
                 (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
                 (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                 (0x80 <= bytes[3] && bytes[3] <= 0xBF)
                 ) ||
               (// planes 4-15
                (0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
                (0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
                ) ||
               (// plane 16
                bytes[0] == 0xF4 &&
                (0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
                (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
                (0x80 <= bytes[3] && bytes[3] <= 0xBF)
                )
               ) {
                bytes += 4;
                continue;
            }
            
            return false;
        }
        
        return true;
    } // is_utf8
    
    /*Converts a std::string to wide string*/
    std::wstring
            utf8_to_utf16(const std::string & str)
    {
#ifdef __NATRON_WIN32__
        std::wstring native;


        native.resize(MultiByteToWideChar (CP_UTF8, 0, str.data(), str.length(), NULL, 0));
        MultiByteToWideChar ( CP_UTF8, 0, str.data(), str.length(), &native[0], (int)native.size() );

        return native;

#else
        std::wstring dest;
        size_t max = str.size() * 4;
        mbtowc (NULL, NULL, max);  /* reset mbtowc */

        const char* cstr = str.c_str();

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


    std::string cleanPath(const std::string &path)
    {
        if (path.empty())
            return path;
        std::string name = path;
        char dir_separator = separator();
        if (dir_separator != '/') {
            std::replace( name.begin(), name.end(), dir_separator, '/');
        }

        int used = 0, levels = 0;
        const int len = name.length();
        std::vector<char> outVector(len);
        char *out = &outVector[0];

        const char *p = name.c_str();
        for (int i = 0, last = -1, iwrite = 0; i < len; ++i) {
            if (p[i] == '/') {
                while (i+1 < len && p[i+1] == '/') {
#if defined(__NATRON_WIN32__) //allow unc paths
                    if (!i)
                        break;
#endif
                    i++;
                }
                bool eaten = false;
                if (i+1 < len && p[i+1] == '.') {
                    int dotcount = 1;
                    if (i+2 < len && p[i+2] == '.')
                        dotcount++;
                    if (i == len - dotcount - 1) {
                        if (dotcount == 1) {
                            break;
                        } else if (levels) {
                            if (last == -1) {
                                for (int i2 = iwrite-1; i2 >= 0; i2--) {
                                    if (out[i2] == '/') {
                                        last = i2;
                                        break;
                                    }
                                }
                            }
                            used -= iwrite - last - 1;
                            break;
                        }
                    } else if (p[i+dotcount+1] == '/') {
                        if (dotcount == 2 && levels) {
                            if (last == -1 || iwrite - last == 1) {
                                for (int i2 = (last == -1) ? (iwrite-1) : (last-1); i2 >= 0; i2--) {
                                    if (out[i2] == '/') {
                                        eaten = true;
                                        last = i2;
                                        break;
                                    }
                                }
                            } else {
                                eaten = true;
                            }
                            if (eaten) {
                                levels--;
                                used -= iwrite - last;
                                iwrite = last;
                                last = -1;
                            }
                        } else if (dotcount == 2 && i > 0 && p[i - 1] != '.') {
                            eaten = true;
                            used -= iwrite - std::max(0, last);
                            iwrite = std::max(0, last);
                            last = -1;
                            ++i;
                        } else if (dotcount == 1) {
                            eaten = true;
                        }
                        if (eaten)
                            i += dotcount;
                    } else {
                        levels++;
                    }
                } else if (last != -1 && iwrite - last == 1) {
#if defined(__NATRON_WIN32__)
                    eaten = (iwrite > 2);
#else
                    eaten = true;
#endif
                    last = -1;
                } else if (last != -1 && i == len-1) {
                    eaten = true;
                } else {
                    levels++;
                }
                if (!eaten)
                    last = i - (i - iwrite);
                else
                    continue;
            } else if (!i && p[i] == '.') {
                int dotcount = 1;
                if (len >= 1 && p[1] == '.')
                    dotcount++;
                if (len >= dotcount && p[dotcount] == '/') {
                    if (dotcount == 1) {
                        i++;
                        while (i+1 < len-1 && p[i+1] == '/')
                            i++;
                        continue;
                    }
                }
            }
            out[iwrite++] = p[i];
            used++;
        }

        std::string ret;
        if (used == len) {
            ret = name;
        } else {
            for (int i = 0; i < used; ++i) {
                ret.push_back(out[i]);
            }
        }
        // Strip away last slash except for root directories
        if (ret.length() > 1 && endsWith(ret, std::string("/"))) {
#if defined(__NATRON_WIN32__)
            if (!(ret.length() == 3 && ret.at(1) == ':'))
#endif
                ret.resize(ret.size() - 1);
        }

        return ret;
    } // cleanPath



    std::string toNativeSeparators(const std::string &pathName)
    {
#if defined(__NATRON_WIN32__)
        std::size_t i = pathName.find_first_of("/");
        if (i != std::string::npos) {
            std::string n(pathName);
            n[i++] = '\\';

            for (; i < n.length(); ++i) {
                if (n[i] == '/')
                    n[i] = '\\';
            }
            
            return n;
        }
#endif
        return pathName;
    }

    std::string fromNativeSeparators(const std::string &pathName)
    {
#if defined(__NATRON_WIN32__)
        std::size_t i = pathName.find_first_of("\\");
        if (i != std::string::npos) {
            std::string n(pathName);
            n[i++] = '/';

            for (; i < n.length(); ++i) {
                if (n[i] == '\\')
                    n[i] = '/';
            }

            return n;
        }
#endif
        return pathName;
    }

    std::vector<std::string> split(const std::string &text, char sep) {
        std::vector<std::string> tokens;
        std::size_t start = 0, end = 0;
        while ((end = text.find(sep, start)) != std::string::npos) {
            if (end != start) {
                tokens.push_back(text.substr(start, end - start));
            }
            start = end + 1;
        }
        if (end != start) {
            tokens.push_back(text.substr(start));
        }
        return tokens;
    }

    std::string join(const std::vector<std::string> &text, char sep)
    {
        std::string ret;
        for (std::size_t i = 0; i < text.size(); ++i) {
            ret += text[i];
            if (i < text.size() - 1) {
                ret += sep;
            }
        }
        return ret;
    }

} // StrUtils

NATRON_NAMESPACE_EXIT
