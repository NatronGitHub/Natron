/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Global/Macros.h"

#include <cassert>
#include <sstream> // stringstream
#include <iostream>
#include <cstring> // for std::memcpy, std::memset, std::strcmp
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#ifdef __NATRON_UNIX__
#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdlib> // malloc
#include <string.h> // strdup
#endif

#include "StrUtils.h"

NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

#if defined(__NATRON_WIN32__)
static std::string
applicationFileName()
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

    if (v == 0) {
        return std::string();
    } else if (v <= MAX_PATH) {
        std::wstring wret(buffer);
        std::string ret = StrUtils::utf16_to_utf8(wret);
        return ret;
    }

    // MAX_PATH sized buffer wasn't large enough to contain the full path, use heap
    wchar_t *b = 0;
    int i = 1;
    size_t size;
    do {
        ++i;
        size = MAX_PATH * i;
        b = reinterpret_cast<wchar_t *>( realloc( b, (size + 1) * sizeof(wchar_t) ) );
        if (b) {
            v = GetModuleFileNameW(NULL, b, size);
        }
    } while (b && v == size);

    if (b) {
        *(b + size) = 0;
    }
    std::wstring wideRet(b);
    std::string ret = StrUtils::utf16_to_utf8(wideRet);
    free(b);
    return ret;
}

#endif // defined(__NATRON_WIN32__)


#ifdef __NATRON_UNIX__
// Copyright (C) 2011-2018 Free Software Foundation, Inc.
// getcwd-lgpl.c from
// https://raw.githubusercontent.com/coreutils/gnulib/master/lib/getcwd-lgpl.c

/* Get the name of the current working directory, and put it in SIZE
   bytes of BUF.  Returns NULL if the directory couldn't be determined
   (perhaps because the absolute name was longer than PATH_MAX, or
   because of missing read/search permissions on parent directories)
   or SIZE was too small.  If successful, returns BUF.  If BUF is
   NULL, an array is allocated with 'malloc'; the array is SIZE bytes
   long, unless SIZE == 0, in which case it is as big as
   necessary.  */

static
char *
rpl_getcwd (char *buf, size_t size)
{
    char *ptr;
    char *result;

    /* Handle single size operations.  */
    if (buf) {
        if (!size) {
            errno = EINVAL;
            return NULL;
        }
        return ::getcwd(buf, size);
    }

    if (size) {
        buf = (char*)std::malloc(size);
        if (!buf) {
            errno = ENOMEM;
            return NULL;
        }
        result = ::getcwd(buf, size);
        if (!result) {
            int saved_errno = errno;
            std::free(buf);
            errno = saved_errno;
        }
        return result;
    }

    /* Flexible sizing requested.  Avoid over-allocation for the common
     case of a name that fits within a 4k page, minus some space for
     local variables, to be sure we don't skip over a guard page.  */
    {
        char tmp[4032];
        size = sizeof tmp;
        ptr = ::getcwd (tmp, size);
        if (ptr) {
            result = ::strdup(ptr);
            if (!result) {
                errno = ENOMEM;
            }
            return result;
        }
        if (errno != ERANGE) {
            return NULL;
        }
    }

    /* My what a large directory name we have.  */
    do {
        size <<= 1;
        ptr = (char*)std::realloc(buf, size);
        if (ptr == NULL) {
            std::free(buf);
            errno = ENOMEM;
            return NULL;
        }
        buf = ptr;
        result = ::getcwd(buf, size);
    } while (!result && errno == ERANGE);

    if (!result) {
        int saved_errno = errno;
        std::free(buf);
        errno = saved_errno;
    } else {
        /* Trim to fit, if possible.  */
        result = (char*)std::realloc(buf, std::strlen(buf) + 1);
        if (!result) {
            result = buf;
        }
    }
    return result;
}

#endif


