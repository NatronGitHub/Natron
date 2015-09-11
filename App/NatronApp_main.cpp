/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include <csignal>
#include <cstdio>  // perror
#include <cstdlib> // exit
#include <fstream>
#include <sstream>

#include "Global/Macros.h"

#if defined(__NATRON_UNIX__)
#include <sys/signal.h>
#endif

#include <QCoreApplication>

#include "Engine/CLArgs.h"

#include "Gui/GuiApplicationManager.h"

static void setShutDownSignal(int signalId);
static void handleShutDownSignal(int signalId);

int
main(int argc,
     char *argv[])
{
    CLArgs::printBackGroundWelcomeMessage();

    CLArgs args(argc,argv,false);
    if (args.getError() > 0) {
        return 1;
    }

    setShutDownSignal(SIGINT);   // shut down on ctrl-c
    setShutDownSignal(SIGTERM);   // shut down on killall

    if (args.isBackgroundMode()) {
        
        AppManager manager;

        // coverity[tainted_data]
        if (!manager.load(argc,argv,args) ) {
            return 1;
        } else {
            return 0;
        }
    } else {
        
        GuiApplicationManager manager;
        
        // coverity[tainted_data]
        bool loaded = manager.load(argc,argv,args);
        if (!loaded) {
            return 1;
        }

        return manager.exec();
    }
} // main

void
setShutDownSignal(int signalId)
{
#if defined(__NATRON_UNIX__)
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

void
handleShutDownSignal( int /*signalId*/ )
{
    QCoreApplication::exit(0);
}

