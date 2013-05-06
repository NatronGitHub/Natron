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

#ifndef _ofxTimeLine_h_
#define _ofxTimeLine_h_

/** @brief Name of the time line suite */
#define kOfxTimeLineSuite "OfxTimeLineSuite"

/** @brief Suite to control timelines 

    This suite is used to enquire and control a timeline associated with a plug-in
    instance.

    This is an optional suite in the Image Effect API.
*/
typedef struct OfxTimeLineSuiteV1 {
  /** @brief Get the time value of the timeline that is controlling to the indicated effect.
      
      \arg instance - is the instance of the effect changing the timeline, cast to a void *
      \arg time - a pointer through which the timeline value should be returned
       
      This function returns the current time value of the timeline associated with the effect instance.

      @returns
      - ::kOfxStatOK - the time enquiry was sucessful
      - ::kOfxStatFailed - the enquiry failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
  */
  OfxStatus (*getTime)(void *instance, double *time);

  /** @brief Move the timeline control to the indicated time.
      
      \arg instance - is the instance of the effect changing the timeline, cast to a void *
      \arg time - is the time to change the timeline to. This is in the temporal coordinate system of the effect.
       
      This function moves the timeline to the indicated frame and returns. Any side effects of the timeline
      change are also triggered and completed before this returns (for example instance changed actions and renders
      if the output of the effect is being viewed).

      @returns
      - ::kOfxStatOK - the time was changed sucessfully, will all side effects if the change completed
      - ::kOfxStatFailed - the change failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
      - ::kOfxStatErrValue - the time was an illegal value       
  */
  OfxStatus (*gotoTime)(void *instance, double time);

  /** @brief Get the current bounds on a timeline
      
      \arg instance - is the instance of the effect changing the timeline, cast to a void *
      \arg firstTime - is the first time on the timeline. This is in the temporal coordinate system of the effect.
      \arg lastTime - is last time on the timeline. This is in the temporal coordinate system of the effect.
       
      This function

      @returns
      - ::kOfxStatOK - the time enquiry was sucessful
      - ::kOfxStatFailed - the enquiry failed for some host specific reason
      - ::kOfxStatErrBadHandle - the effect handle was invalid
  */
  OfxStatus (*getTimeBounds)(void *instance, double *firstTime, double *lastTime);
} OfxTimeLineSuiteV1;

#endif
