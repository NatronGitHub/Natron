/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Timer.h"

#include <iostream>
#include <time.h>
#include <cmath>
#include <cassert>
#include <string>
#include <sstream>
#include <stdexcept>
#include <set>

#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QMutexLocker>

#include "Global/GlobalDefines.h"
#if defined(__NATRON_UNIX__)
#include <execinfo.h>
#include <cxxabi.h>
#endif

#define NATRON_FPS_REFRESH_RATE_SECONDS 1.5

NATRON_NAMESPACE_ENTER

#if defined(__NATRON_WIN32__) && !defined(__NATRON_MINGW__)
int
gettimeofday (struct timeval *tv,
              void *tz)
{
    union
    {
        ULONGLONG ns100;  // time since 1 Jan 1601 in 100ns units
        FILETIME ft;
    }

    now;

    GetSystemTimeAsFileTime (&now.ft);
    tv->tv_usec = long ( (now.ns100 / 10LL) % 1000000LL );
    tv->tv_sec = long ( (now.ns100 - 116444736000000000LL) / 10000000LL );

    return 0;
}

#endif

TimestampVal getTimestampInSeconds() {
#ifdef HAVE_CXX11_CHRONO
    return std::chrono::high_resolution_clock::now();
#else // !HAVE_CXX11_CHRONO
#ifdef _WIN32
    LARGE_INTEGER li_start;
    QueryPerformanceCounter(&li_start);
    return (double)(li_start.QuadPart);
#else // !_WIN32
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6;
#endif  // _WIN32
#endif // HAVE_CXX11_CHRONO
}

double getPerformanceFrequency() {
#ifdef _WIN32
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq)) {
        // From https://msdn.microsoft.com/en-us/library/ms886789.aspx
        // If the hardware does not support a high frequency counter, QueryPerformanceFrequency will return 1000 because the API defaults to a milliseconds GetTickCount implementation.
        return 1000;
    }
    return (double)freq.QuadPart;
#else
    return 1.;
#endif
}

double getTimeElapsed(const TimestampVal& start, const TimestampVal& end, double frequency)
{
#ifdef HAVE_CXX11_CHRONO
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.;
#else
    return (end - start) / frequency;
#endif
}

static double getTimeElapsedClamped(const TimestampVal& start, const TimestampVal& end, double frequency = 1.)
{
    return std::min(0., getTimeElapsed(start, end, frequency));
}

// prints time value as seconds, minutes hours or days
QString
Timer::printAsTime(const double timeInSeconds,
                   const bool clampToSecondsToInt)
{
    const int min = 60;
    const int hour = 60 * min;
    const int day = 24 * hour;
    QString ret;
    double timeRemain = timeInSeconds;

    if (timeRemain >= day) {
        double daysRemaining = timeRemain / day;
        double floorDays = std::floor(daysRemaining);
        if (floorDays > 0) {
            ret.append( ( floorDays > 1 ? tr("%1 days") : tr("%1 day") ).arg( QString::number(floorDays) ) );
            timeRemain -= floorDays * day;
        }
        if (timeInSeconds >= 3 * day) {
            return ret;
        }
    }
    if (timeRemain >= hour) {
        if (timeInSeconds >= day) {
            ret.append( QLatin1Char(' ') );
        }
        double hourRemaining = timeRemain / hour;
        double floorHour = std::floor(hourRemaining);
        if (floorHour > 0) {
            ret.append( ( (floorHour > 1) ? tr("%1 hours") : tr("%1 hour") ).arg( QString::number(floorHour) ) );
            timeRemain -= floorHour * hour;
        }
        if (timeInSeconds >= 3 * hour) {
            return ret;
        }
    }
    if (timeRemain >= min) {
        if (timeInSeconds >= hour) {
            ret.append( QLatin1Char(' ') );
        }
        double minRemaining = timeRemain / min;
        double floorMin = std::floor(minRemaining);
        if (floorMin > 0) {
            ret.append( ( (floorMin > 1) ? tr("%1 minutes") : tr("%1 minute") ).arg( QString::number(floorMin) ) );
            timeRemain -= floorMin * min;
        }
        if (timeInSeconds >= 3 * min) {
            return ret;
        }
    }
    if (timeInSeconds >= min) {
        ret.append( QLatin1Char(' ') );
    }
    if (clampToSecondsToInt) {
        ret.append( ( timeRemain > 1 ? tr("%1 seconds") : tr("%1 second") ).arg( QString::number( (int)timeRemain ) ) );
    } else {
        ret.append( ( timeRemain > 1 ? tr("%1 seconds") : tr("%1 second") ).arg( QString::number(timeRemain, 'f', 2) ) );
    }

    return ret;
} // Timer::printAsTime

