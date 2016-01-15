/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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
#include <sstream>

#include "Global/ProcInfo.h"

#ifdef REPORTER_CLI_ONLY
#include <QCoreApplication>
#else
#include <QApplication>
#endif
#include <QStringList>
#include <QString>
#include <QVarLengthArray>
#include <QDir>
#include <QProcess>

#include "CallbacksManager.h"


namespace {
#ifdef Q_OS_LINUX
static char* qstringToMallocCharArray(const QString& str)
{
    std::string stdStr = str.toStdString();
    return strndup(stdStr.c_str(), stdStr.size() + 1);
}
#endif
} // anon namespace

int
main(int argc,char *argv[])
{
    
    QString natronBinaryFilePath = Natron::applicationFilePath(argv[0]);
    QString natronBinaryPath;
    {
        int foundSlash = natronBinaryFilePath.lastIndexOf('/');
        if (foundSlash != -1) {
            natronBinaryPath = natronBinaryFilePath.mid(0, foundSlash);
        }
    }
    
    natronBinaryPath.append('/');

#ifndef REPORTER_CLI_ONLY
#ifdef Q_OS_UNIX
    natronBinaryPath.append("Natron-bin");
#elif defined(Q_OS_WIN)
    natronBinaryPath.append("Natron-bin.exe");
#endif
#else //REPORTER_CLI_ONLY
#ifdef Q_OS_UNIX
    natronBinaryPath.append("NatronRenderer-bin");
#elif defined(Q_OS_WIN)
    natronBinaryPath.append("NatronRenderer-bin.exe");
#endif
#endif
    

#ifdef NATRON_USE_BREAKPAD
    const bool enableBreakpad = true;
#else
    const bool enableBreakpad = false;
#endif
    
    /*
     Start breakpad generation server if requested
     */
    CallbacksManager manager;
    
#ifndef Q_OS_LINUX
    /*
     Do not declare qApp before fork() on linux
     */
#ifdef REPORTER_CLI_ONLY
    QCoreApplication app(argc,argv);
#else
    QApplication app(argc,argv);
#endif
#endif
    
    
    int client_fd = 0;
    QString pipePath;
    if (enableBreakpad) {
        try {
            manager.initCrashGenerationServer(&client_fd, &pipePath);
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return 1;
       
        }
    }
    
    /*
     At this point the crash generation server is created
     We do not want to declare the qApp variable yet before the fork so make sure to not used any qApp-related functions
     
     For MacOSX we do not use fork() because we need to init the pipe ourselves and we need to do it before the actual
     Natron process is launched. The only way to do it is to first declare qApp (because QLocalSocket requires it) and then
     declare the socket.
     */

#ifdef Q_OS_LINUX
    /*
     On Linux, directly fork this process so that we have no variable yet allocated.
     The child process will exec the actual Natron process. We have to do this in order
     for google-breakpad to correctly work:
     This process will use ptrace to inspect the crashing thread of Natron, but it can only
     do so on some Linux distro if Natron is a child process. 
     Also the crash reporter and Natron must share the same file descriptors for the crash generation
     server to work.
     */
    pid_t natronPID = fork();
    
    bool execOK = true;
    if (natronPID == 0) {
        /*
         We are the child process (i.e: Natron)
         */
        std::vector<char*> argvChild(argc);
        for (int i = 0; i < argc; ++i) {
            argvChild[i] = strdup(argv[i]);
        }
        
        
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_PROCESS_EXEC));
        argvChild.push_back(qstringToMallocCharArray(natronBinaryFilePath));
        
        argvChild.push_back(QString("--" NATRON_BREAKPAD_PROCESS_PID));
        QString pidStr = QString::number((qint64)natronPID);
        argvChild.push_back(pidStr);
        
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_CLIENT_FD_ARG));
        {
            char pipe_fd_string[8];
            sprintf(pipe_fd_string, "%d", client_fd);
            argvChild.push_back(pipe_fd_string);
        }
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_PIPE_ARG));
        argvChild.push_back(qstringToMallocCharArray(pipePath));
        
        execv(natronBinaryPath.toStdString().c_str(),&argvChild.front());
        execOK = false;
        std::cerr << "Forked process failed to execute " << natronBinaryPath.toStdString() << " make sure it exists." << std::endl;
        return 1;
    }
    if (!execOK) {
        return 1;
    }
    
#ifdef REPORTER_CLI_ONLY
    QCoreApplication app(argc,argv);
#else
    QApplication app(argc,argv);
#endif
    
#endif // Q_OS_LINUX
    
    /*
     On Windows we can only create a new process, it doesn't matter if qApp is defined before creating it
     */

#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    /*
     Create the actual Natron process now on Windows
     */
    QProcess actualProcess;
    QObject::connect(&actualProcess,SIGNAL(readyReadStandardOutput()), &manager, SLOT(onNatronProcessStdOutWrittenTo()));
    QObject::connect(&actualProcess,SIGNAL(readyReadStandardError()), &manager, SLOT(onNatronProcessStdErrWrittenTo()));
    manager.setProcess(&actualProcess);
    
    QStringList processArgs;
    for (int i = 0; i < argc; ++i) {
        processArgs.push_back(QString(argv[i]));
    }
    processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_EXEC));
    processArgs.push_back(natronBinaryFilePath);
    processArgs.push_back(QString("--" NATRON_BREAKPAD_PROCESS_PID));
    qint64 natron_pid = app.applicationPid();
    QString pidStr = QString::number(natron_pid);
    processArgs.push_back(pidStr);
    processArgs.push_back(QString("--" NATRON_BREAKPAD_CLIENT_FD_ARG));
    processArgs.push_back(QString::number(client_fd));
    processArgs.push_back(QString("--" NATRON_BREAKPAD_PIPE_ARG));
    processArgs.push_back(pipePath);
    
    actualProcess.start(natronBinaryPath, processArgs);
#endif

#ifndef REPORTER_CLI_ONLY
    app.setQuitOnLastWindowClosed(false);
#endif
    
    return app.exec();
}