#if defined( __NATRON_UNIX__ )
static std::string
applicationFilePath_fromArgv(const char* argv0Param)
{
    std::string argv0;
    if (argv0Param) {
        argv0 = std::string(argv0Param);
    }
    std::string absPath;

    if ( !argv0.empty() && ( argv0[0] == '/' ) ) {
        /*
           If argv0 starts with a slash, it is already an absolute
           file path.
         */
        absPath = argv0;
    } else if ( argv0.find_first_of("/") != std::string::npos ) {
        /*
           If argv0 contains one or more slashes, it is a file path
           relative to the current directory.
         */
        char* cwd = rpl_getcwd(NULL, 0);
        absPath = cwd;
        std::free(cwd);
        absPath.append(argv0);
    } else {
        /*
           Otherwise, the file path has to be determined using the
           PATH environment variable.
         */
        std::string pEnv = getenv("PATH");
        char* cwd = rpl_getcwd(NULL, 0);
        std::string currentDirPath = cwd;
        std::free(cwd);
        std::vector<std::string> paths = StrUtils::split(pEnv, ':');
        for (std::vector<std::string>::const_iterator it = paths.begin(); it != paths.end(); ++it) {
            if (it->empty()) {
                continue;
            }
            std::string candidate = currentDirPath;
            candidate.append(*it + '/' + argv0);

            struct stat _s;
            if (stat(candidate.c_str(), &_s) == -1) {
                continue;
            }
            if ( S_ISDIR(_s.st_mode) ) {
                continue;
            }

            absPath = candidate;
            break;
        }
    }

    absPath = StrUtils::cleanPath(absPath);

    return absPath;
} // applicationFilePath_fromArgv

#endif // defined(__NATRON_UNIX__)


#if defined(__NATRON_OSX__)

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#include <libproc.h>
#include <sys/sysctl.h>


/*
 Copied from Qt qcore_mac_p.h
 Helper class that automates reference counting for CFtypes.
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
    inline NatronCFType(const T &t = 0)
    : type(t) {}

    inline NatronCFType(const NatronCFType &helper)
    : type(helper.type)
    {
        if (type) {CFRetain(type); }
    }

    inline ~NatronCFType()
    {
        if (type) {CFRelease(type); }
    }

    inline operator T() { return type; }

    inline NatronCFType operator =(const NatronCFType &helper)
    {
        if (helper.type) {
            CFRetain(helper.type);
        }
        CFTypeRef type2 = type;
        type = helper.type;
        if (type2) {
            CFRelease(type2);
        }

        return *this;
    }

    inline T *operator&() { return &type; }

    template <typename X>
    X as() const { return reinterpret_cast<X>(type); }

    static NatronCFType constructFromGet(const T &t)
    {
        CFRetain(t);

        return NatronCFType<T>(t);
    }

protected:
    T type;
};


class NatronCFString
: public NatronCFType<CFStringRef>
{
public:
    inline NatronCFString(const std::string &str)
    : NatronCFType<CFStringRef>(0), string(str) {}

    inline NatronCFString(const CFStringRef cfstr = 0)
    : NatronCFType<CFStringRef>(cfstr) {}

    inline NatronCFString(const NatronCFType<CFStringRef> &other)
    : NatronCFType<CFStringRef>(other) {}

    operator std::string() const;
    operator CFStringRef() const;
    static std::string toStdString(CFStringRef cfstr);
    static CFStringRef toCFStringRef(const std::string &str);

private:
    std::string string;
};

std::string
NatronCFString::toStdString(CFStringRef str)
{
    if (!str) {
        return std::string();
    }

    CFIndex length = CFStringGetLength(str) + 1;
    if (length == 0) {
        return std::string();
    }

    char *buffer = (char*)std::malloc(sizeof(char) * (length));

    //CFStringGetCharacters( str, CFRangeMake(0, length), reinterpret_cast<UniChar *>( const_cast<char *>( string.c_str() ) ) );
    int ok = CFStringGetCString(str, buffer, length, kCFStringEncodingUTF8);
    assert(ok);
    std::string ret(buffer, length - 1);
    std::free(buffer);

    return ret;
} // toStdString

NatronCFString::operator std::string() const
{
    if (string.empty() && type) {
        const_cast<NatronCFString*>(this)->string = toStdString(type);
    }

    return string;
}

CFStringRef
NatronCFString::toCFStringRef(const std::string &string)
{
    return CFStringCreateWithCharacters( 0, reinterpret_cast<const UniChar *>( string.c_str() ),
                                        string.length() );
}

NatronCFString::operator CFStringRef() const
{
    if (!type) {
        const_cast<NatronCFString*>(this)->type =
        CFStringCreateWithCharactersNoCopy(0,
                                           reinterpret_cast<const UniChar *>( string.c_str() ),
                                           string.length(),
                                           kCFAllocatorNull);
    }
    
    return type;
}

#endif // defined(__NATRON_OSX__)

NATRON_NAMESPACE_ANONYMOUS_EXIT


std::string
ProcInfo::applicationFilePath(const char* argv0Param)
{
#if defined(__NATRON_WIN32__)
    (void)argv0Param;
    
    //The only viable solution
    return applicationFileName();
#elif defined(__NATRON_OSX__)
    //First guess this way, then use the fallback solution
    static std::string appFileName;

    if ( appFileName.empty() ) {
        NatronCFType<CFURLRef> bundleURL( CFBundleCopyExecutableURL( CFBundleGetMainBundle() ) );
        if (bundleURL) {
            NatronCFString cfPath( CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle) );
            if (cfPath) {
                appFileName = cfPath;
            }
        }
    }
    if ( !appFileName.empty() ) {
        return appFileName;
    } else {
        return applicationFilePath_fromArgv(argv0Param);
    }
#elif defined(__NATRON_LINUX__)
    // Try looking for a /proc/<pid>/exe symlink first which points to
    // the absolute path of the executable
    std::stringstream ss;
    ss << "/proc/" << getpid() << "/exe";
    std::string filename = ss.str();
    char buf[2048] = {0};
    ssize_t sizeofbuf = sizeof(char) * 2048;
    ssize_t size = readlink(filename.c_str(), buf, sizeofbuf);
    if ( (size != 0) && (size != sizeofbuf) ) {
        //detected symlink
        std::string ret(buf);
        return ret;
    } else {
        return applicationFilePath_fromArgv(argv0Param);
    }
#endif
}

std::string
ProcInfo::applicationDirPath(const char* argv0Param)
{

    std::string filePath = ProcInfo::applicationFilePath(argv0Param);
    std::size_t foundSlash = filePath.find_last_of('/');

    if (foundSlash == std::string::npos) {
#ifdef __NATRON_WIN32__
        foundSlash = filePath.find_last_of('\\');
        if (foundSlash == std::string::npos) {
            return std::string();
        }
#else
        return std::string();
#endif
    }

    return filePath.substr(0, foundSlash);
}


bool
ProcInfo::putenv_wrapper(const char *varName, const std::string& value)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return _putenv_s(varName, value.c_str()) == 0;
#else
    std::string buffer(varName);
    buffer += '=';
    buffer += value;
    char* envVar = strdup(buffer.c_str());
    int result = putenv(envVar);
    if (result != 0) {
        // error. we have to delete the string.
        free(envVar);
    }
    return result == 0;
#endif
}

std::string
ProcInfo::getenv_wrapper(const char *varName)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    size_t requiredSize = 0;
    std::vector<char> buffer;
    getenv_s(&requiredSize, 0, 0, varName);
    if (requiredSize == 0)
        return buffer;
    buffer.resize(requiredSize);
    getenv_s(&requiredSize, &buffer[0], requiredSize, varName);
    // requiredSize includes the terminating null, which we don't want.
    assert(buffer[requiredSize - 1] == '\0');
    std::string ret(&buffer[0]);
    return ret;
#else
    char* v = ::getenv(varName);
    if (!v) {
        return std::string();
    } else {
        return std::string(v);
    }
#endif
}


/**
 * @brief Ensures that the command line arguments passed to main are Utf8 encoded. On Windows
 * the command line arguments must be processed to be safe.
 **/
