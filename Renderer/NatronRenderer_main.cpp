//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include <QCoreApplication>

#include "Engine/AppManager.h"


int main(int argc, char *argv[])
{
#if defined (Q_OS_MAC)
    /*
     Avoid 'Too many open files' on Mac

     Increase the number of file descriptors that the process can open to the maximum allowed.
     By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
     */
    // see also https://qt.gitorious.org/qt-creator/qt-creator/commit/7f1f9e1
    // increase maximum numbers of file descriptors
	struct rlimit rl;
	getrlimit(RLIMIT_NOFILE, &rl);
 	rl.rlim_cur = qMin((rlim_t)OPEN_MAX, rl.rlim_max);
	setrlimit(RLIMIT_NOFILE, &rl);
#endif

    bool isBackground;
    QString projectName,mainProcessServerName;
    QStringList writers;
    AppManager::parseCmdLineArgs(argc,argv,&isBackground,projectName,writers,mainProcessServerName);
    AppManager manager;
    if (!manager.load(argc,argv,projectName,writers,mainProcessServerName)) {
        AppManager::printUsage();
        return 1;
    } else {
        return 0;
    }
    return 0;
}

