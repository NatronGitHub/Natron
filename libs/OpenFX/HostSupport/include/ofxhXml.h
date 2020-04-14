#ifndef OFX_XML_H
#define OFX_XML_H

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
#include <sstream> // stringstream

namespace OFX {

  namespace XML {

    inline std::string escape(const std::string &s) {
      std::string ns;
      for (size_t i=0;i<s.size();i++) {
        // The are exactly five characters which must be escaped
        // http://www.w3.org/TR/xml/#syntax
        switch (s[i]) {
          case '<':
            ns += "&lt;";
            break;
          case '>':
            ns += "&gt;";
            break;
          case '&':
            ns += "&amp;";
            break;
          case '"':
            ns += "&quot;";
            break;
          case '\'':
            ns += "&apos;";
            break;
          default: {
            unsigned char c = (unsigned char)(s[i]);
            // Escape even the whitespace characters '\n' '\r' '\t', although they are valid
            // XML, because they would be converted to space when re-read.
            // See http://www.w3.org/TR/xml/#AVNormalize
            if ((0x01 <= c && c <= 0x1f) || (0x7F <= c && c <= 0x9F)) {
              // these characters must be escaped in XML 1.1
              // http://www.w3.org/TR/xml/#sec-references
              ns += "&#x";
              if (c > 0xf) {
                int d = c / 0x10;
                ns += ('0' + d); // d cannot be more than 9 (because c <= 0x9F)
              }
              int d = c & 0xf;
              ns += d < 10 ? ('0' + d) : ('A' + d - 10);
              ns += ';';
            } else {
              ns += s[i];
            }
          } break;
        }
      }
      return ns;
    }

    inline std::string attribute(const std::string &at, const std::string &val)
    {
      return at + "=" + "\"" + escape(val) + "\" ";
    }

    inline std::string attribute(const std::string &st, int val)
    {
      std::ostringstream o;
      o << val;
      return attribute(st, o.str());
    }

  }
}

#endif