Timer::Timer ()
    : playState (ePlayStateRunning)
    , _frequency(getPerformanceFrequency())
    , _spf (1 / 24.0)
    , _timingError (0)
    , _framesSinceLastFpsFrame (0)
    , _actualFrameRate (0)
    , _mutex()
{

    _lastFrameTime = getTimestampInSeconds();
    _lastFpsFrameTime = _lastFrameTime;
}

Timer::~Timer()
{
}

void
Timer::waitUntilNextFrameIsDue ()
{
    if (playState != ePlayStateRunning) {
        //
        // If we are not running, reset all timing state
        // variables and return without waiting.
        //

        _lastFrameTime = getTimestampInSeconds();
        _timingError = 0;
        _lastFpsFrameTime = _lastFrameTime;
        _framesSinceLastFpsFrame = 0;

        return;
    }


    double spf;
    {
        QMutexLocker l(&_mutex);
        spf = _spf;
    }
    //
    // If less than _spf seconds have passed since the last frame
    // was displayed, sleep until exactly _spf seconds have gone by.
    //
    TimestampVal now = getTimestampInSeconds();

    double timeSinceLastFrame = getTimeElapsedClamped(_lastFrameTime, now, _frequency);
    double timeToSleep = spf - timeSinceLastFrame - _timingError;

#ifdef _WIN32

    if (timeToSleep > 0) {
        Sleep ( int (timeToSleep * 1000.0f) );
    }

#else

    if (timeToSleep > 0) {
        timespec ts;
        ts.tv_sec = (time_t) timeToSleep;
        ts.tv_nsec = (long) ( (timeToSleep - ts.tv_sec) * 1e9f );
        nanosleep (&ts, 0);
    }

#endif

    //
    // If we slept, it is possible that we woke up a little too early
    // or a little too late.  Keep track of the difference between
    // now and the exact time when we wanted to wake up; next time
    // we'll try sleep that much longer or shorter.  This should
    // keep our average frame rate close to one fame every _spf seconds.
    //

    now = getTimestampInSeconds();

    timeSinceLastFrame = getTimeElapsed(_lastFrameTime, now, _frequency);


    _timingError += timeSinceLastFrame - spf;

    if (_timingError < -2 * spf) {
        _timingError = -2 * spf;
    }

    if (_timingError >  2 * spf) {
        _timingError =  2 * spf;
    }

    _lastFrameTime = now;

    //
    // Calculate our actual frame rate, averaged over several frames.
    //
    double t = getTimeElapsed(_lastFpsFrameTime, now, _frequency);


    if (t > NATRON_FPS_REFRESH_RATE_SECONDS) {
        double actualFrameRate = _framesSinceLastFpsFrame / t;
        double curActualFrameRate;
        double desiredFrameRate;
        {
            QMutexLocker l(&_mutex);
            if (actualFrameRate != _actualFrameRate) {
                _actualFrameRate = actualFrameRate;
            }
            desiredFrameRate = 1.f / _spf;
            curActualFrameRate = _actualFrameRate;
        }
        Q_EMIT fpsChanged(curActualFrameRate, desiredFrameRate);

        _framesSinceLastFpsFrame = 0;
    }


    if (_framesSinceLastFpsFrame == 0) {
        _lastFpsFrameTime = now;
    }

    _framesSinceLastFpsFrame += 1;
} // waitUntilNextFrameIsDue

double
Timer::getActualFrameRate() const
{
    QMutexLocker l(&_mutex);

    return _actualFrameRate;
}

void
Timer::setDesiredFrameRate (double fps)
{
    QMutexLocker l(&_mutex);

    _spf = 1 / fps;
}

double
Timer::getDesiredFrameRate() const
{
    QMutexLocker l(&_mutex);

    return 1.f / _spf;
}

TimeLapse::TimeLapse()
{
    prev = getTimestampInSeconds();
    constructorTime = prev;
    frequency = getPerformanceFrequency();
}

TimeLapse::~TimeLapse()
{
}

double
TimeLapse::getTimeElapsedReset()
{
    TimestampVal now = getTimestampInSeconds();

    double dt = getTimeElapsed(prev, now, frequency);

    prev = now;

    return dt;
}

void
TimeLapse::reset()
{
    prev = getTimestampInSeconds();
}

double
TimeLapse::getTimeSinceCreation() const
{
    TimestampVal now = getTimestampInSeconds();

    double dt = getTimeElapsed(constructorTime, now, frequency);


    return dt;
}

TimeLapseReporter::TimeLapseReporter(const std::string& message)
    : message(message)
{
    prev = getTimestampInSeconds();
    frequency = getPerformanceFrequency();
}

