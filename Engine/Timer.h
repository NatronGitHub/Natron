/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#endif

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
#include <windows.h>
#else
#include <sys/time.h>
#endif
#ifdef HAVE_CXX11_CHRONO
#include <chrono>
#endif

#include <QtCore/QString>
#include <QtCore/QObject>
#include <QtCore/QMutex>

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

//----------------------------------------------------------------------------
//
//	Timing control
//
//----------------------------------------------------------------------------

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
int gettimeofday (struct timeval *tv, void *tz);
#endif

enum PlayStateEnum
{
    ePlayStateRunning,
    ePlayStatePause,
};



#ifdef HAVE_CXX11_CHRONO
typedef std::chrono::high_resolution_clock::time_point TimestampVal;
#else
typedef double TimestampVal;
#endif // HAVE_CXX11_CHRONO


/**
 * @brief Return a timestamp. It can be compared with another timestamp returned by this function
 **/
TimestampVal getTimestampInSeconds();

/**
 * @brief On windows return the frequency of the values returned by getTimestampInSeconds().
 * On unix the timestamp are always in seconds.
 **/
double getPerformanceFrequency();

/**
 * @brief Returns the number of seconds elapsed between start to end.
 **/
double getTimeElapsed(const TimestampVal& start, const TimestampVal& end, double frequency = 1.);

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

    double _frequency;
    double _spf;                 // desired frame rate,
    // in seconds per frame
    TimestampVal _lastFrameTime;         // time when we displayed the
    // last frame
    double _timingError;             // cumulative timing error
    TimestampVal _lastFpsFrameTime;      // state to keep track of the
    int _framesSinceLastFpsFrame;       // actual frame rate, averaged
    double _actualFrameRate;         // over several frames
    mutable QMutex _mutex; //< protects _spf and _actualFrameRate
};



class TimeLapse
{
    
    TimestampVal prev;
    TimestampVal constructorTime;

    double frequency;

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
    TimestampVal prev;

    double frequency;

    std::string message;

public:

    TimeLapseReporter(const std::string& message);

    ~TimeLapseReporter();
};


struct StackTraceRecorderPrivate;


class StackTraceRecorder
{

    StackTraceRecorder(int maxDepth);

public:

    struct StackFrame
    {
        std::string functionSymbol;
        std::string moduleName;
        std::string offset;
        std::string addr;
        std::string encoded;

    };

    // For now only works on Linux and mac
#ifdef __NATRON_UNIX__
    static std::vector<StackTraceRecorder::StackFrame> getStackTrace(int maxDepth = 16);
#endif

    ~StackTraceRecorder();
    
private:

    boost::scoped_ptr<StackTraceRecorderPrivate> _imp;
};

struct ProfilerPrivate;
class Profiler
{
public:

    Profiler();

    ~Profiler();

    void start(const std::string& functionName);
    void stop();
    std::string dumpLog() const;

private:

    boost::scoped_ptr<ProfilerPrivate> _imp;

    
};


NATRON_NAMESPACE_EXIT

#endif // ifndef NATRON_ENGINE_TIMER_H
