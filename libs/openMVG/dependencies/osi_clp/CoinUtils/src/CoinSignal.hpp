/* $Id: CoinSignal.hpp 1372 2011-01-03 23:31:00Z lou $ */
// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef _CoinSignal_hpp
#define _CoinSignal_hpp

// This file is fully docified.
// There's nothing to docify...

//#############################################################################

#include <csignal>

//#############################################################################

#if defined(_MSC_VER)
   typedef void (__cdecl *CoinSighandler_t) (int);
#  define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if (defined(__GNUC__) && defined(__linux__))
  typedef sighandler_t CoinSighandler_t;
# define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if defined(__CYGWIN__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if defined(__MINGW32__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if defined(__FreeBSD__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if defined(__NetBSD__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined
#endif

//-----------------------------------------------------------------------------

#if defined(_AIX)
#  if defined(__GNUC__)
      typedef typeof(SIG_DFL) CoinSighandler_t;
#     define CoinSighandler_t_defined
#  endif
#endif

//-----------------------------------------------------------------------------

#if defined (__hpux)
#  define CoinSighandler_t_defined
#  if defined(__GNUC__)
      typedef typeof(SIG_DFL) CoinSighandler_t;
#  else
      extern "C" {
         typedef void (*CoinSighandler_t) (int);
      }
#  endif
#endif

//-----------------------------------------------------------------------------

#if defined(__sun)
#  if defined(__SUNPRO_CC)
#     include <signal.h>
      extern "C" {
         typedef void (*CoinSighandler_t) (int);
      }
#     define CoinSighandler_t_defined
#  endif
#  if defined(__GNUC__)
      typedef typeof(SIG_DFL) CoinSighandler_t;
#     define CoinSighandler_t_defined
#  endif
#endif

//-----------------------------------------------------------------------------

#if defined(__MACH__) && defined(__GNUC__)
#if defined(__clang__)
   typedef void(*CoinSighandler_t)(int);
#define CoinSighandler_t_defined
#elif defined(__MACH__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
# define CoinSighandler_t_defined
#endif
#endif

//#############################################################################

#ifndef CoinSighandler_t_defined
#  warning("OS and/or compiler is not recognized. Defaulting to:");
#  warning("extern "C" {")
#  warning("   typedef void (*CoinSighandler_t) (int);")
#  warning("}")
   extern "C" {
      typedef void (*CoinSighandler_t) (int);
   }
#endif

//#############################################################################

#endif
