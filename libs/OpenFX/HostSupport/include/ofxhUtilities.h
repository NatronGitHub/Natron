#ifndef _ofxhUtilities_h_
#define _ofxhUtilities_h_

/*
Software License :

Copyright (c) 2007-2009, The Open Effects Association Ltd. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
* Neither the name The Open Effects Association Ltd, nor the names of its 
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>
#include <list>
#include <vector>
#if defined(_MSC_VER)
#include <float.h> // _isnan
#endif
#include <cmath> // isnan, std::isnan

#include "ofxCore.h"

// macro that intercepts any exception that passes through a plugin's entry point, and transforms it into a message on the host using Host::vmessage()
#define CatchAllSetStatus(stat,host,plugin,msg)                         \
  catch ( const std::bad_alloc& ba ) {                                  \
    (stat) = kOfxStatErrMemory;                                         \
    if (host) {                                                         \
      try {                                                             \
        (host)->message(kOfxMessageError, "",                           \
                        "%s: Memory allocation error occured in plugin %s (%s)", \
                        (msg), (plugin)->pluginIdentifier, ba.what());  \
      } catch (...) {                                                   \
      }                                                                 \
    }                                                                   \
  } catch ( const std::exception &e ) {                                 \
    (stat) = kOfxStatFailed;                                            \
    if (host) {                                                         \
      try {                                                             \
        (host)->message(kOfxMessageError, "",                           \
                        "%s: Exception occured in plugin %s (%s)",      \
                        (msg), (plugin)->pluginIdentifier, e.what());   \
      } catch (...) {                                                   \
      }                                                                 \
    }                                                                   \
  } catch ( ... ) {                                                     \
    (stat) = kOfxStatFailed;                                            \
    if (host) {                                                         \
      try {                                                             \
        (host)->message(kOfxMessageError, "",                           \
                        "%s:Exception occured in plugin %s",            \
                        (msg), (plugin)->pluginIdentifier);             \
      } catch (...) {                                                   \
      }                                                                 \
    }                                                                   \
  }

namespace OFX {

  /// class that is a std::vector of std::strings
  typedef std::vector<std::string> StringVec;

  /// class that is a std::vector of std::strings
  inline void SetStringVecValue(StringVec &sv, const std::string &value, size_t index = 0) 
  {
    size_t size = sv.size();
    if(size <= index) {
      while(size < index) {
        sv.push_back("");
        ++size;
      }
      sv.push_back(value);
    }
    else 
      sv[index] = value;
  }

  /// get me deepest bit depth 
  std::string FindDeepestBitDepth(const std::string &s1, const std::string &s2);

  /// get the min value
  template<class T> inline T Minimum(const T &a, const T &b)
  {
    return a < b ? a : b;
  }

  /// get the min value
  template<class T> inline T Maximum(const T &a, const T &b)
  {
    return a > b ? a : b;
  }

  /// clamp the value
  template<class T> inline T Clamp(const T &v, const T &mn, const T &mx)
  {
    if(v < mn) return mn;
    if(v > mx) return mx;
    return v;
  }

  /// clamp the rect in v to the given bounds
  inline OfxRectD Clamp(const OfxRectD &v,
                        const OfxRectD &bounds)
  {
    OfxRectD r;
    r.x1 = Clamp(v.x1, bounds.x1, bounds.x2);
    r.x2 = Clamp(v.x2, bounds.x1, bounds.x2);
    r.y1 = Clamp(v.y1, bounds.y1, bounds.y2);
    r.y2 = Clamp(v.y2, bounds.y1, bounds.y2);
    return r;
  }

  /// get the union of the two rects
  inline OfxRectD Union(const OfxRectD &a,
                        const OfxRectD &b)
  {
    OfxRectD r;
    r.x1 = Minimum(a.x1, b.x1);
    r.x2 = Maximum(a.x2, b.x2);
    r.y1 = Minimum(a.y1, b.y1);
    r.y2 = Maximum(a.y2, b.y2);
    return r;
  }

  inline const char* StatStr(OfxStatus stat) {
    switch(stat) {
      case kOfxStatOK:
        return "kOfxStatOK";
      case kOfxStatFailed:
        return "kOfxStatFailed";
      case kOfxStatErrFatal:
        return "kOfxStatErrFatal";
      case kOfxStatErrUnknown:
        return "kOfxStatErrUnknown";
      case kOfxStatErrMissingHostFeature:
        return "kOfxStatErrMissingHostFeature";
      case kOfxStatErrUnsupported:
        return "kOfxStatErrUnsupported";
      case kOfxStatErrExists:
        return "kOfxStatErrExists";
      case kOfxStatErrFormat:
        return "kOfxStatErrFormat";
      case kOfxStatErrMemory:
        return "kOfxStatErrMemory";
      case kOfxStatErrBadHandle:
        return "kOfxStatErrBadHandle";
      case kOfxStatErrBadIndex:
        return "kOfxStatErrBadIndex";
      case kOfxStatErrValue:
        return "kOfxStatErrValue";
      case kOfxStatReplyYes:
        return "kOfxStatReplyYes";
      case kOfxStatReplyNo:
        return "kOfxStatReplyNo";
      case kOfxStatReplyDefault:
        return "kOfxStatReplyDefault";
      default:
        return "(unknown error code)";
    }
  }

#if defined(_MSC_VER)
  inline bool IsInfinite(double x) { return _finite(x) == 0 && _isnan(x) == 0; }
  inline bool IsNaN     (double x) { return _isnan(x) != 0;                    }
#else
#  if __cplusplus >= 201103L
  // These definitions are for the normal Unix suspects.
  inline bool IsInfinite(double x) { return (std::isinf)(x);    }
  inline bool IsNaN     (double x) { return (std::isnan)(x);    }
#  else
#    ifdef isnan // isnan is defined as a macro
  inline bool IsInfinite(double x) { return isinf(x);    }
  inline bool IsNaN     (double x) { return isnan(x);    }
#    else
  inline bool IsInfinite(double x) { return ::isinf(x);    }
  inline bool IsNaN     (double x) { return ::isnan(x);    }
#    endif
#  endif
#endif

# ifdef WINDOWS
  std::wstring  utf8_to_utf16(const std::string& s);
  std::string  utf16_to_utf8(const std::wstring& s);
# endif
}
#endif


