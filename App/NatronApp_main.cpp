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

#if defined(__linux__) || defined(__FreeBSD__)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <QApplication>

#include "Gui/GuiApplicationManager.h"

void setShutDownSignal(int signalId);
void handleShutDownSignal(int signalId);

int main(int argc, char *argv[])
{	
#if defined (Q_OS_UNIX)
    /*
     Avoid 'Too many open files' on Unix.
     Increase the number of file descriptors that the process can open to the maximum allowed.
    */
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    if (rl.rlim_max > rl.rlim_cur) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit(RLIMIT_NOFILE, &rl);
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
