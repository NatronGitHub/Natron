//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include <csignal>
#include <cstdio>

#if defined(Q_OS_UNIX)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <QApplication>

#include "Gui/GuiApplicationManager.h"

void setShutDownSignal(int signalId);
void handleShutDownSignal(int signalId);

int main(int argc, char *argv[])
{	
#if defined(Q_OS_UNIX) && defined(RLIMIT_NOFILE)
    /*
     Avoid 'Too many open files' on Unix.

     Increase the number of file descriptors that the process can open to the maximum allowed.
     - By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
     - On Linux, the default limit is usually 1024.
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

    bool isBackground;
    QString projectName,mainProcessServerName;
    QStringList writers;
    AppManager::parseCmdLineArgs(argc,argv,&isBackground,projectName,writers,mainProcessServerName);
    setShutDownSignal(SIGINT);   // shut down on ctrl-c
    setShutDownSignal(SIGTERM);   // shut down on killall
#ifdef Q_OS_UNIX
    if (!projectName.isEmpty()) {
        projectName = AppManager::qt_tildeExpansion(projectName);
    }
#endif
    if (isBackground) {
        if (projectName.isEmpty()) {
            ///Autobackground without a project file name is not correct
            AppManager::printUsage();
            return 1;
        }
        AppManager manager;
        if (!manager.load(argc,argv,projectName,writers,mainProcessServerName)) {
            AppManager::printUsage();
            return 1;
        } else {
            return 0;
        }
    } else {
        GuiApplicationManager manager;
        bool loaded = manager.load(argc,argv,projectName);
        if (!loaded) {
            return 1;
        }
        return manager.exec();
    }

}

void setShutDownSignal( int signalId )
{
#ifdef __NATRON_UNIX__
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1) {
        perror("setting up termination signal");
        exit(1);
    }
#endif
}


void handleShutDownSignal( int /*signalId*/ )
{
    QCoreApplication::exit(0);
}
