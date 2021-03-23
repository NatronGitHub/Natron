/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppManagerPrivate.h"

#include <QtCore/QtGlobal> // for Q_OS_*
#if defined(Q_OS_UNIX)
#include <sys/time.h>     // for getrlimit on linux
#include <sys/resource.h> // for getrlimit
#if defined(__APPLE__)
#include <sys/syslimits.h> // OPEN_MAX
#endif
#endif
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <stdexcept>
#include <sstream> // stringstream

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
GCC_DIAG_OFF(unused-parameter)
#include <boost/serialization/export.hpp>
#include <boost/archive/binary_oarchive.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalServer>
#include <QtNetwork/QLocalSocket>

#include "Global/QtCompat.h" // for removeRecursively
#include "Global/GlobalDefines.h"
#include "Global/GLIncludes.h"
#include "Global/ProcInfo.h"
#include "Global/StrUtils.h"
#include "Global/FStreamsSupport.h"

#include "Engine/CacheSerialization.h"
#include "Engine/CLArgs.h"
#include "Engine/ExistenceCheckThread.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/Image.h"
#include "Engine/OfxHost.h"
#include "Engine/OSGLContext.h"
#include "Engine/ProcessHandler.h" // ProcessInputChannel
#include "Engine/RectDSerialization.h"
#include "Engine/RectISerialization.h"
#include "Engine/StandardPaths.h"


// Don't forget to update glad.h and glad.c as well when updating these
#define NATRON_OPENGL_VERSION_REQUIRED_MAJOR 2
#define NATRON_OPENGL_VERSION_REQUIRED_MINOR 0

BOOST_CLASS_EXPORT(NATRON_NAMESPACE::FrameParams)
BOOST_CLASS_EXPORT(NATRON_NAMESPACE::ImageParams)

NATRON_NAMESPACE_ENTER
AppManagerPrivate::AppManagerPrivate()
    : globalTLS()
    , _appType(AppManager::eAppTypeBackground)
    , _appInstancesMutex()
    , _appInstances()
    , _availableID(0)
    , _topLevelInstanceID(0)
    , _settings()
    , _formats()
    , _plugins()
    , readerPlugins()
    , writerPlugins()
    , ofxHost( new OfxHost() )
    , _knobFactory( new KnobFactory() )
    , _nodeCache()
    , _diskCache()
    , _viewerCache()
    , diskCachesLocationMutex()
    , diskCachesLocation()
    , _backgroundIPC()
    , _loaded(false)
    , _binaryPath()
    , _nodesGlobalMemoryUse(0)
    , errorLogMutex()
    , errorLog()
    , maxCacheFiles(0)
    , currentCacheFilesCount(0)
    , currentCacheFilesCountMutex()
    , idealThreadCount(0)
    , nThreadsToRender(0)
    , nThreadsPerEffect(0)
    , useThreadPool(true)
    , nThreadsMutex()
    , runningThreadsCount()
    , lastProjectLoadedCreatedDuringRC2Or3(false)
    , commandLineArgsUtf8()
    , nArgs(0)
    , mainModule(0)
    , mainThreadState(0)
#ifdef NATRON_USE_BREAKPAD
    , breakpadProcessExecutableFilePath()
    , breakpadProcessPID(0)
    , breakpadHandler()
    , breakpadAliveThread()
#endif
#ifdef USE_NATRON_GIL
    , natronPythonGIL(QMutex::Recursive)
#endif
    , pluginsUseInputImageCopyToRender(false)
    , glRequirements()
    , glHasTextureFloat(false)
    , hasInitializedOpenGLFunctions(false)
    , openGLFunctionsMutex()
    , renderingContextPool()
    , openGLRenderers()
{
    setMaxCacheFiles();

    runningThreadsCount = 0;
}

AppManagerPrivate::~AppManagerPrivate()
{
    

    for (std::size_t i = 0; i < commandLineArgsUtf8Original.size(); ++i) {
        free(commandLineArgsUtf8Original[i]);
    }
#if PY_MAJOR_VERSION >= 3
    // Python 3
    for (std::size_t i = 0; i < commandLineArgsWideOriginal.size(); ++i) {
        free(commandLineArgsWideOriginal[i]);
    }
#endif
}

#ifdef NATRON_USE_BREAKPAD
void
AppManagerPrivate::initBreakpad(const QString& breakpadPipePath,
                                const QString& breakpadComPipePath,
                                int breakpad_client_fd)
{
    assert(!breakpadHandler);
    createBreakpadHandler(breakpadPipePath, breakpad_client_fd);

    /*
       We check periodically that the crash reporter process is still alive. If the user killed it somehow, then we want
       the Natron process to terminate
     */
    breakpadAliveThread = boost::make_shared<ExistenceCheckerThread>(QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK),
                                                                     QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK),
                                                                     breakpadComPipePath);
    QObject::connect( breakpadAliveThread.get(), SIGNAL(otherProcessUnreachable()), appPTR, SLOT(onCrashReporterNoLongerResponding()) );
    breakpadAliveThread->start();
}

void
AppManagerPrivate::createBreakpadHandler(const QString& breakpadPipePath,
                                         int breakpad_client_fd)
{
    QString dumpPath = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);

    Q_UNUSED(breakpad_client_fd);
    try {
#if defined(Q_OS_MAC)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( dumpPath.toStdString(),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL),
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpcb*/,
                                                                                 (void*)NULL,
                                                                                 true,
                                                                                 breakpadPipePath.toStdString().c_str() );
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
        Q_UNUSED(breakpadPipePath);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( google_breakpad::MinidumpDescriptor( dumpPath.toStdString() ),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL),
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpCb*/,
                                                                                 (void*)NULL,
                                                                                 true,
                                                                                 breakpad_client_fd);
