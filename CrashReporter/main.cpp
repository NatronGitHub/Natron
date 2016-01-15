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

#ifdef Q_OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#endif

#ifdef Q_OS_UNIX
#include <dirent.h>
#include <sys/stat.h>
#endif


#include "CallbacksManager.h"

#include "Global/Macros.h"

namespace {

#ifdef Q_OS_MAC
    
    
/*
 Copied from Qt qcore_mac_p.h
 Helper class that automates refernce counting for CFtypes.
 After constructing the QCFType object, it can be copied like a
 value-based type.
 
 Note that you must own the object you are wrapping.
 This is typically the case if you get the object from a Core
 Foundation function with the word "Create" or "Copy" in it. If
 you got the object from a "Get" function, either retain it or use
 constructFromGet(). One exception to this rule is the
 HIThemeGet*Shape functions, which in reality are "Copy" functions.
*/
template <typename T>
class NatronCFType
{
public:
    inline NatronCFType(const T &t = 0) : type(t) {}
    inline NatronCFType(const NatronCFType &helper) : type(helper.type) { if (type) CFRetain(type); }
    inline ~NatronCFType() { if (type) CFRelease(type); }
    inline operator T() { return type; }
    inline NatronCFType operator =(const NatronCFType &helper)
    {
        if (helper.type)
            CFRetain(helper.type);
        CFTypeRef type2 = type;
        type = helper.type;
        if (type2)
            CFRelease(type2);
        return *this;
    }
    inline T *operator&() { return &type; }
    template <typename X> X as() const { return reinterpret_cast<X>(type); }
    static NatronCFType constructFromGet(const T &t)
    {
        CFRetain(t);
        return NatronCFType<T>(t);
    }
protected:
    T type;
};
   
    
class NatronCFString : public NatronCFType<CFStringRef>
{
public:
    inline NatronCFString(const QString &str) : NatronCFType<CFStringRef>(0), string(str) {}
    inline NatronCFString(const CFStringRef cfstr = 0) : NatronCFType<CFStringRef>(cfstr) {}
    inline NatronCFString(const NatronCFType<CFStringRef> &other) : NatronCFType<CFStringRef>(other) {}
    operator QString() const;
    operator CFStringRef() const;
    static QString toQString(CFStringRef cfstr);
    static CFStringRef toCFStringRef(const QString &str);
private:
    QString string;
};
    
QString NatronCFString::toQString(CFStringRef str)
{
    if (!str)
        return QString();
    
    CFIndex length = CFStringGetLength(str);
    if (length == 0)
        return QString();
    
    QString string(length, Qt::Uninitialized);
    CFStringGetCharacters(str, CFRangeMake(0, length), reinterpret_cast<UniChar *>(const_cast<QChar *>(string.unicode())));
    
    return string;
}
    
NatronCFString::operator QString() const
{
    if (string.isEmpty() && type)
        const_cast<NatronCFString*>(this)->string = toQString(type);
    return string;
}
    
CFStringRef NatronCFString::toCFStringRef(const QString &string)
{
    return CFStringCreateWithCharacters(0, reinterpret_cast<const UniChar *>(string.unicode()),
                                        string.length());
}
    
NatronCFString::operator CFStringRef() const
{
    if (!type) {
        const_cast<NatronCFString*>(this)->type =
        CFStringCreateWithCharactersNoCopy(0,
                                           reinterpret_cast<const UniChar *>(string.unicode()),
                                           string.length(),
                                           kCFAllocatorNull);
    }
    return type;
}
    
static QString applicationFileName()
{
    static QString appFileName;
    if (appFileName.isEmpty()) {
        NatronCFType<CFURLRef> bundleURL(CFBundleCopyExecutableURL(CFBundleGetMainBundle()));
        if (bundleURL) {
            NatronCFString cfPath(CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle));
            if (cfPath) {
                appFileName = cfPath;
            }
        }
    }
    return appFileName;
}
#elif defined(Q_OS_WIN)
static QString applicationFileName()
{
    // We do MAX_PATH + 2 here, and request with MAX_PATH + 1, so we can handle all paths
    // up to, and including MAX_PATH size perfectly fine with string termination, as well
    // as easily detect if the file path is indeed larger than MAX_PATH, in which case we
    // need to use the heap instead. This is a work-around, since contrary to what the
    // MSDN documentation states, GetModuleFileName sometimes doesn't set the
    // ERROR_INSUFFICIENT_BUFFER error number, and we thus cannot rely on this value if
    // GetModuleFileName(0, buffer, MAX_PATH) == MAX_PATH.
    // GetModuleFileName(0, buffer, MAX_PATH + 1) == MAX_PATH just means we hit the normal
    // file path limit, and we handle it normally, if the result is MAX_PATH + 1, we use
    // heap (even if the result _might_ be exactly MAX_PATH + 1, but that's ok).
    wchar_t buffer[MAX_PATH + 2];
    DWORD v = GetModuleFileName(0, buffer, MAX_PATH + 1);
    buffer[MAX_PATH + 1] = 0;
    
    if (v == 0)
        return QString();
    else if (v <= MAX_PATH)
        return QString::fromWCharArray(buffer);
    
    // MAX_PATH sized buffer wasn't large enough to contain the full path, use heap
    wchar_t *b = 0;
    int i = 1;
    size_t size;
    do {
        ++i;
        size = MAX_PATH * i;
        b = reinterpret_cast<wchar_t *>(realloc(b, (size + 1) * sizeof(wchar_t)));
        if (b)
            v = GetModuleFileName(NULL, b, size);
    } while (b && v == size);
    
    if (b)
        *(b + size) = 0;
    QString res = QString::fromWCharArray(b);
    free(b);
    
    return res;
}
#endif