TimeLapseReporter::~TimeLapseReporter()
{
    TimestampVal now = getTimestampInSeconds();

    double dt = getTimeElapsed(prev, now, frequency);

    std::cout << message << ' ' << dt << std::endl;
}


struct StackTraceRecorderPrivate
{
    int maxDepth;

    std::vector<StackTraceRecorder::StackFrame> stack;

    StackTraceRecorderPrivate(int maxDepth)
    : maxDepth(maxDepth)
    , stack()
    {
        
    }
};

StackTraceRecorder::StackTraceRecorder(int maxDepth)
: _imp(new StackTraceRecorderPrivate(maxDepth))
{
#ifdef __NATRON_UNIX__
    std::vector<void*> trace(_imp->maxDepth);
    int traceSize = backtrace(&trace[0], trace.size());
    char **messages = backtrace_symbols(&trace[0], traceSize);
    _imp->stack.resize(traceSize);

    for (std::size_t i = 0; i < _imp->stack.size(); ++i) {

        StackTraceRecorder::StackFrame& frame = _imp->stack[i];
        /*

         Typically this is how the backtrace looks like:

         0   <app/lib-name>     0x0000000100000e98 _Z5tracev + 72
         1   <app/lib-name>     0x00000001000015c1 _ZNK7functorclEv + 17
         2   <app/lib-name>     0x0000000100000f71 _Z3fn0v + 17
         3   <app/lib-name>     0x0000000100000f89 _Z3fn1v + 9
         4   <app/lib-name>     0x0000000100000f99 _Z3fn2v + 9
         5   <app/lib-name>     0x0000000100000fa9 _Z3fn3v + 9
         6   <app/lib-name>     0x0000000100000fb9 _Z3fn4v + 9
         7   <app/lib-name>     0x0000000100000fc9 _Z3fn5v + 9
         8   <app/lib-name>     0x0000000100000fd9 _Z3fn6v + 9
         9   <app/lib-name>     0x0000000100001018 main + 56
         10  libdyld.dylib      0x00007fff91b647e1 start + 0

         */

        // split the string, take out chunks out of stack trace
        std::stringstream ss(messages[i]);
        ss >> frame.moduleName;
        ss >> frame.addr;
        ss >> frame.functionSymbol;
        ss >> frame.offset;

        int   validCppName = 0;
        //  if this is a C++ library, symbol will be demangled
        //  on success function sets validCppName to 0
        char* functionName = abi::__cxa_demangle(frame.functionSymbol.c_str(), NULL, 0, &validCppName);

        std::string demangledFunctionName;
        if (validCppName == 0 && functionName) {
            demangledFunctionName.append(functionName);
            free(functionName);
        } else {
            demangledFunctionName = frame.functionSymbol;
        }
        std::stringstream encodeSs;
        encodeSs << "(" << frame.moduleName << ")0x" << frame.addr << " - " << demangledFunctionName  << " + " << frame.offset;
        frame.encoded = encodeSs.str();
    }
    free(messages);
#elif defined(__NATRON_WINDOWS__)
// Todo
#endif // __NATRON_UNIX__
}

#ifdef __NATRON_UNIX__
std::vector<StackTraceRecorder::StackFrame>
StackTraceRecorder::getStackTrace(int maxDepth)
{
    StackTraceRecorder r(maxDepth);
    return r._imp->stack;
}
#endif

StackTraceRecorder::~StackTraceRecorder()
{

}

// A block recorded in a start() stop() for a given thread
struct ProfilerDataBlock
{
    // All times are in seconds
    TimestampVal startTimeStamp;
    std::string functionName;
    std::string threadName;

    ProfilerDataBlock()
    : startTimeStamp()
    , functionName()
    , threadName()
    {

    }
};

struct ProfilerDataBlockStat
{
    // All times are in seconds
    std::string functionName;
    double  totalTime;
    double  averageTime;
    double  minTime;
    double  maxTime;
    unsigned long long nbCalls;

    ProfilerDataBlockStat()
    : functionName()
    , totalTime(0)
    , averageTime(0)
    , minTime(INT_MAX)
    , maxTime(INT_MIN)
    , nbCalls(0)
    {

    }

};

struct PerThreadStackData
{
    std::vector<ProfilerDataBlock> startStopStack;
};

typedef boost::shared_ptr<PerThreadStackData> PerThreadStackDataPtr;

typedef std::map<std::string, PerThreadStackDataPtr> PerThreadStackDataMap;


typedef std::map<std::string, ProfilerDataBlockStat> ProfilerDataBlockStatMap;

struct ProfilerPrivate
{

    ProfilerDataBlockStatMap perFunctionStats;

    // Hold profiler data vector in function of the thread
    PerThreadStackDataMap perThreadCallStack;

    QMutex criticalSectionMutex;

    TimeLapse timer;

    double frequency;

