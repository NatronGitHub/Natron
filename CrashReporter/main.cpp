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

#include <iostream>
#include <string>
#include <cassert>

#ifdef REPORTER_CLI_ONLY
#include <QCoreApplication>
#else
#include <QApplication>
#endif
#include <QStringList>
#include <QString>
#include <QDir>


#include "CallbacksManager.h"

QString printUsage()
{
    QString ret("Wrong number of arguments: ");
    ret += qApp->applicationName();
    ret += "  <crash_generation_pipe> <server_fd> <natron_init_com_pipe> [--auto-upload] ";
    return ret;
}

int
main(int argc,
     char *argv[])
{

#ifdef REPORTER_CLI_ONLY
    QCoreApplication app(argc,argv);
#else
    QApplication app(argc,argv);
    app.setQuitOnLastWindowClosed(false);
#endif

    QStringList args = app.arguments();
    if (args.size() < 4) {
        std::cerr << printUsage().toStdString() << std::endl;
        return 1;
    }
    assert(args.size() >= 4);
    QString qPipeName = args[1];
    int server_fd = args[2].toInt();
    QString comPipName = args[3];
    
    bool autoUpload = false;
    
    if (args.size() == 5) {
        ///Optionnally on CLI only, --auto-upload can be given to upload crash reports instead of just saying where the dump is.
        autoUpload = args[4] == "--auto-upload";
    }
    
    
    CallbacksManager manager(autoUpload);
    manager.writeDebugMessage("Crash reporter started with following arguments: " + qPipeName + " " + QString::number(server_fd) + " " + comPipName);


    QString dumpPath = QDir::tempPath();
    manager.initOuptutPipe(comPipName, qPipeName, dumpPath, server_fd);
    
    int ret = app.exec();
    
    manager.writeDebugMessage("Exiting now.");
    return ret;
}