void
ProcInfo::ensureCommandLineArgsUtf8(int argc, char **argv, std::vector<std::string>* utf8Args)
{
    assert(utf8Args);
#ifndef __NATRON_WIN32__
    // On Unix, command line args are Utf8
    assert(!argc || argv);
    for (int i = 0; i < argc; ++i) {
        std::string str(argv[i]);
        assert(StrUtils::is_utf8(str.c_str()));
        utf8Args->push_back(str);
    }
#else
    // On Windows, it must be converted: http://stackoverflow.com/questions/5408730/what-is-the-encoding-of-argv
    (void)argc;
    (void)argv;

    int nArgsOut;
    wchar_t** argList = CommandLineToArgvW(GetCommandLineW(), &nArgsOut);
    for (int i = 0; i < nArgsOut; ++i) {
        std::wstring wide(argList[i]);
        std::string utf8Str = StrUtils::utf16_to_utf8(wide);
        assert(StrUtils::is_utf8(utf8Str.c_str()));
        utf8Args->push_back(utf8Str);
        if (argv) {
            std::cout << "Non UTF-8 arg: " <<  argv[i] << std::endl;
        }
        std::cout << "UTF-8 arg: " <<  utf8Args->back() << std::endl;
    }
    // Free memory allocated for CommandLineToArgvW arguments.
    LocalFree(argList);


#endif
    
}

void
ProcInfo::ensureCommandLineArgsUtf8(int argc, wchar_t **argv, std::vector<std::string>* utf8Args)
{
    for (int i = 0; i < argc; ++i) {
        std::wstring ws(argv[i]);
        std::string utf8Str = StrUtils::utf16_to_utf8(ws);
        assert(StrUtils::is_utf8(utf8Str.c_str()));
        utf8Args->push_back(utf8Str);
    }
}

