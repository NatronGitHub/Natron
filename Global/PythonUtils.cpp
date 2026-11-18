/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#undef Py_LIMITED_API  // Needed for PyConfig, PyBytes_AS_STRING
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if PY_VERSION_HEX < 0x03080000
#error "Natron needs Python 3.8 or later (the PyConfig initialization API, PEP 587)"
#endif

#include "PythonUtils.h"

#include <cstdlib>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h> // GetFullPathNameW
#endif
#ifdef _MSC_VER
#pragma warning (disable : 4996)
#else
#include <dirent.h>
#endif

#include "../Global/FStreamsSupport.h"
#include "../Global/ProcInfo.h"
#include "../Global/StrUtils.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

static bool fileExists(const std::string& path)
{
    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open(&ifile, path);
    return ifile.good();
}

static bool dirExists(const std::string& path)
{
    // https://stackoverflow.com/q/18100097
    struct stat info;

    if(stat(path.c_str(), &info ) != 0) {
        return false;
    } else if(info.st_mode & S_IFDIR) {
        return true;
    }
    return false;
}

std::wstring setupPythonEnv(const std::string& binPath)
{
    //Disable user sites as they could conflict with Natron bundled packages.
    //If this is set, Python won’t add the user site-packages directory to sys.path.
    //See https://www.python.org/dev/peps/pep-0370/
    // (the PyConfig used to initialize Python also sets user_site_directory = 0;
    // the environment variable is for the Python subprocesses)
    ProcInfo::putenv_wrapper("PYTHONNOUSERSITE", "1");

    //
    // set up paths, clear those that don't exist or are not valid
    //
#ifdef __NATRON_WIN32__
    std::string pythonHome = binPath + "\\..";
    std::string pyPathZip = pythonHome + "\\lib\\python" NATRON_PY_VERSION_STRING_NO_DOT ".zip";
    std::string pyPath = pythonHome +  "\\lib\\python" NATRON_PY_VERSION_STRING;
    std::string pyPathDynLoad = pyPath + "\\lib-dynload";
    std::string pyPathSitePackages = pyPath + "\\site-packages";
    std::string pluginPath = binPath + "\\..\\Plugins";
#else
#  if defined(__NATRON_LINUX__)
    std::string pythonHome = binPath + "/..";
#  elif defined(__NATRON_OSX__)
    std::string pythonHome = binPath+ "/../Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING;
#  else
#    error "unsupported platform"
#  endif
    std::string pyPathZip = pythonHome + "/lib/python" NATRON_PY_VERSION_STRING_NO_DOT ".zip";
    std::string pyPath = pythonHome + "/lib/python" NATRON_PY_VERSION_STRING;
    std::string pyPathDynLoad = pyPath + "/lib-dynload";
    std::string pyPathSitePackages = pyPath + "/site-packages";
    std::string pluginPath = binPath + "/../Plugins";
#endif
    if ( !fileExists( StrUtils::fromNativeSeparators(pyPathZip) ) ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "\"%s\" does not exist, not added to PYTHONPATH\n", pyPathZip.c_str() );
#     endif
        pyPathZip.clear();
    }
    if ( !dirExists( StrUtils::fromNativeSeparators(pyPath) ) ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "\"%s\" does not exist, not added to PYTHONPATH\n", pyPath.c_str() );
#     endif
        pyPath.clear();
    }
    if ( !dirExists( StrUtils::fromNativeSeparators(pyPathDynLoad) ) ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "\"%s\" does not exist, not added to PYTHONPATH\n", pyPathDynLoad.c_str() );
#     endif
        pyPathDynLoad.clear();
    }
    if ( !dirExists( StrUtils::fromNativeSeparators(pyPathSitePackages) ) ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "\"%s\" does not exist, not added to PYTHONPATH\n", pyPathSitePackages.c_str() );
#     endif
        pyPathSitePackages.clear();
    }
    if ( !dirExists( StrUtils::fromNativeSeparators(pluginPath) ) ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "\"%s\" does not exist, not added to PYTHONPATH\n", pluginPath.c_str() );
