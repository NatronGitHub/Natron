/*
Software License :

Copyright (c) 2007-2015, The Open Effects Association Ltd. All rights reserved.

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

#ifndef _ofxProgressSuite_h_
#define _ofxProgressSuite_h_

/** @brief suite for displaying a progress bar */
#define kOfxProgressSuite "OfxProgressSuite"

/** @brief A suite that provides progress feedback from a plugin to an application

    A plugin instance can initiate, update and close a progress indicator with
    this suite.

    This is an optional suite in the Image Effect API.

    API V1.4: Amends the documentation of progress suite V1 so that it is
    expected that it can be raised in a modal manner and have a "cancel"
    button when invoked in instanceChanged. Plugins that perform analysis
    post an appropriate message, raise the progress monitor in a modal manner
    and should poll to see if processing has been aborted. Any cancellation
    should be handled gracefully by the plugin (eg: reset analysis parameters
    to default values), clear allocated memory...

    Many hosts already operate as described above. kOfxStatReplyNo should be
    returned to the plugin during progressUpdate when the user presses
    cancel.

    Suite V2: Adds an ID that can be looked up for internationalisation and
    so on. When a new version is introduced, because plug-ins need to support
    old versions, and plug-in's new releases are not necessary in synch with
    hosts (or users don't immediately update), best practice is to support
    the 2 suite versions. That is, the plugin should check if V2 exists; if
    not then check if V1 exists. This way a graceful transition is
    guaranteed.  So plugin should fetchSuite passing 2,
      (OfxProgressSuiteV2*) fetchSuite(mHost->mHost->host, kOfxProgressSuite,2);
    and if no success pass (OfxProgressSuiteV1*)
      fetchSuite(mHost->mHost->host, kOfxProgressSuite,1);
*/
typedef struct OfxProgressSuiteV1 {

  /** @brief Initiate a progress bar display.

      Call this to initiate the display of a progress bar.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                               associated with. It cannot be NULL.
      \arg \e label          - a text label to display in any message portion of the
                               progress object's user interface. A UTF8 string.

      \pre                   - There is no currently ongoing progress display for this instance.

      \returns
      - ::kOfxStatOK - the handle is now valid for use
      - ::kOfxStatFailed - the progress object failed for some reason
      - ::kOfxStatErrBadHandle - effectInstance was invalid
   */
  OfxStatus (*progressStart)(void *effectInstance,
                             const char *label);

  /** @brief Indicate how much of the processing task has been completed and reports on any abort status.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                                associated with. It cannot be NULL.
      \arg \e progress - a number between 0.0 and 1.0 indicating what proportion of the current task has been processed.

      \returns
      - ::kOfxStatOK - the progress object was successfully updated and the task should continue
      - ::kOfxStatReplyNo - the progress object was successfully updated and the task should abort
      - ::kOfxStatErrBadHandle - the progress handle was invalid,
  */
  OfxStatus (*progressUpdate)(void *effectInstance, double progress);

  /** @brief Signal that we are finished with the progress meter.

      Call this when you are done with the progress meter and no
      longer need it displayed.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                                associated with. It cannot be NULL.

      \post - you can no longer call progressUpdate on the instance

      \returns
      - ::kOfxStatOK - the progress object was successfully closed
      - ::kOfxStatErrBadHandle - the progress handle was invalid,
   */
  OfxStatus (*progressEnd)(void *effectInstance);

} OfxProgressSuiteV1 ;



typedef struct OfxProgressSuiteV2 {

 /** @brief Initiate a progress bar display.

      Call this to initiate the display of a progress bar.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                               associated with. It cannot be NULL.
      \arg \e message        - a text label to display in any message portion of the
                               progress object's user interface. A UTF8 string.
      \arg \e messageId      - plugin-specified id to associate with this message.
                               If overriding the message in an XML resource, the message
			       is identified with this, this may be NULL, or "", in
			       which case no override will occur.
			       New in V2 of this suite.

      \pre                   - There is no currently ongoing progress display for this instance.

      \returns
      - ::kOfxStatOK - the handle is now valid for use
      - ::kOfxStatFailed - the progress object failed for some reason
      - ::kOfxStatErrBadHandle - effectInstance was invalid
 */
OfxStatus (*progressStart)(void *effectInstance,
    const char *message,
    const char *messageid);
  /** @brief Indicate how much of the processing task has been completed and reports on any abort status.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                                associated with. It cannot be NULL.
      \arg \e progress - a number between 0.0 and 1.0 indicating what proportion of the current task has been processed.

      \returns
      - ::kOfxStatOK - the progress object was successfully updated and the task should continue
      - ::kOfxStatReplyNo - the progress object was successfully updated and the task should abort
      - ::kOfxStatErrBadHandle - the progress handle was invalid,
  */
OfxStatus (*progressUpdate)(void *effectInstance, double progress);

  /** @brief Signal that we are finished with the progress meter.

      Call this when you are done with the progress meter and no
      longer need it displayed.

      \arg \e effectInstance - the instance of the plugin this progress bar is
                                associated with. It cannot be NULL.

      \post - you can no longer call progressUpdate on the instance

      \returns
      - ::kOfxStatOK - the progress object was successfully closed
      - ::kOfxStatErrBadHandle - the progress handle was invalid,
   */
OfxStatus (*progressEnd)(void *effectInstance);

} OfxProgressSuiteV2 ;

#endif
