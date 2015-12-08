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

#if defined(Q_OS_MAC)
#include "client/mac/crash_generation/crash_generation_server.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/crash_generation/crash_generation_server.h"
#elif defined(Q_OS_WIN32)
#include "client/windows/crash_generation/crash_generation_server.h"
#endif

#include "CallbacksManager.h"

using namespace google_breakpad;

/*Converts a std::string to wide string*/
inline std::wstring
s2ws(const std::string & s)
{
    
    
#ifdef Q_OS_WIN32
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
#else
    std::wstring dest;
    
    size_t max = s.size() * 4;
    mbtowc (NULL, NULL, max);  /* reset mbtowc */
    
    const char* cstr = s.c_str();
    
    while (max > 0) {
        wchar_t w;
        size_t length = mbtowc(&w,cstr,max);
        if (length < 1) {
            break;
        }
        dest.push_back(w);
        cstr += length;
        max -= length;
    }
    return dest;
#endif
    
}


void dumpRequest_xPlatform(const QString& filePath)
{
    CallbacksManager::instance()->s_emitDoCallBackOnMainThread(filePath);
}

#if defined(Q_OS_MAC)
void OnClientDumpRequest(void *context,
                         const ClientInfo &client_info,
                         const std::string &file_path) {

    dumpRequest_xPlatform(file_path.c_str());
}
#elif defined(Q_OS_LINUX)
void OnClientDumpRequest(void* context,
                         const ClientInfo* client_info,
                         const string* file_path) {

    dumpRequest_xPlatform(file_path->c_str());
}
#elif defined(Q_OS_WIN32)
void OnClientDumpRequest(void* context,
                         const google_breakpad::ClientInfo* client_info,
                         const std::wstring* file_path) {

    QString str = QString::fromWCharArray(file_path->c_str());
    dumpRequest_xPlatform(str);
}
#endif


QString printUsage()
{
    QString ret("Wrong number of arguments: ");
    ret += qApp->applicationName();
    ret += "  <breakpad_pipe> <fd> <natron_init_com_pipe> [--auto-upload] ";
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

    bool showUsage = false;
    if (argc < 4) {
        showUsage = true;
    }

    bool autoUpload = false;

    if (!showUsage) {
        if (args.size() == 5) {
            ///Optionnally on CLI only, --auto-upload can be given to upload crash reports instead of just saying where the dump is.
            autoUpload = args[4] == "--auto-upload";
        }
    }

    CallbacksManager manager(autoUpload);


    if (showUsage) {
        manager.writeDebugMessage(printUsage());
        return 1;
    }

    manager.writeDebugMessage("Crash reporter started with following arguments: " + qPipeName + " " + args[2] + " " + args[3]);


    QString dumpPath = QDir::tempPath();
#if defined(Q_OS_MAC)
    CrashGenerationServer breakpad_server(qPipeName.toStdString().c_str(),
                                          0, // filter cb
                                          0, // filter ctx
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          0, // exit cb
                                          0, // exit ctx
                                          true, // auto-generate dumps
                                          dumpPath.toStdString()); // path to dump to
#elif defined(Q_OS_LINUX)
    int listenFd = args[2].toInt();
    std::string stdDumpPath = dumpPath.toStdString();
    CrashGenerationServer breakpad_server(listenFd,
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          0, // exit cb
                                          0, // exit ctx
                                          true, // auto-generate dumps
                                          &stdDumpPath); // path to dump to
#elif defined(Q_OS_WIN32)
    std::string pipeName = qPipeName.toStdString();
    std::wstring wpipeName = s2ws(pipeName);
    std::string stdDumPath = dumpPath.toStdString();
    std::wstring stdWDumpPath = s2ws(stdDumPath);
    CrashGenerationServer breakpad_server(wpipeName,
                                          0, // SECURITY ATTRS
                                          0, // on client connected cb
                                          0, // on client connected ctx
                                          OnClientDumpRequest, // dump cb
                                          0, // dump ctx
                                          0, // exit cb
                                          0, // exit ctx
                                          0, // upload request cb
                                          0, //  upload request ctx
                                          true, // auto-generate dumps
                                          &stdWDumpPath); // path to dump to
#endif
    if (!breakpad_server.Start()) {
        manager.writeDebugMessage("Failure to start breakpad server, crash generation will fail.");
        return 1;
    } else {
        manager.writeDebugMessage("Crash generation server started successfully.");
    }
    

    manager.initOuptutPipe(args[3]);
    
    int ret = app.exec();
    manager.writeDebugMessage("Exiting now.");
    return ret;
}