#     endif
        pluginPath.clear();
    }
    // PYTHONHOME is really useful if there's a python inside it
    if ( pyPathZip.empty() && pyPath.empty() ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "dir \"%s\" does not exist or does not contain lib/python*, not setting PYTHONHOME\n", pythonHome.c_str() );
#     endif
        pythonHome.clear();
    }

    /////////////////////////////////////////
    // Python home
    /////////////////////////////////////////
    //
    // Returned to the caller, which passes it to initializePython3() or runPythonMain()
    // (PyConfig.home, formerly Py_SetPythonHome()).

#   if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
    if ( !pythonHome.empty() ) {
        printf( "Python home: \"%s\"\n", pythonHome.c_str() );
    }
#   endif


    /////////////////////////////////////////
    // PYTHONPATH and Py_SetPath
    /////////////////////////////////////////
    //
    // note: to check the python path of a python install, execute:
    // python -c 'import sys,pprint; pprint.pprint( sys.path )'
    //
    // to build the python27.zip, cd to lib/python2.7, and generate the pyo and the zip file using:
    //
    //  python -O -m compileall .
    //  zip -r ../python27.zip *.py* bsddb compiler ctypes curses distutils email encodings hotshot idlelib importlib json logging multiprocessing pydoc_data sqlite3 unittest wsgiref xml
    //
    std::string pythonPath = ProcInfo::getenv_wrapper("PYTHONPATH");
    //Add the Python distribution of Natron to the Python path

    std::vector<std::string> toPrepend;
    if ( !pyPathZip.empty() ) {
        toPrepend.push_back(pyPathZip);
    }
    if ( !pyPath.empty() ) {
        toPrepend.push_back(pyPath);
    }
    if ( !pyPathDynLoad.empty() ) {
        toPrepend.push_back(pyPathDynLoad);
    }
    if ( !pyPathSitePackages.empty() ) {
        toPrepend.push_back(pyPathSitePackages);
    }
    if ( !pluginPath.empty() ) {
        toPrepend.push_back(pluginPath);
    }

#if defined(__NATRON_OSX__) && defined DEBUG
    // in debug mode, also prepend the local PySide directory
    // homebrew's pyside directory
    toPrepend.push_back("/usr/local/Cellar/pyside@1.2/1.2.2_2/lib/python" NATRON_PY_VERSION_STRING "/site-packages");
    // macport's pyside directory
    toPrepend.push_back("/opt/local/Library/Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING "/lib/python" NATRON_PY_VERSION_STRING "/site-packages");
#endif

    if ( toPrepend.empty() ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf("PYTHONPATH not modified\n");
#     endif
    } else {
#     ifdef __NATRON_WIN32__
        const char pathSep = ';';
#     else
        const char pathSep = ':';
#     endif
        std::string toPrependStr = StrUtils::join(toPrepend, pathSep);
        if (pythonPath.empty()) {
            pythonPath = toPrependStr;
        } else {
            pythonPath = toPrependStr + pathSep + pythonPath;
        }
        // Py_SetPath() sets the whole path, but setting PYTHONPATH still keeps the system's python path
        ProcInfo::putenv_wrapper( "PYTHONPATH", pythonPath.c_str() );
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "PYTHONPATH set to %s\n", pythonPath.c_str() );
#     endif
    }

    return pythonHome.empty() ? std::wstring() : StrUtils::utf8_to_utf16(pythonHome);
} // setupPythonEnv

// Exit on a Python initialization error, as Py_Initialize() and Py_Main() do.
static void
exitIfPyStatusException(PyStatus status,
                        PyConfig* config)
{
    if ( PyStatus_Exception(status) ) {
        PyConfig_Clear(config);
        Py_ExitStatusException(status); // prints the error message and exits
    }
}

