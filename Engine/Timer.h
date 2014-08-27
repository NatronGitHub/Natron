
///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006, Industrial Light & Magic, a division of Lucas
// Digital Ltd. LLC
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Industrial Light & Magic nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

#ifndef NATRON_ENGINE_TIMER_H_
#define NATRON_ENGINE_TIMER_H_

//----------------------------------------------------------------------------
//
//	Timing control
//
//----------------------------------------------------------------------------

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#ifdef _WIN32
int gettimeofday (struct timeval *tv, void *tz);
#endif

enum PlayState
{
    PREPARE_TO_RUN,
    RUNNING,
    PREPARE_TO_PAUSE,
    PAUSE,
};


class Timer
{
public:

    //------------
    // Constructor
    //------------

    Timer ();


    //--------------------------------------------------------
    // Timing control to maintain the desired frame rate:
    // the redrawWindow() function in the display thread calls
    // waitUntilNextFrameIsDue() before displaying each frame.
    //
    // If playState == RUNNING, then waitUntilNextFrameIsDue()
    // sleeps until the apropriate amount of time has elapsed
    // since the last call to waitUntilNextFrameIsDue().
    // If playState != RUNNING, then waitUntilNextFrameIsDue()
    // returns immediately.
    //--------------------------------------------------------

    void    waitUntilNextFrameIsDue ();


    //-------------------------------------------------
    // Set and get the frame rate, in frames per second
    //-------------------------------------------------

    void    setDesiredFrameRate (float fps);
    float   getDesiredFrameRate() const;

    float   actualFrameRate ();


    //-------------------
    // Current play state
    //-------------------

    PlayState playState;

private:

    float _spf;                 // desired frame rate,
    // in seconds per frame
    timeval _lastFrameTime;         // time when we displayed the
    // last frame
    float _timingError;             // cumulative timing error
    timeval _lastFpsFrameTime;      // state to keep track of the
    int _framesSinceLastFpsFrame;       // actual frame rate, averaged
    float _actualFrameRate;         // over several frames
};

#endif // ifndef NATRON_ENGINE_TIMER_H_
