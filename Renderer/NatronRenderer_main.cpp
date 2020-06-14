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

#include <QtCore/QtGlobal> // for Q_OS_*
#if defined(Q_OS_UNIX)
#include <sys/time.h>     // for getrlimit on linux
#include <sys/resource.h> // for getrlimit
#if defined(__APPLE__)
#include <sys/syslimits.h> // OPEN_MAX
#endif
#endif

#include <cstdio>  // perror
#include <cstdlib> // exit
#include <iostream>

#include <QtCore/QCoreApplication>

#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/AppManager.h"
#include "Engine/CLArgs.h"

NATRON_NAMESPACE_USING


#ifdef Q_OS_WIN
// g++ knows nothing about wmain
// https://sourceforge.net/p/mingw-w64/wiki2/Unicode%20apps/
extern "C" {
    int wmain(int argc, wchar_t** argv)
#else
    int main(int argc, char *argv[])
#endif
{
#ifdef DEBUG
    boost_adaptbx::floating_point::exception_trapping trap(boost_adaptbx::floating_point::exception_trapping::division_by_zero |
                                                           boost_adaptbx::floating_point::exception_trapping::invalid |
                                                           boost_adaptbx::floating_point::exception_trapping::overflow);
    assert(boost_adaptbx::floating_point::is_invalid_trapped());
#endif
#if defined(Q_OS_UNIX) && defined(RLIMIT_NOFILE)
    /*
     Avoid 'Too many open files' on Unix.

     Increase the number of file descriptors that the process can open to the maximum allowed.
     - By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
     - On Linux, the default limit is usually 1024.

     Note that due to a bug in stdio on OS X, the limit on the number of files opened using fopen()
     cannot be changed after the first call to stdio (e.g. printf() or fopen()).
     Consequently, this has to be the first thing to do in main().
     */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        if (rl.rlim_max > rl.rlim_cur) {
            rl.rlim_cur = rl.rlim_max;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
#             if defined(__APPLE__) && defined(OPEN_MAX)
                // On Mac OS X, setrlimit(RLIMIT_NOFILE, &rl) fails to set
                // rlim_cur above OPEN_MAX even if rlim_max > OPEN_MAX.
                if (rl.rlim_cur > OPEN_MAX) {
                    rl.rlim_cur = OPEN_MAX;
                    setrlimit(RLIMIT_NOFILE, &rl);
                }
#             endif
            }
        }
    }
#endif

    CLArgs args(argc, argv, true);

    if (args.getError() > 0) {
        return 1;
    }

    AppManager manager;

    // coverity[tainted_data]
#ifdef Q_OS_WIN
        if ( !manager.loadW(argc, argv, args) ) {
#else
        if ( !manager.load(argc, argv, args) ) {
#endif
        return 1;
    } else {
        return 0;
    }

    return 0;
} //main
#ifdef Q_OS_WIN
} // extern "C"
#endif