#elif defined(Q_OS_WIN32)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler = boost::make_shared<google_breakpad::ExceptionHandler>( dumpPath.toStdWString(),
                                                                                 google_breakpad::ExceptionHandler::FilterCallback(NULL), //filter callback
                                                                                 google_breakpad::ExceptionHandler::MinidumpCallback(NULL) /*dmpcb*/,
                                                                                 (void*)NULL, //context
                                                                                 google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                                 MiniDumpNormal,
                                                                                 breakpadPipePath.toStdWString().c_str(),
                                                                                 (google_breakpad::CustomClientInfo*)NULL);
#endif
    } catch (const std::exception& e) {
        qDebug() << e.what();

        return;
    }
}

#endif // NATRON_USE_BREAKPAD


void
AppManagerPrivate::initProcessInputChannel(const QString & mainProcessServerName)
{
    // scoped_ptr
    _backgroundIPC.reset( new ProcessInputChannel(mainProcessServerName) );
}

void
AppManagerPrivate::loadBuiltinFormats()
{
    // Initializing list of all Formats available by default in a project, the user may add formats to this list via Python in the init.py script, see
    // example here: http://natron.readthedocs.io/en/workshop/startupscripts.html#init-py
    std::vector<std::string> formatNames;
    struct BuiltinFormat
    {
        const char *name;
        int w;
        int h;
        double par;
    };

    // list of formats: the names must match those in ofxsFormatResolution.h
    BuiltinFormat formats[] = {
        { "PC_Video",              640,  480, 1 },
        { "NTSC",                  720,  486, 0.91 },
        { "PAL",                   720,  576, 1.09 },
        { "NTSC_16:9",             720,  486, 1.21 },
        { "PAL_16:9",              720,  576, 1.46 },
        { "HD_720",               1280,  720, 1 },
        { "HD",                   1920, 1080, 1 },
        { "UHD_4K",               3840, 2160, 1 },
        { "1K_Super_35(full-ap)", 1024,  778, 1 },
        { "1K_Cinemascope",        914,  778, 2 },
        { "2K_Super_35(full-ap)", 2048, 1556, 1 },
        { "2K_Cinemascope",       1828, 1556, 2 },
        { "2K_DCP",               2048, 1080, 1 },
        { "4K_Super_35(full-ap)", 4096, 3112, 1 },
        { "4K_Cinemascope",       3656, 3112, 2 },
        { "4K_DCP",               4096, 2160, 1 },
        { "square_256",            256,  256, 1 },
        { "square_512",            512,  512, 1 },
        { "square_1K",            1024, 1024, 1 },
        { "square_2K",            2048, 2048, 1 },
        { NULL, 0, 0, 0 }
    };

    for (BuiltinFormat* f = &formats[0]; f->name != NULL; ++f) {
        Format _frmt(0, 0, f->w, f->h, f->name, f->par);
        _formats.push_back(_frmt);
    }
} // loadBuiltinFormats

//
// only used for Natron internal plugins (ViewerGroup, Dot, DiskCache, BackDrop, Roto)
// see also AppManager::getPluginBinary(), OFX::Host::PluginCache::getPluginById()
//
Plugin*
AppManagerPrivate::findPluginById(const QString& newId,
                                  int major,
                                  int minor) const
{
    for (PluginsMap::const_iterator it = _plugins.begin(); it != _plugins.end(); ++it) {
        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin();
             itver != it->second.rend();
             ++itver) {
            if ( ( (*itver)->getPluginID() == newId ) &&
                 ( (*itver)->getMajorVersion() == major ) &&
                 ( (*itver)->getMinorVersion() == minor ) ) {
                return (*itver);
            }
        }
    }

    return 0;
}

void
AppManagerPrivate::declareSettingsToPython()
{
    std::stringstream ss;

    ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.getSettings()\n";
    const KnobsVec& knobs = _settings->getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        const std::string name = (*it)->getName();
        if ( NATRON_PYTHON_NAMESPACE::isKeyword(name) ) {
            throw std::runtime_error(name + " is a Python keyword");
        }
        ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings." <<
        name << " = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings.getParam('" << name << "')\n";
    }
}

template <typename T>
void
saveCache(Cache<T>* cache)
{
    std::string cacheRestoreFilePath = cache->getRestoreFilePath();
    FStreamsSupport::ofstream ofile;
    FStreamsSupport::open(&ofile, cacheRestoreFilePath);

    if (!ofile) {
        std::cerr << "Failed to save cache to " << cacheRestoreFilePath.c_str() << std::endl;

        return;
    }


    typename Cache<T>::CacheTOC toc;
    cache->save(&toc);
    unsigned int version = cache->cacheVersion();
    try {
        boost::archive::binary_oarchive oArchive(ofile);
        oArchive << version;
        oArchive << toc;
    } catch (const std::exception & e) {
        qDebug() << "Failed to serialize the cache table of contents:" << e.what();
    }
}

void
AppManagerPrivate::saveCaches()
{
    if (!appPTR->isBackground()) {
        saveCache<FrameEntry>( _viewerCache.get() );
    }
    saveCache<Image>( _diskCache.get() );
} // saveCaches

