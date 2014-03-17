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

