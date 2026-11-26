// Copyright (c) 2007, 2008, 2009 libmv authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef LIBMV_LOGGING_LOGGING_H
#define LIBMV_LOGGING_LOGGING_H

// glog/logging.h is Ceres' miniglog with the bundled Ceres (libs/ceres), or
// the glog of an installed Ceres (NATRON_SYSTEM_LIBS).
#if defined(_WIN32) && defined(ERROR)
// wingdi.h (from windows.h) defines ERROR as 0, and miniglog declares a global
// constant named ERROR: hide the macro while miniglog is read.
#pragma push_macro("ERROR")
#undef ERROR
#include <glog/logging.h>
#pragma pop_macro("ERROR")
#else
#include <glog/logging.h>
#endif

#ifdef DEBUG
//#define TRACE_LIB_MV 1
#endif

#ifdef TRACE_LIB_MV
#define LIBMV_LOG(severity) DLOG(severity)
#else
// Compiled out: LOG_IF exists in glog and in Ceres' miniglog (which does not
// have google::LogMessageVoidify).
#define LIBMV_LOG(severity) LOG_IF(severity, false)
#endif

// miniglog defines its own LG.
#undef LG
#define LG LIBMV_LOG(INFO)

// V0, V1 and V2 used to be defined here as aliases for LG. Nothing in libmv or
// in Natron ever used them, and Qt6 names a template parameter V2 in
// qnumeric.h, so any translation unit including this header before QtCore
// failed to compile.

#endif  // LIBMV_LOGGING_LOGGING_H