#ifdef Q_OS_UNIX
static QString currentPath()
{
    struct stat st;
    int statFailed = stat(".", &st);
    assert(!statFailed);
    if (!statFailed) {
#if defined(__GLIBC__) && !defined(PATH_MAX)
        char *currentName = ::get_current_dir_name();
        if (currentName) {
            QString ret(currentName);
            ::free(currentName);
            return ret;
            
        }
#else
        char currentName[PATH_MAX+1];
        if (::getcwd(currentName, PATH_MAX)) {
            QString ret(currentName);
            return ret;
        }
#endif
    }
    return QString();
}
#endif

static QString applicationFilePath_fromArgv(const char* argv0Param)
{
#if defined( Q_OS_UNIX )
    QString argv0(argv0Param);
    QString absPath;
    
    if (!argv0.isEmpty() && argv0.at(0) == QLatin1Char('/')) {
        /*
         If argv0 starts with a slash, it is already an absolute
         file path.
         */
        absPath = argv0;
    } else if (argv0.contains(QLatin1Char('/'))) {
        /*
         If argv0 contains one or more slashes, it is a file path
         relative to the current directory.
         */
        absPath = currentPath();
        absPath.append(argv0);
    } else {
        /*
         Otherwise, the file path has to be determined using the
         PATH environment variable.
         */
        QByteArray pEnv = qgetenv("PATH");
        QString currentDirPath = currentPath();
        QStringList paths = QString::fromLocal8Bit(pEnv.constData()).split(QLatin1Char(':'));
        for (QStringList::const_iterator p = paths.constBegin(); p != paths.constEnd(); ++p) {
            if ((*p).isEmpty())
                continue;
            QString candidate = currentDirPath;
            candidate.append(*p + QLatin1Char('/') + argv0);
            
            struct stat _s;
            if (stat(candidate.toStdString().c_str(), &_s) == -1) {
                continue;
            }
            if (S_ISDIR(_s.st_mode)) {
                continue;
            }
            
            absPath = candidate;
            break;
        }
    }
    
    absPath = QDir::cleanPath(absPath);
    return absPath;
#endif

}

static QString applicationFilePath(const char* argv0Param)
{

#if defined(Q_WS_WIN)
    //The only viable solution
    return applicationFileName();
#elif defined(Q_WS_MAC)
    //First guess this way, then use the fallback solution
    QString appFile = applicationFileName();
    if (!appFile.isEmpty()) {
        return appFile;
    } else {
        return applicationFilePath_fromArgv(argv0Param);
    }
#elif defined(Q_OS_LINUX)
    // Try looking for a /proc/<pid>/exe symlink first which points to
    // the absolute path of the executable
    std::stringstream ss;
    ss << "/proc/" << getpid() << "/exe";
    std::string filename = ss.str();
    
    char buf[2048] = {0};
    std::size_t sizeofbuf = sizeof(char) * 2048;
    ssize_t size = readlink(filename.c_str(), buf, sizeofbuf);
    if (size != 0 && size != sizeofbuf) {
        //detected symlink
        return QString(QByteArray(buf));
    } else {
        return applicationFilePath_fromArgv(argv0Param);
    }
#endif
}

static QString applicationDirPath(const char* argv0Param)
{
    QString filePath = applicationFilePath(argv0Param);
    int foundSlash = filePath.lastIndexOf('/');
    if (foundSlash == -1) {
        return QString();
    }
    return filePath.mid(0, foundSlash);
}


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

    QString natronBinaryPath = applicationDirPath(argv[0]) + "/";
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
        
        
        argvChild.push_back(strdup("--" NATRON_BREAKPAD_ENABLED_ARG));
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
    processArgs.push_back(QString("--" NATRON_BREAKPAD_ENABLED_ARG));
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