    ProfilerPrivate()
    : perFunctionStats()
    , perThreadCallStack()
    , criticalSectionMutex()
    , timer()
    , frequency(getPerformanceFrequency())
    {
        
    }
};



Profiler::Profiler()
: _imp(new ProfilerPrivate())
{

}

Profiler::~Profiler()
{

}

static std::string
getThreadName(QThread* thread)
{
    std::stringstream ss;
    ss << thread->objectName().toStdString() << " (" << thread << ")";
    return ss.str();

}

void
Profiler::start(const std::string& functionName)
{

    QThread* curThread = QThread::currentThread();

    // Add the profile name in the callstack vector
    ProfilerDataBlock data;
    data.startTimeStamp = getTimestampInSeconds();

    // Get the thread name, map against thread name and not thread pointer because at dump time the thread might no longer exist.
    data.threadName = getThreadName(curThread);

    PerThreadStackDataPtr threadData;
    {
        QMutexLocker k(&_imp->criticalSectionMutex);
        PerThreadStackDataPtr& thisThreadStack = _imp->perThreadCallStack[data.threadName];
        if (!thisThreadStack) {
            thisThreadStack.reset(new PerThreadStackData);
        }
        threadData = thisThreadStack;
    }

    /*
    // Record the current stack trace
    std::vector<StackTraceRecorder::StackFrame> stack = StackTraceRecorder::getStackTrace();
    if (!stack.empty()) {
        StackTraceRecorder::StackFrame& frame = stack.front();
        data.functionName = frame.encoded;
    }*/
    data.functionName = functionName;
    threadData->startStopStack.push_back(data);
} // start

void
Profiler::stop()
{
    QThread* curThread = QThread::currentThread();

    std::string threadName = getThreadName(curThread);


    PerThreadStackDataPtr threadData;
    {
        // Retrieve the right entry in function of the threadId
        QMutexLocker k(&_imp->criticalSectionMutex);
        PerThreadStackDataMap::iterator foundCallStack = _imp->perThreadCallStack.find(threadName);
        if (foundCallStack == _imp->perThreadCallStack.end()) {
            // You should call stop for each corresponding call to start
            assert(false);
            return;
        }
        assert(foundCallStack->second);
        threadData = foundCallStack->second;
    }
    assert(!threadData->startStopStack.empty());
    if (threadData->startStopStack.empty()) {
        return;
    }
    // Retrieve the last block that was started
    ProfilerDataBlock& curBlock = threadData->startStopStack.back();

    // Compute elapsed time
    double elapsedTime;
    {
        TimestampVal now = getTimestampInSeconds();
        elapsedTime = getTimeElapsed(curBlock.startTimeStamp, now, _imp->frequency);
    }
    {
        QMutexLocker k(&_imp->criticalSectionMutex);
        ProfilerDataBlockStat &stats = _imp->perFunctionStats[curBlock.functionName];
        stats.functionName = curBlock.functionName;
        stats.totalTime += elapsedTime;
        // Retrieve time information to compute min and max time
        stats.minTime = std::min(stats.minTime, elapsedTime);
        stats.maxTime = std::max(stats.maxTime, elapsedTime);
        ++stats.nbCalls;
        stats.averageTime = stats.totalTime / stats.nbCalls;
    }


    threadData->startStopStack.pop_back();

} // stop



std::string
Profiler::dumpLog() const
{

    std::stringstream finalStream;

    finalStream << std::endl << std::endl;
    finalStream << "Profile dump:" << std::endl;
    finalStream << "_______________________________________________________________________________________" << std::endl;
    finalStream << "| Total time   | Avg Time     |  Min time    |  Max time    | Calls  | Section" << std::endl;
    finalStream << "_______________________________________________________________________________________" << std::endl;

    for (ProfilerDataBlockStatMap::const_iterator it = _imp->perFunctionStats.begin(); it != _imp->perFunctionStats.end(); ++it) {
        finalStream << "  " <<
        QString::number(it->second.totalTime * 1000, 'f', 6).toStdString() << "\t\t" <<
        QString::number(it->second.averageTime * 1000, 'f', 6).toStdString() << "\t\t" <<
        QString::number(it->second.minTime * 1000, 'f', 6).toStdString() << "\t\t" <<
        QString::number(it->second.maxTime * 1000, 'f', 6).toStdString() << "\t\t" <<
        QString::number(it->second.nbCalls).toStdString() << "\t\t" <<
        it->second.functionName << std::endl;
        finalStream << "_______________________________________________________________________________________" <<  std::endl;
    }
    finalStream << std::endl << std::endl;

    return finalStream.str();
} // dumpLog


NATRON_NAMESPACE_EXIT


NATRON_NAMESPACE_USING
#include "moc_Timer.cpp"