// The first item of sys.path, as computed by PySys_SetArgv(), which is deprecated since
// Python 3.11 (see _PyPathConfig_ComputeSysPath0() in CPython's Python/pathconfig.c):
// the directory of argv[0], with symbolic links resolved, or "" if argv[0] has no directory.
static std::wstring
sysPath0FromArgv0(const wchar_t* argv0)
{
#ifdef __NATRON_WIN32__
    std::wstring path0(argv0);
    wchar_t fullpath[MAX_PATH];
    wchar_t* filePart;
    DWORD len = GetFullPathNameW(argv0, MAX_PATH, fullpath, &filePart);
    if ( (len > 0) && (len < MAX_PATH) ) {
        path0 = fullpath;
    }
    std::size_t sep = path0.find_last_of(L"\\/");
    if (sep == std::wstring::npos) {
        return std::wstring();
    }
    std::size_t n = sep + 1;
    if ( (n > 1) && (path0[sep - 1] != L':') ) {
        --n; // drop the trailing separator, except after a drive letter
    }

    return path0.substr(0, n);
#else
    std::string path0 = StrUtils::utf16_to_utf8(argv0);
    char* fullpath = realpath(path0.c_str(), NULL);
    if (fullpath) {
        path0 = fullpath;
        std::free(fullpath);
    }
    std::size_t sep = path0.rfind('/');
    if (sep == std::string::npos) {
        return std::wstring();
    }
    std::size_t n = sep + 1;
    if (n > 1) {
        --n; // drop the trailing separator, except for "/"
    }

    return StrUtils::utf8_to_utf16( path0.substr(0, n) );
#endif
}

