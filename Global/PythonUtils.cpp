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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#undef Py_LIMITED_API  // Needed for Py_NoUserSiteDirectory, PyBytes_AS_STRING
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PythonUtils.h"

#include <cstdlib>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

void setupPythonEnv(const std::string& binPath)
{
    //Disable user sites as they could conflict with Natron bundled packages.
    //If this is set, Python won’t add the user site-packages directory to sys.path.
    //See https://www.python.org/dev/peps/pep-0370/
    ProcInfo::putenv_wrapper("PYTHONNOUSERSITE", "1");
    ++Py_NoUserSiteDirectory;

    //
    // set up paths, clear those that don't exist or are not valid
    //
#ifdef __NATRON_WIN32__
    static std::string pythonHome = binPath + "\\.."; // must use static storage
    static const std::wstring pythonHomeW = StrUtils::utf8_to_utf16(pythonHome);
    std::string pyPathZip = pythonHome + "\\lib\\python" NATRON_PY_VERSION_STRING_NO_DOT ".zip";
    std::string pyPath = pythonHome +  "\\lib\\python" NATRON_PY_VERSION_STRING;
    std::string pyPathDynLoad = pyPath + "\\lib-dynload";
    std::string pyPathSitePackages = pyPath + "\\site-packages";
    std::string pluginPath = binPath + "\\..\\Plugins";
#else
#  if defined(__NATRON_LINUX__)
    static std::string pythonHome = binPath + "/.."; // must use static storage
#  elif defined(__NATRON_OSX__)
    static std::string pythonHome = binPath+ "/../Frameworks/Python.framework/Versions/" NATRON_PY_VERSION_STRING; // must use static storage
#  else
#    error "unsupported platform"
#  endif
    static const std::wstring pythonHomeW = StrUtils::utf8_to_utf16(pythonHome);
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
    // Py_SetPythonHome
    /////////////////////////////////////////
    //
    // Must be done before Py_Initialize (see doc of Py_Initialize)
    //
    // The argument should point to a zero-terminated character string in static storage whose contents will not change for the duration of the program’s execution

    if ( !pythonHome.empty() ) {
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "Py_SetPythonHome(\"%s\")\n", pythonHome.c_str() );
#     endif
#     if PY_MAJOR_VERSION >= 3
        // Python 3
        Py_SetPythonHome( const_cast<wchar_t*>( pythonHomeW.c_str() ) );
#     else
        // Python 2
        Py_SetPythonHome( const_cast<char*>( pythonHome.c_str() ) );
#     endif
    }


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
#     if 0 // PY_MAJOR_VERSION >= 3 // commented for the reason above
        std::wstring pythonPathString = StrUtils::utf8_to_utf16(pythonPath);
        Py_SetPath( pythonPathString.c_str() ); // argument is copied internally, no need to use static storage
#     else
#      if 0//def __NATRON_WIN32__
        // qputenv on mingw will just call putenv, but we want to keep the utf16 info, so we need to call _wputenv
        _wputenv_s(L"PYTHONPATH", StrUtils::utf8_to_utf16(pythonPath).c_str());
#      else
        ProcInfo::putenv_wrapper( "PYTHONPATH", pythonPath.c_str() );
        //Py_SetPath( pythonPathString.c_str() ); // does not exist in Python 2
#      endif
#     endif
#     if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
        printf( "PYTHONPATH set to %s\n", pythonPath.c_str() );
#     endif
    }

} // setupPythonEnv

#if PY_MAJOR_VERSION >= 3
PyObject* initializePython3(const std::vector<wchar_t*>& commandLineArgsWide)
#else
PyObject* initializePython2(const std::vector<char*>& commandLineArgsUtf8)
#endif
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
    // Py_SetProgramName
    /////////////////////////////////////////
    //
    // Must be done before Py_Initialize (see doc of Py_Initialize)
    //

#if PY_MAJOR_VERSION >= 3
    // Python 3
    Py_SetProgramName(commandLineArgsWide[0]);
#else
    // Python 2
    printf( "Py_SetProgramName(\"%s\")\n", commandLineArgsUtf8[0] );
    Py_SetProgramName(commandLineArgsUtf8[0]);
#endif

    /////////////////////////////////////////
    // Py_Initialize
    /////////////////////////////////////////
    //
    // Initialize the Python interpreter. In an application embedding Python, this should be called before using any other Python/C API functions; with the exception of Py_SetProgramName(), Py_SetPythonHome() and Py_SetPath().
#if defined(NATRON_CONFIG_SNAPSHOT) || defined(DEBUG)
    printf("Py_Initialize()\n");
#endif
    Py_Initialize();
    // pythonHome must be const, so that the c_str() pointer is never invalidated

