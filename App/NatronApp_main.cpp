//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include <csignal>
#include <cstdio>

#include <QApplication>

#if defined(Q_OS_UNIX)
//#include <sys/time.h> // <why ?
#include <sys/signal.h> ///for handling signals
#endif

#include "Gui/GuiApplicationManager.h"

void setShutDownSignal(int signalId);
void handleShutDownSignal(int signalId);
#include <fstream>
#include <sstream>
int
main(int argc,
     char *argv[])
{
    

    bool isBackground;
    QString projectName,mainProcessServerName;
    QStringList writers;
    AppManager::parseCmdLineArgs(argc,argv,&isBackground,projectName,writers,mainProcessServerName);
	setShutDownSignal(SIGINT);   // shut down on ctrl-c
    setShutDownSignal(SIGTERM);   // shut down on killall
#ifdef Q_OS_UNIX
    if ( !projectName.isEmpty() ) {
        projectName = AppManager::qt_tildeExpansion(projectName);
    }
#endif
    if (isBackground) {
        if ( projectName.isEmpty() ) {
            ///Autobackground without a project file name is not correct
            AppManager::printUsage();

            return 1;
        }
        AppManager manager;
        if ( !manager.load(argc,argv,projectName,writers,mainProcessServerName) ) {
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
} // main

void
setShutDownSignal( int signalId )
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

void
handleShutDownSignal( int /*signalId*/ )
{
    QCoreApplication::exit(0);
}