template <typename T>
void
restoreCache(AppManagerPrivate* p,
             Cache<T>* cache)
{
    if ( p->checkForCacheDiskStructure( cache->getCachePath(), cache->isTileCache() ) ) {
        std::string settingsFilePath = cache->getRestoreFilePath();
        FStreamsSupport::ifstream ifile;
        FStreamsSupport::open(&ifile, settingsFilePath);
        if (!ifile) {
            std::cerr << "Failure to open cache restore file at: " << settingsFilePath << std::endl;

            return;
        }
        typename Cache<T>::CacheTOC tableOfContents;
        unsigned int cacheVersion = 0x1; //< default to 1 before NATRON_CACHE_VERSION was introduced
        try {
            boost::archive::binary_iarchive iArchive(ifile);
            if (cache->cacheVersion() >= NATRON_CACHE_VERSION) {
                iArchive >> cacheVersion;
            }
            //Only load caches with same version, otherwise wipe it!
            if ( cacheVersion == cache->cacheVersion() ) {
                iArchive >> tableOfContents;
            } else {
                p->cleanUpCacheDiskStructure( cache->getCachePath(), cache->isTileCache() );
            }
        } catch (const std::exception & e) {
            qDebug() << "Exception when reading disk cache TOC:" << e.what();
            p->cleanUpCacheDiskStructure( cache->getCachePath(), cache->isTileCache() );

            return;
        }

        QFile restoreFile( QString::fromUtf8( settingsFilePath.c_str() ) );
        restoreFile.remove();

        cache->restore(tableOfContents);
    }
}

void
AppManagerPrivate::restoreCaches()
{
    restoreCache<FrameEntry>( this, _viewerCache.get() );
    restoreCache<Image>( this, _diskCache.get() );
} // restoreCaches

bool
AppManagerPrivate::checkForCacheDiskStructure(const QString & cachePath, bool isTiled)
{
    QString settingsFilePath = cachePath;

    if ( !settingsFilePath.endsWith( QChar::fromLatin1('/') ) ) {
        settingsFilePath += QChar::fromLatin1('/');
    }
    settingsFilePath += QString::fromUtf8("restoreFile." NATRON_CACHE_FILE_EXT);

    if ( !QFile::exists(settingsFilePath) ) {
        cleanUpCacheDiskStructure(cachePath, isTiled);

        return false;
    }

    if (!isTiled) {
        QDir directory(cachePath);
        QStringList files = directory.entryList(QDir::AllDirs);


        /*Now counting actual data files in the cache*/
        /*check if there's 256 subfolders, otherwise reset cache.*/
        int count = 0; // -1 because of the restoreFile
        int subFolderCount = 0;
        Q_FOREACH(const QString &file, files) {
            QString subFolder(cachePath);

            subFolder.append( QDir::separator() );
            subFolder.append(file);
            if ( ( subFolder.right(1) == QString::fromUtf8(".") ) || ( subFolder.right(2) == QString::fromUtf8("..") ) ) {
                continue;
            }
            QDir d(subFolder);
            if ( d.exists() ) {
                ++subFolderCount;
                QStringList items = d.entryList();
                for (int j = 0; j < items.size(); ++j) {
                    if ( ( items[j] != QString::fromUtf8(".") ) && ( items[j] != QString::fromUtf8("..") ) ) {
                        ++count;
                    }
                }
            }
        }
        if (subFolderCount < 256) {
            qDebug() << cachePath << "doesn't contain sub-folders indexed from 00 to FF. Resetting.";
            cleanUpCacheDiskStructure(cachePath, isTiled);
            
            return false;
        }
    }
    return true;
} // AppManagerPrivate::checkForCacheDiskStructure

void
AppManagerPrivate::cleanUpCacheDiskStructure(const QString & cachePath, bool isTiled)
{
    /*re-create cache*/

    QDir cacheFolder(cachePath);

#   if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
    QtCompat::removeRecursively(cachePath);
#   else
    if ( cacheFolder.exists() ) {
        cacheFolder.removeRecursively();
    }
#endif
    if ( !cacheFolder.exists() ) {
        bool success = cacheFolder.mkpath( QChar::fromLatin1('.') );
        if (!success) {
            qDebug() << "Warning: cache directory" << cachePath << "could not be created";
        }
    }

    QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);
    // if not 256 subdirs, we re-create the cache
    if (etr.size() < 256) {
        Q_FOREACH (const QString &e, etr) {
            cacheFolder.rmdir(e);
        }
    }
    if (!isTiled) {
        for (U32 i = 0x00; i <= 0xF; ++i) {
            for (U32 j = 0x00; j <= 0xF; ++j) {
                std::ostringstream oss;
                oss << std::hex << i;
                oss << std::hex << j;
                std::string str = oss.str();
                bool success = cacheFolder.mkdir( QString::fromUtf8( str.c_str() ) );
                if (!success) {
                    qDebug() << "Warning: cache directory" << (cachePath.toStdString() + '/' + str).c_str() << "could not be created";
                }
            }
        }
    }
}

