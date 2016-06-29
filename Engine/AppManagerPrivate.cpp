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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "AppManagerPrivate.h"

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

#include "Engine/FStreamsSupport.h"
#include "Engine/CacheSerialization.h"
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


BOOST_CLASS_EXPORT(NATRON_NAMESPACE::FrameParams)
BOOST_CLASS_EXPORT(NATRON_NAMESPACE::ImageParams)

NATRON_NAMESPACE_ENTER;
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
    , args()
    , mainModule(0)
    , mainThreadState(0)
#ifdef NATRON_USE_BREAKPAD
    , breakpadProcessExecutableFilePath()
    , breakpadProcessPID(0)
    , breakpadHandler()
    , breakpadAliveThread()
#endif
    , natronPythonGIL(QMutex::Recursive)
    , pluginsUseInputImageCopyToRender(false)
    , hasRequiredOpenGLVersionAndExtensions(true)
    , missingOpenglError()
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
    for (U32 i = 0; i < args.size(); ++i) {
        free(args[i]);
    }
    args.clear();
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
    breakpadAliveThread.reset( new ExistenceCheckerThread(QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK),
                                                          QString::fromUtf8(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK),
                                                          breakpadComPipePath) );
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
        breakpadHandler.reset( new google_breakpad::ExceptionHandler( dumpPath.toStdString(),
                                                                      0,
                                                                      0 /*dmpcb*/,
                                                                      0,
                                                                      true,
                                                                      breakpadPipePath.toStdString().c_str() ) );
#elif defined(Q_OS_LINUX)
        Q_UNUSED(breakpadPipePath);
        breakpadHandler.reset( new google_breakpad::ExceptionHandler( google_breakpad::MinidumpDescriptor( dumpPath.toStdString() ),
                                                                      0,
                                                                      0 /*dmpCb*/,
                                                                      0,
                                                                      true,
                                                                      breakpad_client_fd) );
#elif defined(Q_OS_WIN32)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler.reset( new google_breakpad::ExceptionHandler( dumpPath.toStdWString(),
                                                                      0, //filter callback
                                                                      0 /*dmpcb*/,
                                                                      0, //context
                                                                      google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                      MiniDumpNormal,
                                                                      breakpadPipePath.toStdWString().c_str(),
                                                                      0) );
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

    BuiltinFormat formats[] = {
        { "PC_Video",              640,  480, 1 },
        { "NTSC",                  720,  486, 0.91 },
        { "PAL",                   720,  576, 1.09 },
        { "HD",                   1920, 1080, 1 },
        { "NTSC_16:9",             720,  486, 1.21 },
        { "PAL_16:9",              720,  576, 1.46 },
        { "1K_Super_35(full-ap)", 1024,  778, 1 },
        { "1K_Cinemascope",        914,  778, 2 },
        { "2K_Super_35(full-ap)", 2048, 1556, 1 },
        { "2K_Cinemascope",       1828, 1556, 2 },
        { "4K_Super_35(full-ap)", 4096, 3112, 1 },
        { "4K_Cinemascope",       3656, 3112, 2 },
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

Plugin*
AppManagerPrivate::findPluginById(const QString& newId,
                                  int major,
                                  int minor) const
{
    for (PluginsMap::const_iterator it = _plugins.begin(); it != _plugins.end(); ++it) {
        for (PluginMajorsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ( ( (*it2)->getPluginID() == newId ) && ( (*it2)->getMajorVersion() == major ) && ( (*it2)->getMinorVersion() == minor ) ) {
                return (*it2);
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
        ss <<  NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings." <<
        (*it)->getName() << " = " << NATRON_ENGINE_PYTHON_MODULE_NAME << ".natron.settings.getParam('" << (*it)->getName() << "')\n";
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
    saveCache<FrameEntry>( _viewerCache.get() );
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
    if ( !appPTR->isBackground() ) {
        restoreCache<FrameEntry>( this, _viewerCache.get() );
        restoreCache<Image>( this, _diskCache.get() );
    }
} // restoreCaches

bool
AppManagerPrivate::checkForCacheDiskStructure(const QString & cachePath, bool isTiled)
{
    QString settingsFilePath = cachePath;

    if ( !settingsFilePath.endsWith( QChar::fromLatin1('/') ) ) {
        settingsFilePath += QChar::fromLatin1('/');
    }
    settingsFilePath += QString::fromUtf8("restoreFile.");
    settingsFilePath += QString::fromUtf8(NATRON_CACHE_FILE_EXT);

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
            qDebug() << cachePath << "doesn't contain sub-folders indexed from 00 to FF. Reseting.";
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

#   if QT_VERSION < 0x050000
    QtCompat::removeRecursively(cachePath);
#   else
    if ( cacheFolder.exists() ) {
        cacheFolder.removeRecursively();
    }
#endif
    cacheFolder.mkpath( QChar::fromLatin1('.') );

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
                oss << std::hex <<  i;
                oss << std::hex << j;
                std::string str = oss.str();
                cacheFolder.mkdir( QString::fromUtf8( str.c_str() ) );
            }
        }
    }
}

void
AppManagerPrivate::setMaxCacheFiles()
{
    /*Default to something reasonnable if the code below would happen to not work for some reason*/
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
post_gl_call(const char */*name*/,
             void */*funcptr*/,
             int /*len_args*/,
             ...)
{
#ifdef GL_TRACE_CALLS
    GLenum _glerror_ = glGetError();
    if (_glerror_ != GL_NO_ERROR) {
        std::cout << "GL_ERROR : " << __FILE__ << ":" << __LINE__ << " " << gluErrorString(_glerror_) << std::endl;
        glError();
    }
#endif
}

#endif // debug

void
AppManagerPrivate::initGLAPISpecific()
{
#ifdef Q_OS_WIN32
    wglInfo.reset(new OSGLContext_wgl_data);
    OSGLContext_win::initWGLData( wglInfo.get() );
#elif defined(Q_OS_LINUX)
    glxInfo.reset(new OSGLContext_glx_data);
    OSGLContext_x11::initGLXData( glxInfo.get() );

#endif // Q_OS_WIN32
}

void
AppManagerPrivate::initGl()
{
    // Private should not lock
    assert( !openGLFunctionsMutex.tryLock() );

    hasInitializedOpenGLFunctions = true;
    hasRequiredOpenGLVersionAndExtensions = true;


#ifdef DEBUG
    glad_set_pre_callback(pre_gl_call);
    glad_set_post_callback(post_gl_call);
#endif


    if ( !gladLoadGL() ||
        GLVersion.major < 2 || (GLVersion.major == 2 && GLVersion.minor < 0)) {
        missingOpenglError = tr("Failed to load required OpenGL functions. " NATRON_APPLICATION_NAME " requires at least OpenGL 2.0 with the following extensions: ");
        missingOpenglError += QString::fromUtf8("GL_ARB_vertex_buffer_object,GL_ARB_pixel_buffer_object,GL_ARB_vertex_array_object,GL_ARB_framebuffer_object,GL_ARB_texture_float");
        missingOpenglError += QLatin1String("\n");
        missingOpenglError += tr("Your OpenGL version ");
        missingOpenglError += appPTR->getOpenGLVersion();
        hasRequiredOpenGLVersionAndExtensions = false;

        return;
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
#elif defined(Q_OS_LINUX)
    if (glxInfo) {
        OSGLContext_x11::destroyGLXData( glxInfo.get() );
    }
#endif
}

NATRON_NAMESPACE_EXIT;
