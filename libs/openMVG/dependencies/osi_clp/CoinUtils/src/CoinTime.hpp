/* $Id: CoinTime.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef _CoinTime_hpp
#define _CoinTime_hpp

// Uncomment the next three lines for thorough memory initialisation.
// #ifndef ZEROFAULT
// # define ZEROFAULT
// #endif

//#############################################################################

#include <ctime>
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#else
// MacOS-X and FreeBSD needs sys/time.h
#if defined(__MACH__) || defined (__FreeBSD__)
#include <sys/time.h>
#endif
#if !defined(__MSVCRT__)
#include <sys/resource.h>
#endif
#endif

//#############################################################################

#if defined(_MSC_VER)

#if 0 // change this to 1 if want to use the win32 API
#include <windows.h>
#ifdef small
/* for some unfathomable reason (to me) rpcndr.h (pulled in by windows.h) does a
   '#define small char' */
#undef small
#endif
#define TWO_TO_THE_THIRTYTWO 4294967296.0
#define DELTA_EPOCH_IN_SECS  11644473600.0
inline double CoinGetTimeOfDay()
{
  FILETIME ft;
 
  GetSystemTimeAsFileTime(&ft);
  double t = ft.dwHighDateTime * TWO_TO_THE_THIRTYTWO + ft.dwLowDateTime;
  t = t/10000000.0 - DELTA_EPOCH_IN_SECS;
  return t;
}
#else
#include <sys/types.h>
#include <sys/timeb.h>
inline double CoinGetTimeOfDay()
{
  struct _timeb timebuffer;
#pragma warning(disable:4996)
  _ftime( &timebuffer ); // C4996
#pragma warning(default:4996)
  return timebuffer.time + timebuffer.millitm/1000.0;
}
#endif

#else

#include <sys/time.h>

inline double CoinGetTimeOfDay()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<double>(tv.tv_sec) + static_cast<int>(tv.tv_usec)/1000000.0;
}

#endif // _MSC_VER

/**
   Query the elapsed wallclock time since the first call to this function. If
   a positive argument is passed to the function then the time of the first
   call is set to that value (this kind of argument is allowed only at the
   first call!). If a negative argument is passed to the function then it
   returns the time when it was set.
*/

inline double CoinWallclockTime(double callType = 0)
{
    double callTime = CoinGetTimeOfDay();
    static const double firstCall = callType > 0 ? callType : callTime;
    return callType < 0 ? firstCall : callTime - firstCall;
}

//#############################################################################

//#define HAVE_SDK // if SDK under Win32 is installed, for CPU instead of elapsed time under Win 
#ifdef HAVE_SDK
#include <windows.h>
#ifdef small
/* for some unfathomable reason (to me) rpcndr.h (pulled in by windows.h) does a
   '#define small char' */
#undef small
#endif
#define TWO_TO_THE_THIRTYTWO 4294967296.0
#endif

static inline double CoinCpuTime()
{
  double cpu_temp;
#if defined(_MSC_VER) || defined(__MSVCRT__)
#ifdef HAVE_SDK
  FILETIME creation;
  FILETIME exit;
  FILETIME kernel;
  FILETIME user;
  GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user);
  double t = user.dwHighDateTime * TWO_TO_THE_THIRTYTWO + user.dwLowDateTime;
  return t/10000000.0;
#else
  unsigned int ticksnow;        /* clock_t is same as int */
  ticksnow = (unsigned int)clock();
  cpu_temp = (double)((double)ticksnow/CLOCKS_PER_SEC);
#endif

#else
  struct rusage usage;
# ifdef ZEROFAULT
  usage.ru_utime.tv_sec = 0 ;
  usage.ru_utime.tv_usec = 0 ;
# endif
  getrusage(RUSAGE_SELF,&usage);
  cpu_temp = static_cast<double>(usage.ru_utime.tv_sec);
  cpu_temp += 1.0e-6*(static_cast<double> (usage.ru_utime.tv_usec));
#endif
  return cpu_temp;
}

//#############################################################################



static inline double CoinSysTime()
{
  double sys_temp;
#if defined(_MSC_VER) || defined(__MSVCRT__)
  sys_temp = 0.0;
#else
  struct rusage usage;
# ifdef ZEROFAULT
  usage.ru_utime.tv_sec = 0 ;
  usage.ru_utime.tv_usec = 0 ;
# endif
  getrusage(RUSAGE_SELF,&usage);
  sys_temp = static_cast<double>(usage.ru_stime.tv_sec);
  sys_temp += 1.0e-6*(static_cast<double> (usage.ru_stime.tv_usec));
#endif
  return sys_temp;
}

