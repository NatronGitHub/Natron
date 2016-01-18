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

#include "ProcInfo.h"

#include <cassert>
#include <sstream>
#include <iostream>

#include <QDir>
#include <QStringList>
#include <QDebug>

namespace {

#ifdef __NATRON_OSX__

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
} // toQString

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
    DWORD v = GetModuleFileNameW(0, buffer, MAX_PATH + 1);
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
            v = GetModuleFileNameW(NULL, b, size);
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
} // currentPath
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

} // applicationFilePath_fromArgv

} // anon namespace

namespace Natron {

QString applicationFilePath(const char* argv0Param)
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
    ssize_t sizeofbuf = sizeof(char) * 2048;
    ssize_t size = readlink(filename.c_str(), buf, sizeofbuf);
    if (size != 0 && size != sizeofbuf) {
        //detected symlink
        return QString(QByteArray(buf));
    } else {
        return applicationFilePath_fromArgv(argv0Param);
    }
#endif
}

QString applicationDirPath(const char* argv0Param)
{
    QString filePath = applicationFilePath(argv0Param);
    int foundSlash = filePath.lastIndexOf('/');
    if (foundSlash == -1) {
        return QString();
    }
    return filePath.mid(0, foundSlash);
}
    
bool checkIfProcessIsRunning(const char* /*processAbsoluteFilePath*/, Q_PID /*pid*/)
{
    //Not working yet
    return true;
#if 0
#ifdef __NATRON_WIN32__
    DWORD dwExitCode = 9999;
    //See https://login.live.com/login.srf?wa=wsignin1.0&rpsnv=12&checkda=1&ct=1451385015&rver=6.0.5276.0&wp=MCMBI&wlcxt=msdn%24msdn%24msdn&wreply=https%3a%2f%2fmsdn.microsoft.com%2fen-us%2flibrary%2fwindows%2fdesktop%2fms683189%2528v%3dvs.85%2529.aspx&lc=1033&id=254354&mkt=en-US
    if (GetExitCodeProcess(pid, &dwExitCode)) {
        if (dwExitCode == STILL_ACTIVE) {
            return true;
        }
        return false;
    } else {
        qDebug() << "Call to GetExitCodeProcess failed.";
        return false;
    }
#elif defined(__NATRON_LINUX__)
    std::stringstream ss;
    ss << "/proc/" << (long)pid << "/stat";
    std::string procFile = ss.str();

    char buf[2048];
    std::size_t bufSize = sizeof(char) * 2048;
    if (readlink(procFile.c_str(), buf, bufSize) != -1) {
        //the file in /proc/ exists for the given PID
        //check that the path is the same than the given file path
        if (strcmp(buf, processAbsoluteFilePath) != 0) {
            return false;
        }
    } else {
        //process does not exist
        return false;
    }

    //Open the /proc/ file and check that the process is running
    FILE *fp = 0;
    fp = fopen(procFile.c_str(), "r");
    if (!fp) {
        return false;
    }

    char pname[512] = {0,};
    char state;
    //See http://man7.org/linux/man-pages/man5/proc.5.html
    if ( (fscanf(fp, "%ld (%[^)]) %c", &pid, pname, &state)) != 3 ){
        qDebug() << "checkIfProcessIsRunning(): fscanf call on" << procFile << "failed";
        fclose(fp);
        return false;
    }
    //If the process is running, return true
    bool ret = state == 'R';
    fclose(fp);
    return ret;
#elif defined(__NATRON_OSX__)
    //See https://developer.apple.com/legacy/library/qa/qa2001/qa1123.html
    static const int    name[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.

    struct kinfo_proc process;
    size_t procBufferSize = sizeof(process);
    int err = sysctl( (int *)name, 4,NULL, &procBufferSize, NULL, 0);
    if (err == 0 && procBufferSize != 0) {
 
        //Process exist and is running, now check that it's actual path is the given one
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        if (proc_pidpath (pid, pathbuf, sizeof(pathbuf) > 0)) {
            return !strcmp(pathbuf, processAbsoluteFilePath);
        }
        
        if (process.kp_proc.p_stat != SRUN && process.kp_proc.p_stat != SIDL) {
            //Process is not running
            return false;
        }
        return true;
        
    }
    return false;
#endif
#endif
} // checkIfProcessIsRunning

} // Natron
