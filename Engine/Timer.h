/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_TIMER_H
#define NATRON_ENGINE_TIMER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

//----------------------------------------------------------------------------
//
//	Timing control
//
//----------------------------------------------------------------------------

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
#include <windows.h>
#else
#include <sys/time.h>
#endif


#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
int gettimeofday (struct timeval *tv, void *tz);
#endif

enum PlayStateEnum
{
    ePlayStateRunning,
    ePlayStatePause,
};

class Timer
    : public QObject
{
    Q_OBJECT

public:

    //------------
    // Constructor/Destructor
    //------------

    Timer ();

    ~Timer();

    //--------------------------------------------------------
    // Timing control to maintain the desired frame rate:
    // the redrawWindow() function in the display thread calls
    // waitUntilNextFrameIsDue() before displaying each frame.
    //
    // If playState == ePlayStateRunning, then waitUntilNextFrameIsDue()
    // sleeps until the appropriate amount of time has elapsed
    // since the last call to waitUntilNextFrameIsDue().
    // If playState != ePlayStateRunning, then waitUntilNextFrameIsDue()
    // returns immediately.
    //--------------------------------------------------------

    void    waitUntilNextFrameIsDue ();


    //-------------------------------------------------
    // Set and get the frame rate, in frames per second
    //-------------------------------------------------

    void  setDesiredFrameRate (double fps);
    double getDesiredFrameRate() const;

    double getActualFrameRate() const;

    //-------------------
    // Current play state
    //-------------------

    PlayStateEnum playState;
    static QString printAsTime(double timeInSeconds, bool clampToSecondsToInt);

Q_SIGNALS:

    void fpsChanged(double actualfps, double desiredfps);

private:

    double _spf;                 // desired frame rate,
    // in seconds per frame
    timeval _lastFrameTime;         // time when we displayed the
    // last frame
    double _timingError;             // cumulative timing error
    timeval _lastFpsFrameTime;      // state to keep track of the
    int _framesSinceLastFpsFrame;       // actual frame rate, averaged
    double _actualFrameRate;         // over several frames
    mutable QMutex _mutex; //< protects _spf and _actualFrameRate
};


class TimeLapse
{
    timeval prev;
    timeval constructorTime;

public:

    TimeLapse();

    /**
     * @brief Returns the time elapsed in seconds since getTimeElapsedReset was last called. If getTimeElapsedReset has never been called
     * this will return the time since the object was instantiated.
     **/
    double getTimeElapsedReset();


    void reset();

    /**
     * @brief Returns the time elapsed since this object was created.
     **/
    double getTimeSinceCreation() const;

    ~TimeLapse();
};

/**
 * @class A small objects that will print the time elapsed (in seconds) between the constructor and the destructor.
 **/
class TimeLapseReporter
{
    timeval prev;
    std::string message;

public:

    TimeLapseReporter(const std::string& message);

    ~TimeLapseReporter();
};

NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_ENGINE_TIMER_H