void
AppManagerPrivate::setMaxCacheFiles()
{
    /*Default to something reasonable if the code below would happen to not work for some reason*/
    size_t hardMax = NATRON_MAX_CACHE_FILES_OPENED;

#if defined(Q_OS_UNIX) && defined(RLIMIT_NOFILE)
    /*
       Avoid 'Too many open files' on Unix.

       Increase the number of file descriptors that the process can open to the maximum allowed.
       - By default, Mac OS X only allows 256 file descriptors, which can easily be reached.
       - On Linux, the default limit is usually 1024.

        Note that due to a bug in stdio on OS X, the limit on the number of files opened using fopen()
        cannot be changed after the first call to stdio (e.g. printf() or fopen()).
        Consequently, this has to be the first thing to do in main().
     */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        if (rl.rlim_max > rl.rlim_cur) {
            rl.rlim_cur = rl.rlim_max;
            if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
#             if defined(__APPLE__) && defined(OPEN_MAX)
                // On Mac OS X, setrlimit(RLIMIT_NOFILE, &rl) fails to set
                // rlim_cur above OPEN_MAX even if rlim_max > OPEN_MAX.
                if (rl.rlim_cur > OPEN_MAX) {
                    rl.rlim_cur = OPEN_MAX;
                    hardMax = rl.rlim_cur;
                    setrlimit(RLIMIT_NOFILE, &rl);
                }
#             endif
            } else {
                hardMax = rl.rlim_cur;
            }
        }
    }
    //#elif defined(Q_OS_WIN)
    // The following code sets the limit for stdio-based calls only.
    // Note that low-level calls (CreateFile(), WriteFile(), ReadFile(), CloseHandle()...) are not affected by this limit.
    // References:
    // - http://msdn.microsoft.com/en-us/library/6e3b887c.aspx
    // - https://stackoverflow.com/questions/870173/is-there-a-limit-on-number-of-open-files-in-windows/4276338
    // - http://bugs.mysql.com/bug.php?id=24509
    //_setmaxstdio(2048); // sets the limit for stdio-based calls
    // On Windows there seems to be no limit at all. The following test program can prove it:
    /*
       #include <windows.h>
       int
       main(int argc,
         char *argv[])
       {
        const int maxFiles = 10000;

        std::list<HANDLE> files;

        for (int i = 0; i < maxFiles; ++i) {
            std::stringstream ss;
            ss << "C:/Users/Lex/Documents/GitHub/Natron/App/win32/debug/testMaxFiles/file" << i ;
            std::string filename = ss.str();
            HANDLE file_handle = ::CreateFile(filename.c_str(), GENERIC_READ | GENERIC_WRITE,
                                              0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);


            if (file_handle != INVALID_HANDLE_VALUE) {
                files.push_back(file_handle);
                std::cout << "Good files so far: " << files.size() << std::endl;

            } else {
                char* message ;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,GetLastError(),0,(LPSTR)&message,0,NULL);
                std::cout << "Failed to open " << filename << ": " << message << std::endl;
                LocalFree(message);
            }
        }
        std::cout << "Total opened files: " << files.size() << std::endl;
        for (std::list<HANDLE>::iterator it = files.begin(); it != files.end(); ++it) {
            CloseHandle(*it);
        }
       }
     */
#endif

    maxCacheFiles = hardMax * 0.9;
}

#ifdef DEBUG
// logs every gl call to the console
static void
pre_gl_call(const char */*name*/,
            void */*funcptr*/,
            int /*len_args*/,
            ...)
{
#ifdef GL_TRACE_CALLS
    printf("Calling: %s (%d arguments)\n", name, len_args);
#endif
}

// logs every gl call to the console
static void
post_gl_call(const char *name,
             void */*funcptr*/,
             int /*len_args*/,
             ...)
{
#ifdef GL_TRACE_CALLS
    GLenum error_code;
    error_code = glad_glGetError();

    if (error_code != GL_NO_ERROR) {
        fprintf(stderr, "ERROR %d(%s) in %s\n", error_code, gluErrorString(error_code), name);
    }
#endif
}

#endif // debug

void
AppManagerPrivate::initGLAPISpecific()
{
#ifdef Q_OS_WIN32
    // scoped_ptr
    wglInfo.reset(new OSGLContext_wgl_data);
    OSGLContext_win::initWGLData( wglInfo.get() );
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    // scoped_ptr
    glxInfo.reset(new OSGLContext_glx_data);
    OSGLContext_x11::initGLXData( glxInfo.get() );

#endif // Q_OS_WIN32
}