long long
ProcInfo::getCurrentProcessPID()
{
#if defined(__NATRON_WIN32__)
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool
ProcInfo::checkIfProcessIsRunning(const char* processAbsoluteFilePath,
                                  long long pid)
{
#if defined(__NATRON_WIN32__)
    (void)processAbsoluteFilePath;
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!processHandle) {
#ifdef DEBUG
        std::cerr << "checkIfProcessIsRunning: process " << pid << " is not running." << std::endl;
#endif
        return false;
    }
    DWORD dwExitCode = 9999;
    //See https://login.live.com/login.srf?wa=wsignin1.0&rpsnv=12&checkda=1&ct=1451385015&rver=6.0.5276.0&wp=MCMBI&wlcxt=msdn%24msdn%24msdn&wreply=https%3a%2f%2fmsdn.microsoft.com%2fen-us%2flibrary%2fwindows%2fdesktop%2fms683189%2528v%3dvs.85%2529.aspx&lc=1033&id=254354&mkt=en-US

    bool caughtExitCode = GetExitCodeProcess(processHandle, &dwExitCode);
    CloseHandle(processHandle);
    if (caughtExitCode) {

        if (dwExitCode == STILL_ACTIVE) {
            return true;
        }

        return false;
    } else {
#ifdef DEBUG
        std::cerr << "Call to GetExitCodeProcess failed." << std::endl;
#endif
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
        if (std::strcmp(buf, processAbsoluteFilePath) != 0) {
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

    char pname[512] = {0, };
    char state;
    //See http://man7.org/linux/man-pages/man5/proc.5.html
    if ( ( fscanf(fp, "%lld (%[^)]) %c", &pid, pname, &state) ) != 3 ) {
#ifdef DEBUG
        std::cerr << "ProcInfo::checkIfProcessIsRunning(): fscanf call on " << procFile << " failed";
#endif
        fclose(fp);

        return false;
    }
    //If the process is running, return true
    bool ret = state == 'R';
    fclose(fp);

    return ret;
#elif defined(__NATRON_OSX__)
    //See https://developer.apple.com/legacy/library/qa/qa2001/qa1123.html
    int err;
    struct kinfo_proc * result;
    bool done;
    static const int name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, (int)pid, 0 };
    // Declaring name as const requires us to cast it when passing it to
    // sysctl because the prototype doesn't include the const modifier.
    size_t length;

    //struct kinfo_proc process;
    //size_t procBufferSize = sizeof(process);
    // We start by calling sysctl with result == NULL and length == 0.
    // That will succeed, and set length to the appropriate length.
    // We then allocate a buffer of that size and call sysctl again
    // with that buffer.  If that succeeds, we're done.  If that fails
    // with ENOMEM, we have to throw away our buffer and loop.  Note
    // that the loop causes use to call sysctl with NULL again; this
    // is necessary because the ENOMEM failure case sets length to
    // the amount of data returned, not the amount of data that
    // could have been returned.

    result = NULL;
    done = false;
    do {
        assert(result == NULL);

        // Call sysctl with a NULL buffer.

        length = 0;
        err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                     NULL, &length,
                     NULL, 0);
        if (err == -1) {
            err = errno;
        }

        // Allocate an appropriately sized buffer based on the results
        // from the previous call.

        if (err == 0) {
            result = (struct kinfo_proc *)malloc(length);
            if (result == NULL) {
                err = ENOMEM;
            }
        }

        // Call sysctl again with the new buffer.  If we get an ENOMEM
        // error, toss away our buffer and start again.

        if (err == 0) {
            err = sysctl( (int *) name, (sizeof(name) / sizeof(*name)) - 1,
                         result, &length,
                         NULL, 0);
            if (err == -1) {
                err = errno;
            }
            if (err == 0) {
                done = true;
            } else if (err == ENOMEM) {
                assert(result != NULL);
                free(result);
                result = NULL;
                err = 0;
            }
        }
    } while (err == 0 && ! done);

    bool retval = false;

    if ( (err == 0) && (length != 0) ) {
        //Process exist and is running, now check that it's actual path is the given one
        char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
        int len = proc_pidpath ( pid, pathbuf, sizeof(pathbuf));
        if (len > 0) {
            retval = !std::strcmp(pathbuf, processAbsoluteFilePath);
        } else if ( (result->kp_proc.p_stat != SRUN) && (result->kp_proc.p_stat != SIDL) ) {
            //Process is not running
            retval = false;
        } else {
            retval = true;
        }
    }
    if (err != 0 && result != NULL) {
        free(result);
        result = NULL;
    }

    return retval;
#endif // ifdef Q_OS_WIN
} // ProcInfo::checkIfProcessIsRunning

NATRON_NAMESPACE_EXIT
