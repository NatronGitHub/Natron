//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include <QCoreApplication>

#if defined(Q_OS_UNIX)
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "Engine/AppManager.h"


int
main(int argc,
     char *argv[])
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
#ifdef Q_OS_UNIX
    projectName = AppManager::qt_tildeExpansion(projectName);
#endif

    ///auto-background without a project name is not valid.
    if ( projectName.isEmpty() ) {
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

    return 0;
}

