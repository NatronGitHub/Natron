/*
Software License :

Copyright (c) 2014-2015, The Open Effects Association Ltd. All rights reserved.

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

#ifndef _ofxOLD_h_
#define _ofxOLD_h_

 
/** @brief String to label images with YUVA components
--ofxImageEffects.h
@deprecated - removed in v1.4. Note, this has been deprecated in v1.3

 */
#define kOfxImageComponentYUVA "OfxImageComponentYUVA"

/** @brief Indicates whether an effect is performing an analysis pass.
--ofxImageEffects.h
   - Type - int X 1
   - Property Set -  plugin instance (read/write)
   - Default - to 0
   - Valid Values - This must be one of 0 or 1

@deprecated - This feature has been deprecated - officially commented out v1.4.
*/
#define kOfxImageEffectPropInAnalysis "OfxImageEffectPropInAnalysis"


/** @brief Defines an 8 bit per component YUVA pixel 
-- ofxPixels.h
Deprecated in 1.3, removed in 1.4
*/

typedef struct OfxYUVAColourB {
  unsigned char y, u, v, a;
}OfxYUVAColourB;


/** @brief Defines an 16 bit per component YUVA pixel 
-- ofxPixels.h
@deprecated -  Deprecated in 1.3, removed in 1.4
*/

typedef struct OfxYUVAColourS {
  unsigned short y, u, v, a;
}OfxYUVAColourS;

/** @brief Defines an floating point component YUVA pixel
-- ofxPixels.h
@deprecated -  Deprecated in 1.3, removed in 1.4
 */
typedef struct OfxYUVAColourF {
  float y, u, v, a;
}OfxYUVAColourF;


/** @brief The size of an interact's openGL viewport
-- ofxInteract.h
    - Type - int X 2 
    - Property Set - read only property on the interact instance and in argument to all the interact actions.

@deprecated - V1.3: This property is the redundant and its use will be deprecated in future releases.
V1.4: Removed
 */
#define kOfxInteractPropViewportSize "OfxInteractPropViewport"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating a size normalised to the X dimension. See \ref ::kOfxParamPropDoubleType.
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::OfxParamDoubleTypeX
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedX  "OfxParamDoubleTypeNormalisedX"


/** @brief value for the ::kOfxParamPropDoubleType property, indicating a size normalised to the Y dimension. See \ref ::kOfxParamPropDoubleType.
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::OfxParamDoubleTypeY
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedY  "OfxParamDoubleTypeNormalisedY"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating an absolute position normalised to the X dimension. See \ref ::kOfxParamPropDoubleType. 
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::OfxParamDoubleTypeXAbsolute
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedXAbsolute  "OfxParamDoubleTypeNormalisedXAbsolute"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating an absolute position  normalised to the Y dimension. See \ref ::kOfxParamPropDoubleType.
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::OfxParamDoubleTypeYAbsolute
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedYAbsolute  "OfxParamDoubleTypeNormalisedYAbsolute"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating normalisation to the X and Y dimension for 2D params. See \ref ::kOfxParamPropDoubleType. 
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::OfxParamDoubleTypeXY
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedXY  "OfxParamDoubleTypeNormalisedXY"

/** @brief value for the ::kOfxParamPropDoubleType property, indicating normalisation to the X and Y dimension for a 2D param that can be interpretted as an absolute spatial position. See \ref ::kOfxParamPropDoubleType. 
-- ofxParam.h
@deprecated - V1.3: Deprecated in favour of ::kOfxParamDoubleTypeXYAbsolute 
V1.4: Removed
 */
#define kOfxParamDoubleTypeNormalisedXYAbsolute  "OfxParamDoubleTypeNormalisedXYAbsolute"



#endif
