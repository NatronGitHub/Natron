#ifndef _ofxInteract_h_
#define _ofxInteract_h_

#include "ofxCore.h"

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


#ifdef __cplusplus
extern "C" {
#endif

/** @file ofxInteract.h
Contains the API for ofx plugin defined GUIs and interaction.
*/

#define kOfxInteractSuite "OfxInteractSuite"


/** @brief Blind declaration of an OFX interactive gui 
*/
typedef struct OfxInteract *OfxInteractHandle;

/**
   \addtogroup PropertiesAll
*/
/*@{*/
/**
   \defgroup PropertiesInteract Interact Property Definitions

These are the list of properties used by the Interact API documented in \ref CustomInteractionPage.
*/
/*@{*/
/** @brief The set of parameters on which a value change will trigger a redraw for an interact.

   - Type - string X N
   - Property Set - interact instance property (read/write)
   - Default - no values set
   - Valid Values - the name of any parameter associated with this interact.

If the interact is representing the state of some set of OFX parameters, then is will
need to be redrawn if any of those parameters' values change. This multi-dimensional property
links such parameters to the interact.

The interact can be slaved to multiple parameters (setting index 0, then index 1 etc...)
 */
#define kOfxInteractPropSlaveToParam "OfxInteractPropSlaveToParam"

/** @brief The size of a real screen pixel under the interact's canonical projection.

   - Type - double X 2
   - Property Set - interact instance and actions (read only)

 */
#define kOfxInteractPropPixelScale "OfxInteractPropPixelScale"

/** @brief The size of an interact's openGL viewport

    - Type - int X 2 
    - Property Set - read only property on the interact instance and in argument to all the interact actions.

This property is the redundant and its use will be deprecated in future releases.
 */
#define kOfxInteractPropViewportSize "OfxInteractPropViewport"

/** @brief The background colour of the application behind an interact instance

    - Type - double X 3
    - Property Set - read only on the interact instance and in argument to the ::kOfxInteractActionDraw action
    - Valid Values - from 0 to 1

The components are in the order red, green then blue.

 */
#define kOfxInteractPropBackgroundColour "OfxInteractPropBackgroundColour"

/** @brief The suggested colour to draw a widget in an interact, typically for overlays.
 
    - Type - double X 3
    - Property Set - read only on the interact instance
    - Default - 1.0
    - Valid Values - greater than or equal to 0.0

Some applications allow the user to specify colours of any overlay via a colour picker, this
property represents the value of that colour. Plugins are at liberty to use this or not when
they draw an overlay.

If a host does not support such a colour, it should return kOfxStatReplyDefault
*/
#define kOfxInteractPropSuggestedColour "OfxInteractPropSuggestedColour"

/** @brief The position of the pen in an interact.

   - Type - double X 2
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions

This value passes the postion of the pen into an interact. This is in the interact's canonical coordinates.
 */
#define kOfxInteractPropPenPosition "OfxInteractPropPenPosition"

/** @brief The position of the pen in an interact in viewport coordinates.

   - Type - int X 2
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions

This value passes the postion of the pen into an interact. This is in the interact's openGL viewport coordinates, with 0,0 being at the bottom left.
 */
#define kOfxInteractPropPenViewportPosition "OfxInteractPropPenViewportPosition"

/** @brief The pressure of the pen in an interact.

   - Type - double X 1
   - Property Set - read only in argument to the ::kOfxInteractActionPenMotion, ::kOfxInteractActionPenDown and ::kOfxInteractActionPenUp actions
   - Valid Values - from 0 (no pressure) to 1 (maximum pressure)

This is used to indicate the status of the 'pen' in an interact. If a pen has only two states (eg: a mouse button), these should map to 0.0 and 1.0.
 */
#define kOfxInteractPropPenPressure "OfxInteractPropPenPressure"

/** @brief Indicates whether the dits per component in the interact's openGL frame buffer

   - Type - int X 1
   - Property Set - interact instance and descriptor (read only)

 */
#define kOfxInteractPropBitDepth "OfxInteractPropBitDepth"

/** @brief Indicates whether the interact's frame buffer has an alpha component or not

   - Type - int X 1
   - Property Set - interact instance and descriptor (read only)
   - Valid Values - This must be one of
       - 0 indicates no alpha component
       - 1 indicates an alpha component
 */
#define kOfxInteractPropHasAlpha "OfxInteractPropHasAlpha"

/*@}*/
/*@}*/

/**
   \addtogroup ActionsAll
*/
/*@{*/
/**
   \defgroup InteractActions Interact Actions

These are the list of actions passed to an interact's entry point function. For more details on how to deal with actions, see \ref InteractActions.
*/
/*@{*/

/** @brief Action passed to interacts telling it to redraw, see \ref InteractsActionDraw for more details */
#define kOfxInteractActionDraw "OfxInteractActionDraw"

/** @brief Action passed to interacts for an interact pen motion , see \ref InteractsActionPen for more details
 */
#define kOfxInteractActionPenMotion "OfxInteractActionPenMotion"

/**@brief Action passed to interacts for a pen down , see \ref InteractsActionPen for more details
 */
#define kOfxInteractActionPenDown   "OfxInteractActionPenDown"

/**@brief Action passed to interacts for a pen up, see \ref InteractsActionPen for more details
 */
#define kOfxInteractActionPenUp     "OfxInteractActionPenUp"

/**@brief Action passed to interacts for a key down, see \ref InteractsActionKey for more details
 */
#define kOfxInteractActionKeyDown   "OfxInteractActionKeyDown"

/**@brief Action passed to interacts for a key down, see \ref InteractsActionKey for more details

 */
#define kOfxInteractActionKeyUp     "OfxInteractActionKeyUp"

/**@brief Action passed to interacts for a key repeat , see \ref InteractsActionKey for more details

 */
#define kOfxInteractActionKeyRepeat     "OfxInteractActionKeyRepeat"

/**@brief Action passed to interacts for a gain of input focus, see \ref InteractsActionFocus for more details
 */
#define kOfxInteractActionGainFocus "OfxInteractActionGainFocus"

/**@brief Action passed to interacts for a loss of input focus, see \ref InteractsActionFocus for more details
 */
#define kOfxInteractActionLoseFocus "OfxInteractActionLoseFocus"

/*@}*/
/*@}*/

/** @brief OFX suite that allows an effect to interact with an openGL window so as to provide custom interfaces.

*/
typedef struct OfxInteractSuiteV1 {	
  /** @brief Requests an openGL buffer swap on the interact instance */
  OfxStatus (*interactSwapBuffers)(OfxInteractHandle interactInstance);

  /** @brief Requests a redraw of the interact instance */
  OfxStatus (*interactRedraw)(OfxInteractHandle interactInstance);

  /** @brief Gets the property set handle for this interact handle */
  OfxStatus (*interactGetPropertySet)(OfxInteractHandle interactInstance,
				      OfxPropertySetHandle *property);
} OfxInteractSuiteV1;

#ifdef __cplusplus
}
#endif

#endif