PyObject* initializePython3(const std::vector<wchar_t*>& commandLineArgsWide,
                            const std::wstring& pythonHome)
{
    //See https://developer.blender.org/T31507
    //Python will not load anything in site-packages if this is set
    //We are sure that nothing in system wide site-packages is loaded, for instance on OS X with Python installed
    //through macports on the system, the following printf show the following:

    /*Py_GetProgramName is /Applications/Natron.app/Contents/MacOS/Natron
     Py_GetPrefix is /Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7
     Py_GetExecPrefix is /Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7
     Py_GetProgramFullPath is /Applications/Natron.app/Contents/MacOS/Natron
     Py_GetPath is /Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7:/Applications/Natron.app/Contents/MacOS/../Plugins:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python27.zip:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/plat-darwin:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/plat-mac:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/plat-mac/lib-scriptpackages:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/lib-tk:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/lib-old:/Applications/Natron.app/Contents/MacOS/../Frameworks/Python.framework/Versions/2.7/lib/python2.7/lib-dynload
     Py_GetPythonHome is ../Frameworks/Python.framework/Versions/2.7/lib
     Python library is in /Applications/Natron.app/Contents/Frameworks/Python.framework/Versions/2.7/lib/python2.7/site-packages*/

    //Py_NoSiteFlag = 1;

    /////////////////////////////////////////
    // Py_InitializeFromConfig
    /////////////////////////////////////////
    //
    // Initialize the Python interpreter with the PyConfig API (PEP 587), which replaces
    // Py_SetProgramName(), Py_SetPythonHome(), Py_NoUserSiteDirectory and PySys_SetArgv(),
    // deprecated since Python 3.11.
    // The configuration is the one Py_Initialize() used (its "compat" configuration, which is
    // not public): the "Python configuration" without command line parsing, without
    // configuring the C standard streams, and without the UTF-8 mode or the C locale
    // coercion, which Py_Initialize() did not enable either.
#if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
    printf("Py_InitializeFromConfig()\n");
#endif
    PyStatus status;
    {
        PyPreConfig preconfig;
        PyPreConfig_InitPythonConfig(&preconfig);
        preconfig.parse_argv = 0;
        preconfig.utf8_mode = 0;
        preconfig.coerce_c_locale = 0;
        preconfig.coerce_c_locale_warn = 0;
        status = Py_PreInitialize(&preconfig);
        if ( PyStatus_Exception(status) ) {
            Py_ExitStatusException(status);
        }
    }
    {
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.parse_argv = 0; // the Natron command line is not a Python command line
        config.configure_c_stdio = 0;
        config.user_site_directory = 0; // see setupPythonEnv()
        // the program name (formerly Py_SetProgramName())
        status = PyConfig_SetString(&config, &config.program_name, commandLineArgsWide[0]);
        exitIfPyStatusException(status, &config);
        // the Python home (formerly Py_SetPythonHome()): if empty, PYTHONHOME is used, as before
        if ( !pythonHome.empty() ) {
            status = PyConfig_SetString( &config, &config.home, pythonHome.c_str() );
            exitIfPyStatusException(status, &config);
        }
        status = Py_InitializeFromConfig(&config);
        exitIfPyStatusException(status, &config);
        PyConfig_Clear(&config);
    }

    // sys.prefix and sys.exec_prefix are the Python home
    // https://github.com/NatronGitHub/Natron/issues/696
    if ( !pythonHome.empty() ) {
        PyObject *prefix = PyUnicode_FromWideChar(pythonHome.c_str(), -1);
        PySys_SetObject(const_cast<char*>("prefix"), prefix);
        PySys_SetObject(const_cast<char*>("exec_prefix"), prefix);
        Py_XDECREF(prefix);
    }

    /////////////////////////////////////////
    // sys.argv and sys.path[0]
    /////////////////////////////////////////
    //
    // As PySys_SetArgv() did (PyConfig.argv would also set sys.argv, but only Py_RunMain()
    // would insert sys.path[0]): sys.argv is the Natron command line, and the directory of
    // argv[0] is prepended to sys.path (relative module import).
    {
        PyObject* argv = PyList_New(0);
        for (std::size_t i = 0; i < commandLineArgsWide.size(); ++i) {
            PyObject* arg = PyUnicode_FromWideChar(commandLineArgsWide[i], -1);
            if (arg) {
                PyList_Append(argv, arg);
                Py_DECREF(arg);
            }
        }
        PySys_SetObject(const_cast<char*>("argv"), argv);
        Py_XDECREF(argv);

        PyObject* sysPath = PySys_GetObject("path"); // borrowed reference
        PyObject* path0 = PyUnicode_FromWideChar(sysPath0FromArgv0(commandLineArgsWide[0]).c_str(), -1);
        if ( sysPath && PyList_Check(sysPath) && path0 ) {
            PyList_Insert(sysPath, 0, path0);
        }
        Py_XDECREF(path0);
    }

    PyObject* mainModule = PyImport_ImportModule("__main__"); //create main module , new ref

    //See https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    //Python releases the GIL every 100 virtual Python instructions, we do not want that to happen in the middle of an expression.
    // Not recessary since we also have the Natron GIL to control the execution of our own scripts.
    //_PyEval_SetSwitchInterval(std::numeric_limits<long>::max());

    //See answer for http://stackoverflow.com/questions/15470367/pyeval-initthreads-in-python-3-how-when-to-call-it-the-saga-continues-ad-naus
    // Note: on Python >= 3.7 this is already done by Py_Initialize(),
#if PY_VERSION_HEX < 0x03070000
    PyEval_InitThreads();
#endif

    // Follow https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    ///All calls to the Python API should call PythonGILLocker beforehand.
    // Disabled because it seems to crash Natron at launch.
    //_imp->mainThreadState = PyGILState_GetThisThreadState();
    //PyEval_ReleaseThread(_imp->mainThreadState);

    std::string err;
#if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
    /// print info about python lib
    {
        printf( "PATH is %s\n", Py_GETENV("PATH") );
        printf( "PYTHONPATH is %s\n", Py_GETENV("PYTHONPATH") );
        printf( "PYTHONHOME is %s\n", Py_GETENV("PYTHONHOME") );

#define DUMP_SYS(NAME) \
            do { \
                obj = PySys_GetObject(#NAME); \
                PySys_FormatStderr("  sys.%s = ", #NAME); \
                if (obj != NULL) { \
                    PySys_FormatStdout("%A", obj); \
                } \
                else { \
                    PySys_WriteStdout("(not set)"); \
                } \
                PySys_FormatStdout("\n"); \
            } while (0)

        PyObject *obj;
        DUMP_SYS(version);
        DUMP_SYS(flags); // formerly printed from the deprecated Py_*Flag global variables
        DUMP_SYS(_base_executable);
        DUMP_SYS(base_prefix);
        DUMP_SYS(base_exec_prefix);
        DUMP_SYS(platlibdir);
        DUMP_SYS(executable);
        DUMP_SYS(prefix);
        DUMP_SYS(exec_prefix);
#undef DUMP_SYS

        PyObject *sys_path = PySys_GetObject("path");  /* borrowed reference */
        if (sys_path != NULL && PyList_Check(sys_path)) {
            PySys_WriteStdout("  sys.path = [\n");
            Py_ssize_t len = PyList_GET_SIZE(sys_path);
            for (Py_ssize_t i=0; i < len; i++) {
                PyObject *path = PyList_GET_ITEM(sys_path, i);
                PySys_FormatStdout("    %A,\n", path);
            }
            PySys_WriteStdout("  ]\n");
        }

        PyObject* dict = PyModule_GetDict(mainModule);

        PyErr_Clear();

        ///This is faster than PyRun_SimpleString since is doesn't call PyImport_AddModule("__main__")
        std::string script("from distutils.sysconfig import get_python_lib; print('Python library is in ' + get_python_lib())");
        PyObject* v = PyRun_String(script.c_str(), Py_file_input, dict, 0);
        if (v) {
            Py_DECREF(v);
        }
    }
#endif

    // Release the GIL, because PyEval_InitThreads acquires the GIL
    // see https://docs.python.org/3.7/c-api/init.html#c.PyEval_InitThreads
    PyThreadState *_save = PyEval_SaveThread();
    // The lock should be released just before PyFinalize() using:
    // PyEval_RestoreThread(_save);

    return mainModule;
} // initializePython

int
runPythonMain(int argc,
              wchar_t** argv,
              const std::wstring& pythonHome)
{
    // What Py_Main() does, with the Python home (formerly Py_SetPythonHome()) and the user
    // site disabled (formerly Py_NoUserSiteDirectory, which Py_Main() ignored: the
    // PYTHONNOUSERSITE environment variable set by setupPythonEnv() disabled it).
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    // also pre-initializes Python from the command line (-E, -I, -X utf8...), as Py_Main()
    PyStatus status = PyConfig_SetArgv(&config, argc, argv);
    if ( !PyStatus_Exception(status) && !pythonHome.empty() ) {
        status = PyConfig_SetString( &config, &config.home, pythonHome.c_str() );
    }
    if ( !PyStatus_Exception(status) ) {
        config.user_site_directory = 0;
        status = Py_InitializeFromConfig(&config);
    }
    PyConfig_Clear(&config);
    if ( PyStatus_Exception(status) ) {
        if ( PyStatus_IsExit(status) ) {
            return status.exitcode; // e.g. --help or --version
        }
        Py_ExitStatusException(status);
    }

    return Py_RunMain();
}


std::string
PyStringToStdString(PyObject* py_val)
{
    ///Must be locked
    assert( PyThreadState_Get() );
    std::string val;
    PyObject* s = nullptr;
    // The following should work with Python 2 and 3.
    // https://stackoverflow.com/a/38600095
    if (!py_val) {
        val = "(null)";
    } else if( PyUnicode_Check(py_val) ) {  // python3 has unicode, but we convert to bytes
        s = PyUnicode_AsUTF8String(py_val);
    } else if( PyBytes_Check(py_val) ) {  // python2 has bytes already
        s = PyObject_Bytes(py_val);
    } else {
        // Not a string => Error, warning ...
        val = "(not a string)";
    }

    // If succesfully converted to bytes, then convert to C++ string
    if (s) {
        val = std::string( PyBytes_AS_STRING(s) );
        Py_XDECREF(s);
    }

    return val;
}

NATRON_PYTHON_NAMESPACE_EXIT

NATRON_NAMESPACE_EXIT