//#############################################################################
// On most systems SELF seems to include children threads, This is for when it doesn't
static inline double CoinCpuTimeJustChildren()
{
  double cpu_temp;
#if defined(_MSC_VER) || defined(__MSVCRT__)
  cpu_temp = 0.0;
#else
  struct rusage usage;
# ifdef ZEROFAULT
  usage.ru_utime.tv_sec = 0 ;
  usage.ru_utime.tv_usec = 0 ;
# endif
  getrusage(RUSAGE_CHILDREN,&usage);
  cpu_temp = static_cast<double>(usage.ru_utime.tv_sec);
  cpu_temp += 1.0e-6*(static_cast<double> (usage.ru_utime.tv_usec));
#endif
  return cpu_temp;
}
//#############################################################################

#include <fstream>

/**
 This class implements a timer that also implements a tracing functionality.

 The timer stores the start time of the timer, for how much time it was set to
 and when does it expire (start + limit = end). Queries can be made that tell
 whether the timer is expired, is past an absolute time, is past a percentage
 of the length of the timer. All times are given in seconds, but as double
 numbers, so there can be fractional values.

 The timer can also be initialized with a stream and a specification whether
 to write to or read from the stream. In the former case the result of every
 query is written into the stream, in the latter case timing is not tested at
 all, rather the supposed result is read out from the stream. This makes it
 possible to exactly retrace time sensitive program execution.
*/
class CoinTimer
{
private:
   /// When the timer was initialized/reset/restarted
   double start;
   /// 
   double limit;
   double end;
#ifdef COIN_COMPILE_WITH_TRACING
   std::fstream* stream;
   bool write_stream;
#endif

private:
#ifdef COIN_COMPILE_WITH_TRACING
   inline bool evaluate(bool b_tmp) const {
      int i_tmp = b_tmp;
      if (stream) {
	 if (write_stream)
	    (*stream) << i_tmp << "\n";
	 else 
	    (*stream) >> i_tmp;
      }
      return i_tmp;
   }
   inline double evaluate(double d_tmp) const {
      if (stream) {
	 if (write_stream)
	    (*stream) << d_tmp << "\n";
	 else 
	    (*stream) >> d_tmp;
      }
      return d_tmp;
   }
#else
   inline bool evaluate(const bool b_tmp) const {
      return b_tmp;
   }
   inline double evaluate(const double d_tmp) const {
      return d_tmp;
   }
#endif   

public:
   /// Default constructor creates a timer with no time limit and no tracing
   CoinTimer() :
      start(0), limit(1e100), end(1e100)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}

   /// Create a timer with the given time limit and with no tracing
   CoinTimer(double lim) :
      start(CoinCpuTime()), limit(lim), end(start+lim)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}

#ifdef COIN_COMPILE_WITH_TRACING
   /** Create a timer with no time limit and with writing/reading the trace
       to/from the given stream, depending on the argument \c write. */
   CoinTimer(std::fstream* s, bool write) :
      start(0), limit(1e100), end(1e100),
      stream(s), write_stream(write) {}
   
   /** Create a timer with the given time limit and with writing/reading the
       trace to/from the given stream, depending on the argument \c write. */
   CoinTimer(double lim, std::fstream* s, bool w) :
      start(CoinCpuTime()), limit(lim), end(start+lim),
      stream(s), write_stream(w) {}
#endif
   
   /// Restart the timer (keeping the same time limit)
   inline void restart() { start=CoinCpuTime(); end=start+limit; }
   /// An alternate name for \c restart()
   inline void reset() { restart(); }
   /// Reset (and restart) the timer and change its time limit
   inline void reset(double lim) { limit=lim; restart(); }

   /** Return whether the given percentage of the time limit has elapsed since
       the timer was started */
   inline bool isPastPercent(double pct) const {
      return evaluate(start + limit * pct < CoinCpuTime());
   }
   /** Return whether the given amount of time has elapsed since the timer was
       started */
   inline bool isPast(double lim) const {
      return evaluate(start + lim < CoinCpuTime());
   }
   /** Return whether the originally specified time limit has passed since the
       timer was started */
   inline bool isExpired() const {
      return evaluate(end < CoinCpuTime());
   }

   /** Return how much time is left on the timer */
   inline double timeLeft() const {
      return evaluate(end - CoinCpuTime());
   }

   /** Return how much time has elapsed */
   inline double timeElapsed() const {
      return evaluate(CoinCpuTime() - start);
   }

   inline void setLimit(double l) {
      limit = l;
      return;
   }
};

#endif