extern "C" {
extern int GLAD_GL_ARB_vertex_buffer_object;
extern int GLAD_GL_ARB_framebuffer_object;
extern int GLAD_GL_ARB_pixel_buffer_object;
extern int GLAD_GL_ARB_vertex_array_object;
extern int GLAD_GL_ARB_texture_float;
extern int GLAD_GL_EXT_framebuffer_object;
extern int GLAD_GL_APPLE_vertex_array_object;

typedef GLboolean (APIENTRYP PFNGLISRENDERBUFFEREXTPROC)(GLuint renderbuffer);
extern PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbufferEXT;
extern PFNGLISRENDERBUFFEREXTPROC glad_glIsRenderbuffer;

typedef void (APIENTRYP PFNGLBINDRENDERBUFFEREXTPROC)(GLenum target, GLuint renderbuffer);
extern PFNGLBINDRENDERBUFFEREXTPROC glad_glBindRenderbufferEXT;
extern PFNGLBINDRENDERBUFFERPROC glad_glBindRenderbuffer;

typedef void (APIENTRYP PFNGLDELETERENDERBUFFERSEXTPROC)(GLsizei n, const GLuint* renderbuffers);
extern PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffersEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glad_glDeleteRenderbuffers;


typedef void (APIENTRYP PFNGLGENRENDERBUFFERSEXTPROC)(GLsizei n, GLuint* renderbuffers);
extern PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glad_glGenRenderbuffers;
    
typedef void (APIENTRYP PFNGLRENDERBUFFERSTORAGEEXTPROC)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorageEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glad_glRenderbufferStorage;

typedef void (APIENTRYP PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)(GLenum target, GLenum pname, GLint* params);
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameterivEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glad_glGetRenderbufferParameteriv;


typedef void (APIENTRYP PFNGLBINDFRAMEBUFFEREXTPROC)(GLenum target, GLuint framebuffer);
extern PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glad_glBindFramebuffer;


typedef GLboolean (APIENTRYP PFNGLISFRAMEBUFFEREXTPROC)(GLuint framebuffer);
extern PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebufferEXT;
extern PFNGLISFRAMEBUFFEREXTPROC glad_glIsFramebuffer;


typedef void (APIENTRYP PFNGLDELETEFRAMEBUFFERSEXTPROC)(GLsizei n, const GLuint* framebuffers);
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffersEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glad_glDeleteFramebuffers;


typedef void (APIENTRYP PFNGLGENFRAMEBUFFERSEXTPROC)(GLsizei n, GLuint* framebuffers);
extern PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glad_glGenFramebuffers;


typedef GLenum (APIENTRYP PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)(GLenum target);
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatusEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glad_glCheckFramebufferStatus;


typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glad_glFramebufferTexture1D;


typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glad_glFramebufferTexture2D;


typedef void (APIENTRYP PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glad_glFramebufferTexture3D;


typedef void (APIENTRYP PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbufferEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glad_glFramebufferRenderbuffer;


typedef void (APIENTRYP PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)(GLenum target, GLenum attachment, GLenum pname, GLint* params);
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameterivEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glad_glGetFramebufferAttachmentParameteriv;

typedef void (APIENTRYP PFNGLGENERATEMIPMAPEXTPROC)(GLenum target);
extern PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmapEXT;
extern PFNGLGENERATEMIPMAPEXTPROC glad_glGenerateMipmap;

typedef void (APIENTRYP PFNGLBINDVERTEXARRAYAPPLEPROC)(GLuint array);
extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_glBindVertexArrayAPPLE;
extern PFNGLBINDVERTEXARRAYAPPLEPROC glad_glBindVertexArray;

typedef void (APIENTRYP PFNGLDELETEVERTEXARRAYSAPPLEPROC)(GLsizei n, const GLuint* arrays);
extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_glDeleteVertexArraysAPPLE;
extern PFNGLDELETEVERTEXARRAYSAPPLEPROC glad_glDeleteVertexArrays;

typedef void (APIENTRYP PFNGLGENVERTEXARRAYSAPPLEPROC)(GLsizei n, GLuint* arrays);
extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_glGenVertexArraysAPPLE;
extern PFNGLGENVERTEXARRAYSAPPLEPROC glad_glGenVertexArrays;

typedef GLboolean (APIENTRYP PFNGLISVERTEXARRAYAPPLEPROC)(GLuint array);
extern PFNGLISVERTEXARRAYAPPLEPROC glad_glIsVertexArrayAPPLE;
extern PFNGLISVERTEXARRAYAPPLEPROC glad_glIsVertexArray;

}

void
AppManagerPrivate::addOpenGLRequirementsString(QString& str, OpenGLRequirementsTypeEnum type)
{
    switch (type) {
        case eOpenGLRequirementsTypeViewer: {
            str += QLatin1Char('\n');
            str += tr("%1 requires at least OpenGL %2.%3 with the following extensions so the viewer works appropriately:").arg(QLatin1String(NATRON_APPLICATION_NAME)).arg(NATRON_OPENGL_VERSION_REQUIRED_MAJOR).arg(NATRON_OPENGL_VERSION_REQUIRED_MINOR);
            str += QLatin1Char('\n');
            str += QString::fromUtf8("GL_ARB_vertex_buffer_object,GL_ARB_pixel_buffer_object");
            str += QLatin1Char('\n');
#ifdef __NATRON_WIN32__
            str += tr("To fix this: Re-start the installer, select \"Package manager\" and then install the \"Software OpenGL\" component.\n");
            str += tr("If you don't have the installer you can manually copy opengl32.dll located in the \"bin\\mesa\" directory of your %1 installation to the \"bin\" directory.").arg(QLatin1String(NATRON_APPLICATION_NAME));
#endif
            break;
        }
        case eOpenGLRequirementsTypeRendering: {
            str += QLatin1Char('\n');
            str += tr("%1 requires at least OpenGL %2.%3 with the following extensions to perform OpenGL rendering:").arg(QLatin1String(NATRON_APPLICATION_NAME)).arg(NATRON_OPENGL_VERSION_REQUIRED_MAJOR).arg(NATRON_OPENGL_VERSION_REQUIRED_MINOR);
            str += QLatin1Char('\n');
            str += QString::fromUtf8("GL_ARB_vertex_buffer_object <br /> GL_ARB_pixel_buffer_object <br /> GL_ARB_vertex_array_object or GL_APPLE_vertex_array_object <br /> GL_ARB_framebuffer_object or GL_EXT_framebuffer_object <br /> GL_ARB_texture_float");
            str += QLatin1Char('\n');
            break;
        }
    }
    std::list<OpenGLRendererInfo> openGLRenderers;
    OSGLContext::getGPUInfos(openGLRenderers);
    if ( !openGLRenderers.empty() ) {
        str += QLatin1Char('\n');
        str += tr("Available OpenGL renderers:");
        str += QLatin1Char('\n');
        for (std::list<OpenGLRendererInfo>::iterator it = openGLRenderers.begin(); it != openGLRenderers.end(); ++it) {
            str += (tr("Vendor:") + QString::fromUtf8(" ") + QString::fromUtf8( it->vendorName.c_str() ) +
                    QLatin1Char('\n') +
                    tr("Renderer:") + QString::fromUtf8(" ") + QString::fromUtf8( it->rendererName.c_str() ) +
                    QLatin1Char('\n') +
                    tr("OpenGL Version:") + QString::fromUtf8(" ") + QString::fromUtf8( it->glVersionString.c_str() ) +
                    QLatin1Char('\n') +
                    tr("GLSL Version:") + QString::fromUtf8(" ") + QString::fromUtf8( it->glslVersionString.c_str() ) );
            str += QLatin1Char('\n');
            str += QLatin1Char('\n');
        }
    }
}

void
AppManagerPrivate::initGl(bool checkRenderingReq)
{
    // Private should not lock
    assert( !openGLFunctionsMutex.tryLock() );

    hasInitializedOpenGLFunctions = true;

    bool glLoaded = gladLoadGL();

#ifdef GLAD_DEBUG
    if (glLoaded) {
        glad_set_pre_callback(pre_gl_call);
        glad_set_post_callback(post_gl_call);

        // disablepost_gl_call for glBegin and commands that are authorized between glBegin and glEnd
        // (because glGetError isn't authorized there)
        // see https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glBegin.xml

        glad_debug_glBegin = glad_glBegin;

        glad_debug_glVertex2s = glad_glVertex2s;
        glad_debug_glVertex2i = glad_glVertex2i;
        glad_debug_glVertex2f = glad_glVertex2f;
        glad_debug_glVertex2d = glad_glVertex2d;
        glad_debug_glVertex3s = glad_glVertex3s;
        glad_debug_glVertex3i = glad_glVertex3i;
        glad_debug_glVertex3f = glad_glVertex3f;
        glad_debug_glVertex3d = glad_glVertex3d;
        glad_debug_glVertex4s = glad_glVertex4s;
        glad_debug_glVertex4i = glad_glVertex4i;
        glad_debug_glVertex4f = glad_glVertex4f;
        glad_debug_glVertex4d = glad_glVertex4d;
        glad_debug_glColor3b = glad_glColor3b;
        glad_debug_glColor3s = glad_glColor3s;
        glad_debug_glColor3i = glad_glColor3i;
        glad_debug_glColor3f = glad_glColor3f;
        glad_debug_glColor3d = glad_glColor3d;
        glad_debug_glColor3ub = glad_glColor3ub;
        glad_debug_glColor3us = glad_glColor3us;
        glad_debug_glColor3ui = glad_glColor3ui;
        glad_debug_glColor4b = glad_glColor4b;
        glad_debug_glColor4s = glad_glColor4s;
        glad_debug_glColor4i = glad_glColor4i;
        glad_debug_glColor4f = glad_glColor4f;
        glad_debug_glColor4d = glad_glColor4d;
        glad_debug_glColor4ub = glad_glColor4ub;
        glad_debug_glColor4us = glad_glColor4us;
        glad_debug_glColor4ui = glad_glColor4ui;
        glad_debug_glSecondaryColor3b = glad_glSecondaryColor3b;
        glad_debug_glSecondaryColor3s = glad_glSecondaryColor3s;
        glad_debug_glSecondaryColor3i = glad_glSecondaryColor3i;
        glad_debug_glSecondaryColor3f = glad_glSecondaryColor3f;
        glad_debug_glSecondaryColor3d = glad_glSecondaryColor3d;
        glad_debug_glSecondaryColor3ub = glad_glSecondaryColor3ub;
        glad_debug_glSecondaryColor3us = glad_glSecondaryColor3us;
        glad_debug_glSecondaryColor3ui = glad_glSecondaryColor3ui;
        glad_debug_glIndexs = glad_glIndexs;
        glad_debug_glIndexi = glad_glIndexi;
        glad_debug_glIndexf = glad_glIndexf;
        glad_debug_glIndexd = glad_glIndexd;
        glad_debug_glIndexub = glad_glIndexub;
        glad_debug_glNormal3b = glad_glNormal3b;
        glad_debug_glNormal3d = glad_glNormal3d;
        glad_debug_glNormal3f = glad_glNormal3f;
        glad_debug_glNormal3i = glad_glNormal3i;
        glad_debug_glNormal3s = glad_glNormal3s;
        glad_debug_glFogCoorddv = glad_glFogCoorddv;
        glad_debug_glFogCoordfv = glad_glFogCoordfv;
        glad_debug_glTexCoord1s = glad_glTexCoord1s;
        glad_debug_glTexCoord1i = glad_glTexCoord1i;
        glad_debug_glTexCoord1f = glad_glTexCoord1f;
        glad_debug_glTexCoord1d = glad_glTexCoord1d;
        glad_debug_glTexCoord2s = glad_glTexCoord2s;
        glad_debug_glTexCoord2i = glad_glTexCoord2i;
        glad_debug_glTexCoord2f = glad_glTexCoord2f;
        glad_debug_glTexCoord2d = glad_glTexCoord2d;
        glad_debug_glTexCoord3s = glad_glTexCoord3s;
        glad_debug_glTexCoord3i = glad_glTexCoord3i;
        glad_debug_glTexCoord3f = glad_glTexCoord3f;
        glad_debug_glTexCoord3d = glad_glTexCoord3d;
        glad_debug_glTexCoord4s = glad_glTexCoord4s;
        glad_debug_glTexCoord4i = glad_glTexCoord4i;
        glad_debug_glTexCoord4f = glad_glTexCoord4f;
        glad_debug_glTexCoord4d = glad_glTexCoord4d;
        glad_debug_glMultiTexCoord1s = glad_glMultiTexCoord1s;
        glad_debug_glMultiTexCoord1i = glad_glMultiTexCoord1i;
        glad_debug_glMultiTexCoord1f = glad_glMultiTexCoord1f;
        glad_debug_glMultiTexCoord1d = glad_glMultiTexCoord1d;
        glad_debug_glMultiTexCoord2s = glad_glMultiTexCoord2s;
        glad_debug_glMultiTexCoord2i = glad_glMultiTexCoord2i;
        glad_debug_glMultiTexCoord2f = glad_glMultiTexCoord2f;
        glad_debug_glMultiTexCoord2d = glad_glMultiTexCoord2d;
        glad_debug_glMultiTexCoord3s = glad_glMultiTexCoord3s;
        glad_debug_glMultiTexCoord3i = glad_glMultiTexCoord3i;
        glad_debug_glMultiTexCoord3f = glad_glMultiTexCoord3f;
        glad_debug_glMultiTexCoord3d = glad_glMultiTexCoord3d;
        glad_debug_glMultiTexCoord4s = glad_glMultiTexCoord4s;
        glad_debug_glMultiTexCoord4i = glad_glMultiTexCoord4i;
        glad_debug_glMultiTexCoord4f = glad_glMultiTexCoord4f;
        glad_debug_glMultiTexCoord4d = glad_glMultiTexCoord4d;
        glad_debug_glVertexAttrib1f = glad_glVertexAttrib1f;
        glad_debug_glVertexAttrib1s = glad_glVertexAttrib1s;
        glad_debug_glVertexAttrib1d = glad_glVertexAttrib1d;
        glad_debug_glVertexAttrib2f = glad_glVertexAttrib2f;
        glad_debug_glVertexAttrib2s = glad_glVertexAttrib2s;
        glad_debug_glVertexAttrib2d = glad_glVertexAttrib2d;
        glad_debug_glVertexAttrib3f = glad_glVertexAttrib3f;
        glad_debug_glVertexAttrib3s = glad_glVertexAttrib3s;
        glad_debug_glVertexAttrib3d = glad_glVertexAttrib3d;
        glad_debug_glVertexAttrib4f = glad_glVertexAttrib4f;
        glad_debug_glVertexAttrib4s = glad_glVertexAttrib4s;
        glad_debug_glVertexAttrib4d = glad_glVertexAttrib4d;
        glad_debug_glVertexAttrib4Nub = glad_glVertexAttrib4Nub;
        glad_debug_glEvalCoord1f = glad_glEvalCoord1f;
        glad_debug_glEvalCoord1d = glad_glEvalCoord1d;
        glad_debug_glEvalCoord2f = glad_glEvalCoord2f;
        glad_debug_glEvalCoord2d = glad_glEvalCoord2d;
        glad_debug_glEvalPoint1 = glad_glEvalPoint1;
        glad_debug_glEvalPoint2 = glad_glEvalPoint2;
        glad_debug_glArrayElement = glad_glArrayElement;
        glad_debug_glMaterialf = glad_glMaterialf;
        glad_debug_glMateriali = glad_glMateriali;
        glad_debug_glEdgeFlag = glad_glEdgeFlag;

        glad_debug_glCallList = glad_glCallList;
        glad_debug_glCallLists = glad_glCallLists;
    }
#endif

    // If only EXT_framebuffer is present and not ARB link functions
    if (glLoaded && GLAD_GL_EXT_framebuffer_object && !GLAD_GL_ARB_framebuffer_object) {
        glad_glIsRenderbuffer = glad_glIsRenderbufferEXT;
        glad_glBindRenderbuffer = glad_glBindRenderbufferEXT;
        glad_glDeleteRenderbuffers = glad_glDeleteRenderbuffersEXT;
        glad_glGenRenderbuffers = glad_glGenRenderbuffersEXT;
        glad_glRenderbufferStorage = glad_glRenderbufferStorageEXT;
        glad_glGetRenderbufferParameteriv = glad_glGetRenderbufferParameterivEXT;
        glad_glBindFramebuffer = glad_glBindFramebufferEXT;
        glad_glIsFramebuffer = glad_glIsFramebufferEXT;
        glad_glDeleteFramebuffers = glad_glDeleteFramebuffersEXT;
        glad_glGenFramebuffers = glad_glGenFramebuffersEXT;
        glad_glCheckFramebufferStatus = glad_glCheckFramebufferStatusEXT;
        glad_glFramebufferTexture1D = glad_glFramebufferTexture1DEXT;
        glad_glFramebufferTexture2D = glad_glFramebufferTexture2DEXT;
        glad_glFramebufferTexture3D = glad_glFramebufferTexture3DEXT;
        glad_glFramebufferRenderbuffer = glad_glFramebufferRenderbufferEXT;
        glad_glGetFramebufferAttachmentParameteriv = glad_glGetFramebufferAttachmentParameterivEXT;
        glad_glGenerateMipmap = glad_glGenerateMipmapEXT;
    }

    if (glLoaded && GLAD_GL_APPLE_vertex_array_object && !GLAD_GL_ARB_vertex_buffer_object) {
        glad_glBindVertexArray = glad_glBindVertexArrayAPPLE;
        glad_glDeleteVertexArrays = glad_glDeleteVertexArraysAPPLE;
        glad_glGenVertexArrays = glad_glGenVertexArraysAPPLE;
        glad_glIsVertexArray = glad_glIsVertexArrayAPPLE;
    }

    OpenGLRequirementsData& viewerReq = glRequirements[eOpenGLRequirementsTypeViewer];
    viewerReq.hasRequirements = true;

    OpenGLRequirementsData& renderingReq = glRequirements[eOpenGLRequirementsTypeRendering];
    if (checkRenderingReq) {
        renderingReq.hasRequirements = true;
    }


    if (!GLAD_GL_ARB_texture_float) {
        glHasTextureFloat = false;
    } else {
        glHasTextureFloat = true;
    }


    if ( !glLoaded ||
        GLVersion.major < NATRON_OPENGL_VERSION_REQUIRED_MAJOR ||
        (GLVersion.major == NATRON_OPENGL_VERSION_REQUIRED_MAJOR && GLVersion.minor < NATRON_OPENGL_VERSION_REQUIRED_MINOR) ||
        !GLAD_GL_ARB_pixel_buffer_object ||
        !GLAD_GL_ARB_vertex_buffer_object) {

        viewerReq.error = tr("Failed to load OpenGL.");
        addOpenGLRequirementsString(viewerReq.error, eOpenGLRequirementsTypeViewer);
        viewerReq.hasRequirements = false;
        renderingReq.hasRequirements = false;
#ifdef DEBUG
        std::cerr << viewerReq.error.toStdString() << std::endl;
#endif

        return;
    }

    if (checkRenderingReq) {
        if (!GLAD_GL_ARB_texture_float ||
            (!GLAD_GL_ARB_framebuffer_object && !GLAD_GL_EXT_framebuffer_object) ||
            (!GLAD_GL_ARB_vertex_array_object && !GLAD_GL_APPLE_vertex_array_object))
        {
            renderingReq.error += QLatin1String("<p>");
            renderingReq.error += tr("Failed to load OpenGL.");
            addOpenGLRequirementsString(renderingReq.error, eOpenGLRequirementsTypeRendering);


            renderingReq.hasRequirements = false;
        } else {
            renderingReq.hasRequirements = true;
        }
    }
    // OpenGL is now read to be used! just include "Global/GLIncludes.h"
}

void
AppManagerPrivate::tearDownGL()
{
    // Kill all rendering context
    renderingContextPool.reset();

#ifdef Q_OS_WIN32
    if (wglInfo) {
        OSGLContext_win::destroyWGLData( wglInfo.get() );
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    if (glxInfo) {
        OSGLContext_x11::destroyGLXData( glxInfo.get() );
    }
#endif
}

void
AppManagerPrivate::copyUtf8ArgsToMembers(const std::vector<std::string>& utf8Args)
{
    // Copy command line args to local members that live throughout the lifetime of AppManager
#if PY_MAJOR_VERSION >= 3
    // Python 3
    commandLineArgsWideOriginal.resize(utf8Args.size());
#endif
    commandLineArgsUtf8Original.resize(utf8Args.size());
    nArgs = (int)utf8Args.size();
    for (std::size_t i = 0; i < utf8Args.size(); ++i) {
        commandLineArgsUtf8Original[i] = strdup(utf8Args[i].c_str());

        // Python 3 needs wchar_t arguments
#if PY_MAJOR_VERSION >= 3
        // Python 3
        commandLineArgsWideOriginal[i] = char2wchar(utf8Args[i].c_str());
#endif
    }
    commandLineArgsUtf8 = commandLineArgsUtf8Original;
#if PY_MAJOR_VERSION >= 3
    // Python 3
    commandLineArgsWide = commandLineArgsWideOriginal;
#endif
}

void
AppManagerPrivate::handleCommandLineArgs(int argc, char** argv)
{
    // Ensure the arguments are Utf-8 encoded
    std::vector<std::string> utf8Args;
    if (argv) {
        CLArgs::ensureCommandLineArgsUtf8(argc, argv, &utf8Args);
    } else {
        // If the user didn't specify launch arguments (e.g unit testing),
        // At least append the binary path
        std::string path = ProcInfo::applicationDirPath(0);
        utf8Args.push_back(path);
    }

    copyUtf8ArgsToMembers(utf8Args);
}

void
AppManagerPrivate::handleCommandLineArgsW(int argc, wchar_t** argv)
{
    std::vector<std::string> utf8Args;
    if (argv) {
        for (int i = 0; i < argc; ++i) {
            std::wstring wide(argv[i]);
            std::string str = StrUtils::utf16_to_utf8(wide);
            assert(StrUtils::is_utf8(str.c_str()));
            utf8Args.push_back(str);
        }
    } else {
        // If the user didn't specify launch arguments (e.g unit testing),
        // At least append the binary path
        std::string path = ProcInfo::applicationDirPath(0);
        utf8Args.push_back(path);
    }

    copyUtf8ArgsToMembers(utf8Args);
}

NATRON_NAMESPACE_EXIT