#if PY_MAJOR_VERSION >= 3
    // Py_SetPath clears sys.prefix and sys.exec_prefix
    // https://github.com/NatronGitHub/Natron/issues/696
    PyObject *prefix = PyUnicode_FromWideChar(Py_GetPythonHome(), -1);
    PySys_SetObject(const_cast<char*>("prefix"), prefix);
    Py_XDECREF(prefix);
    PyObject *exec_prefix = PyUnicode_FromWideChar(Py_GetPythonHome(), -1);
    PySys_SetObject(const_cast<char*>("exec_prefix"), exec_prefix);
    Py_XDECREF(exec_prefix);
#endif

    /////////////////////////////////////////
    // PySys_SetArgv
    /////////////////////////////////////////
    //
#if PY_MAJOR_VERSION >= 3
    // Python 3
    PySys_SetArgv( commandLineArgsWide.size(), const_cast<wchar_t**>(&commandLineArgsWide[0]) ); /// relative module import
#else
    // Python 2
    PySys_SetArgv( commandLineArgsUtf8.size(), const_cast<char**>(&commandLineArgsUtf8[0]) ); /// relative module import
#endif

    PyObject* mainModule = PyImport_ImportModule("__main__"); //create main module , new ref

    //See https://web.archive.org/web/20150918224620/http://wiki.blender.org/index.php/Dev:2.4/Source/Python/API/Threads
    //Python releases the GIL every 100 virtual Python instructions, we do not want that to happen in the middle of an expression.
    // Not recessary since we also have the Natron GIL to control the execution of our own scripts.
    //_PyEval_SetSwitchInterval(LONG_MAX);

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
        printf( "Py_DebugFlag is %d\n", Py_DebugFlag );
        printf( "Py_VerboseFlag is %d\n", Py_VerboseFlag );
        printf( "Py_InteractiveFlag is %d\n", Py_InteractiveFlag );
        printf( "Py_InspectFlag is %d\n", Py_InspectFlag );
        printf( "Py_OptimizeFlag is %d\n", Py_OptimizeFlag );
        printf( "Py_NoSiteFlag is %d\n", Py_NoSiteFlag );
        printf( "Py_BytesWarningFlag is %d\n", Py_BytesWarningFlag );
#if PY_MAJOR_VERSION < 3
        printf( "Py_UseClassExceptionsFlag is %d\n", Py_UseClassExceptionsFlag );
#endif
        printf( "Py_FrozenFlag is %d\n", Py_FrozenFlag );
#if PY_MAJOR_VERSION < 3
        printf( "Py_TabcheckFlag is %d\n", Py_TabcheckFlag );
        printf( "Py_UnicodeFlag is %d\n", Py_UnicodeFlag );
#else
        printf( "Py_HashRandomizationFlag is %d\n", Py_HashRandomizationFlag );
        printf( "Py_IsolatedFlag is %d\n", Py_IsolatedFlag );
        printf( "Py_QuietFlag is %d\n", Py_QuietFlag );
#endif
        printf( "Py_IgnoreEnvironmentFlag is %d\n", Py_IgnoreEnvironmentFlag );
#if PY_MAJOR_VERSION < 3
        printf( "Py_DivisionWarningFlag is %d\n", Py_DivisionWarningFlag );
#endif
        printf( "Py_DontWriteBytecodeFlag is %d\n", Py_DontWriteBytecodeFlag );
        printf( "Py_NoUserSiteDirectory is %d\n", Py_NoUserSiteDirectory );
#if PY_MAJOR_VERSION < 3
        printf( "Py_GetProgramName is %s\n", Py_GetProgramName() );
        printf( "Py_GetPrefix is %s\n", Py_GetPrefix() );
        printf( "Py_GetExecPrefix is %s\n", Py_GetPrefix() );
        printf( "Py_GetProgramFullPath is %s\n", Py_GetProgramFullPath() );
        printf( "Py_GetPath is %s\n", Py_GetPath() );
        printf( "Py_GetPythonHome is %s\n", Py_GetPythonHome() );
#else // PY_MAJOR_VERSION >= 3
        printf( "Py_GetProgramName is %ls\n", Py_GetProgramName() );
        printf( "Py_GetPrefix is %ls\n", Py_GetPrefix() );
        printf( "Py_GetExecPrefix is %ls\n", Py_GetPrefix() );
        printf( "Py_GetProgramFullPath is %ls\n", Py_GetProgramFullPath() );
        printf( "Py_GetPath is %ls\n", Py_GetPath() );
        printf( "Py_GetPythonHome is %ls\n", Py_GetPythonHome() );

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
#endif // PY_MAJOR_VERSION >= 3
    }
#endif

    // Release the GIL, because PyEval_InitThreads acquires the GIL
    // see https://docs.python.org/3.7/c-api/init.html#c.PyEval_InitThreads
    PyThreadState *_save = PyEval_SaveThread();
    // The lock should be released just before PyFinalize() using:
    // PyEval_RestoreThread(_save);

    return mainModule;
} // initializePython


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
