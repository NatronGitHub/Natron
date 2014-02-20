//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012. 
*contact: immarespond at gmail dot com
*
*/

#include <QApplication>

#include "Gui/GuiApplicationManager.h"


int main(int argc, char *argv[])
{	

    bool isBackground;
    QString projectName,mainProcessServerName;
    QStringList writers;
    AppManager::parseCmdLineArgs(argc,argv,&isBackground,projectName,writers,mainProcessServerName);
    if (isBackground) {
        QCoreApplication app(argc,argv);
        AppManager* manager = new AppManager;
        bool loaded = manager->load(projectName,writers,mainProcessServerName);
        delete manager;

        if (!loaded) {
            AppManager::printUsage();
            return 1;
        }
        return 0;
    } else {
        AppManager* manager = new GuiApplicationManager;

        //load and create data structures
        bool loaded = manager->load(argc,argv);
        assert(loaded);


        int ret =  qApp->exec();
        delete qApp;
        return ret;
    }

}

