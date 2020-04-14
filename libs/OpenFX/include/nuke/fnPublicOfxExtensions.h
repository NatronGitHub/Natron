//------------------------------------------------------------------------------
// Apps/Nuke/nuke/src/fnPublicOfxExtensions.h
//
// Copyright (c) 2009 The Foundry Visionmongers Ltd.  All Rights Reserved.
//------------------------------------------------------------------------------

#ifndef _fnPublicOfxExtensions_h_
#define _fnPublicOfxExtensions_h_

/*
   Software License :

   Copyright (c) 2009, The Foundry Visionmongers Ltd. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
 * Neither the name The Foundry Visionmongers Ltd, nor the names of its
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

#include "ofxImageEffect.h"

/** @brief Layout hint for hierarchical layouts

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 0
    - Valid Values - 0,1 or 2
        0 - for a new line after the parameter
        1 - for a seperator between this parameter and the one to follow
        2 - for no new line, continue the next parameter on the same horizontal level

   This is a property on parameters of type ::kOfxParamPropLayoutHint, and tells the group whether it should be open or closed by default.

 */
#define kOfxParamPropLayoutHint "OfxParamPropLayoutHint"

//   lay out as normal
#define kOfxParamPropLayoutHintNormal 0

//   put a divider after parameter
#define kOfxParamPropLayoutHintDivider 1

//   have the next parameter start on the same line as this
#define kOfxParamPropLayoutHintNoNewLine 2

/** @brief Layout padding for hierarchical views, only pertinent with kOfxParamPropLayoutHint==2

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 0
    - Valid Values - any positive integer value

   This is a property on parameters of type ::kOfxParamPropLayoutPadWidth
   It tells the host how much space (in pixels) to leave between the current parameter and the next parameter in horizontal layouts.
 */
#define kOfxParamPropLayoutPadWidth "OfxParamPropLayoutPadWidth"

/** @brief The suggested colour of an overlay colour in an interact.

    - Type - double X 3
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 1.0
    - Valid Values - greater than or equal to 0.0

   This is a property of an overlay interact instance.
 */
#define kOfxPropOverlayColour "OfxPropOverlayColour"

/** @brief Unique user readable version string for that identifies something from other versions

    - Type - string X 1
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - none, the host needs to set this
    - Valid Values - ASCII string
 */
#define kOfxPropVersionLabel "OfxPropVersionLabel"

/** @brief The displayed named of the host

    - Type - string X 1
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - none
    - Valid Values - ASCII string
 */
#define kOfxPropHostProductTitle "OfxPropHostProductTitle"

/** @brief The major version of the host

    - Type - int X 1
    - Property Set - param set instance (read/write)
    - Default - none
    - Valid Values - any positive integer values
 */
#define kOfxPropHostMajorVersion "OfxPropHostMajorVersion"

/** @brief The minor version of the host

    - Type - int X 1
    - Property Set - param set instance (read/write)
    - Default - none
    - Valid Values - any positive integer values
 */
#define kOfxPropHostMinorVersion "OfxPropHostMinorVersion"

/** @brief The build version of the host

    - Type - string X 1
    - Property Set - host descriptor (read only), plugin descriptor (read/write)
    - Default - none
    - Valid Values - ASCII string
 */
#define kOfxPropHostBuildVersion "OfxPropHostBuildVersion"

/** @brief Whether to display a group as a tab

    - Type - int X 1
    - Property Set - plugin parameter descriptor (read/write) and instance (read only)
    - Default - 0
    - Valid Values - 0 or 1
 */
#define kFnOfxParamPropGroupIsTab "FnOfxParamPropGroupIsTab"


#endif
