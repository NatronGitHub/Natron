#ifndef _ofxPixels_h_
#define _ofxPixels_h_

/*
Software License :

Copyright (c) 2009-15, The Open Effects Association Ltd. All rights reserved.

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


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxPixels.h
Contains pixel struct definitions
*/

/** @brief Defines an 8 bit per component RGBA pixel */
typedef struct OfxRGBAColourB {
  unsigned char r, g, b, a;
}OfxRGBAColourB;

/** @brief Defines a 16 bit per component RGBA pixel */
typedef struct OfxRGBAColourS {
  unsigned short r, g, b, a;
}OfxRGBAColourS;

/** @brief Defines a floating point component RGBA pixel */
typedef struct OfxRGBAColourF {
  float r, g, b, a;
}OfxRGBAColourF;


/** @brief Defines a double precision floating point component RGBA pixel */
typedef struct OfxRGBAColourD {
  double r, g, b, a;
}OfxRGBAColourD;


/** @brief Defines an 8 bit per component RGB pixel */
typedef struct OfxRGBColourB {
  unsigned char r, g, b;
}OfxRGBColourB;

/** @brief Defines a 16 bit per component RGB pixel */
typedef struct OfxRGBColourS {
  unsigned short r, g, b;
}OfxRGBColourS;

/** @brief Defines a floating point component RGB pixel */
typedef struct OfxRGBColourF {
  float r, g, b;
}OfxRGBColourF;

/** @brief Defines a double precision floating point component RGB pixel */
typedef struct OfxRGBColourD {
  double r, g, b;
}OfxRGBColourD;

#ifdef __cplusplus
}
#endif

#endif
