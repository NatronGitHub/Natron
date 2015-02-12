//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <csignal>
#include <cstdio>  // perror
#include <cstdlib> // exit

#if defined(Q_OS_UNIX)
#include <sys/signal.h>
#endif

#include <QCoreApplication>

#include "Engine/AppManager.h"

static void setShutDownSignal(int signalId);
static void handleShutDownSignal(int signalId);

int
main(int argc,
     char *argv[])
{
    CLArgs args(argc,argv,true);
    if (args.getError() > 0) {
        return 1;
    }
    
    setShutDownSignal(SIGINT);   // shut down on ctrl-c
    setShutDownSignal(SIGTERM);   // shut down on killall

    AppManager manager;

    // coverity[tainted_data]
    if (!manager.load(argc,argv,args)) {
        return 1;
    } else {
        return 0;
    }

    return 0;
} //main

static void
setShutDownSignal(int signalId)
{
#if defined(Q_OS_UNIX)
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1) {
        std::perror("setting up termination signal");
        std::exit(1);
    }
#else
    std::signal(signalId, handleShutDownSignal);
#endif
}

static void
handleShutDownSignal( int /*signalId*/ )
{
    QCoreApplication::exit(0);
}

