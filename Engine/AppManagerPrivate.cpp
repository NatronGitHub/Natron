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

#include "Engine/CacheSerialization.h"
#include "Engine/ExistenceCheckThread.h"
#include "Engine/Format.h"
#include "Engine/FrameEntry.h"
#include "Engine/Image.h"
#include "Engine/OfxHost.h"
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
, ofxHost( new OfxHost() )
, _knobFactory( new KnobFactory() )
, _nodeCache()
, _diskCache()
, _viewerCache()
, diskCachesLocationMutex()
, diskCachesLocation()
,_backgroundIPC(0)
,_loaded(false)
,_binaryPath()
,_nodesGlobalMemoryUse(0)
,_ofxLogMutex()
,_ofxLog()
,maxCacheFiles(0)
,currentCacheFilesCount(0)
,currentCacheFilesCountMutex()
,idealThreadCount(0)
,nThreadsToRender(0)
,nThreadsPerEffect(0)
,useThreadPool(true)
,nThreadsMutex()
,runningThreadsCount()
,lastProjectLoadedCreatedDuringRC2Or3(false)
,args()
,mainModule(0)
,mainThreadState(0)
#ifdef NATRON_USE_BREAKPAD
,breakpadProcessExecutableFilePath()
,breakpadProcessPID(0)
,breakpadHandler()
,breakpadAliveThread()
#endif
,natronPythonGIL(QMutex::Recursive)
{
    setMaxCacheFiles();
    
    runningThreadsCount = 0;
}


AppManagerPrivate::~AppManagerPrivate()
{
    for (U32 i = 0; i < args.size() ; ++i) {
        free(args[i]);
    }
    args.clear();
}


#ifdef NATRON_USE_BREAKPAD
void
AppManagerPrivate::initBreakpad(const QString& breakpadPipePath, const QString& breakpadComPipePath, int breakpad_client_fd)
{
 
    assert(!breakpadHandler);
    createBreakpadHandler(breakpadPipePath, breakpad_client_fd);

    /*
     We check periodically that the crash reporter process is still alive. If the user killed it somehow, then we want
     the Natron process to terminate
     */
    breakpadAliveThread.reset(new ExistenceCheckerThread(NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK,
                                                         NATRON_NATRON_TO_BREAKPAD_EXISTENCE_CHECK_ACK,
                                                         breakpadComPipePath));
    QObject::connect(breakpadAliveThread.get(), SIGNAL(otherProcessUnreachable()), appPTR, SLOT(onCrashReporterNoLongerResponding()));
    breakpadAliveThread->start();

    
}

void
AppManagerPrivate::createBreakpadHandler(const QString& breakpadPipePath, int breakpad_client_fd)
{
    QString dumpPath = StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationTemp);
    Q_UNUSED(breakpad_client_fd);
    try {
#if defined(Q_OS_MAC)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler.reset(new google_breakpad::ExceptionHandler( dumpPath.toStdString(),
                                                                     0,
                                                                     0/*dmpcb*/,
                                                                     0,
                                                                     true,
                                                                     breakpadPipePath.toStdString().c_str()));
#elif defined(Q_OS_LINUX)
        Q_UNUSED(breakpadPipePath);
        breakpadHandler.reset(new google_breakpad::ExceptionHandler( google_breakpad::MinidumpDescriptor(dumpPath.toStdString()),
                                                                     0,
                                                                     0/*dmpCb*/,
                                                                     0,
                                                                     true,
                                                                     breakpad_client_fd));
#elif defined(Q_OS_WIN32)
        Q_UNUSED(breakpad_client_fd);
        breakpadHandler.reset(new google_breakpad::ExceptionHandler( dumpPath.toStdWString(),
                                                                     0, //filter callback
                                                                     0/*dmpcb*/,
																	 0, //context
                                                                     google_breakpad::ExceptionHandler::HANDLER_ALL,
                                                                     MiniDumpNormal,
                                                                     breakpadPipePath.toStdWString().c_str(),
                                                                     0));
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
    _backgroundIPC = new ProcessInputChannel(mainProcessServerName);
}



void
AppManagerPrivate::loadBuiltinFormats()
{
    /*initializing list of all Formats available*/
    std::vector<std::string> formatNames;

    struct BuiltinFormat {
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
        Format* _frmt = new Format(0, 0, f->w, f->h, f->name, f->par);
        assert(_frmt);
        _formats.push_back(_frmt);
    }
} // loadBuiltinFormats

Plugin*
AppManagerPrivate::findPluginById(const QString& newId,int major, int minor) const
{
    for (PluginsMap::const_iterator it = _plugins.begin(); it != _plugins.end(); ++it) {
        for (PluginMajorsOrdered::const_iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            if ((*it2)->getPluginID() == newId && (*it2)->getMajorVersion() == major && (*it2)->getMinorVersion() == minor) {
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
void saveCache(Cache<T>* cache)
{
    std::ofstream ofile;
    ofile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    std::string cacheRestoreFilePath = cache->getRestoreFilePath();
    try {
        ofile.open(cacheRestoreFilePath.c_str(),std::ofstream::out);
    } catch (const std::ios_base::failure & e) {
        qDebug() << "Exception occured when opening file" <<  cacheRestoreFilePath.c_str() << ':' << e.what();
        // The following is C++11 only:
        //<< "Error code: " << e.code().value()
        //<< " (" << e.code().message().c_str() << ")\n"
        //<< "Error category: " << e.code().category().name();
        
        return;
    }
    
    if ( !ofile.good() ) {
        qDebug() << "Failed to save cache to" << cacheRestoreFilePath.c_str();
        
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
    
    ofile.close();

}

void
AppManagerPrivate::saveCaches()
{
    saveCache<FrameEntry>(_viewerCache.get());
    saveCache<Image>(_diskCache.get());
} // saveCaches

template <typename T>
void restoreCache(AppManagerPrivate* p,Cache<T>* cache)
{
    if ( p->checkForCacheDiskStructure( cache->getCachePath() ) ) {
        std::ifstream ifile;
        std::string settingsFilePath = cache->getRestoreFilePath();
        try {
            ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            ifile.open(settingsFilePath.c_str(),std::ifstream::in);
        } catch (const std::ifstream::failure & e) {
            qDebug() << "Failed to open the cache restoration file:" << e.what();
            
            return;
        }
        
        if ( !ifile.good() ) {
            qDebug() << "Failed to cache file for restoration:" <<  settingsFilePath.c_str();
            ifile.close();
            
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
            if (cacheVersion == cache->cacheVersion()) {
                iArchive >> tableOfContents;
            } else {
                p->cleanUpCacheDiskStructure(cache->getCachePath());
            }
        } catch (const std::exception & e) {
            qDebug() << "Exception when reading disk cache TOC:" << e.what();
            ifile.close();
            
            return;
        }
        
        ifile.close();
        
        QFile restoreFile( settingsFilePath.c_str() );
        restoreFile.remove();
        
        cache->restore(tableOfContents);
    }
}

void
AppManagerPrivate::restoreCaches()
{
    
    if (!appPTR->isBackground()) {
        restoreCache<FrameEntry>(this, _viewerCache.get());
        restoreCache<Image>(this, _diskCache.get());
    }
} // restoreCaches

bool
AppManagerPrivate::checkForCacheDiskStructure(const QString & cachePath)
{
    QString settingsFilePath(cachePath + QDir::separator() + "restoreFile." NATRON_CACHE_FILE_EXT);

    if ( !QFile::exists(settingsFilePath) ) {
        cleanUpCacheDiskStructure(cachePath);

        return false;
    }
    QDir directory(cachePath);
    QStringList files = directory.entryList(QDir::AllDirs);


    /*Now counting actual data files in the cache*/
    /*check if there's 256 subfolders, otherwise reset cache.*/
    int count = 0; // -1 because of the restoreFile
    int subFolderCount = 0;
    for (int i = 0; i < files.size(); ++i) {
        QString subFolder(cachePath);
        subFolder.append( QDir::separator() );
        subFolder.append(files[i]);
        if ( ( subFolder.right(1) == QString(".") ) || ( subFolder.right(2) == QString("..") ) ) {
            continue;
        }
        QDir d(subFolder);
        if ( d.exists() ) {
            ++subFolderCount;
            QStringList items = d.entryList();
            for (int j = 0; j < items.size(); ++j) {
                if ( ( items[j] != QString(".") ) && ( items[j] != QString("..") ) ) {
                    ++count;
                }
            }
        }
    }
    if (subFolderCount < 256) {
        qDebug() << cachePath << "doesn't contain sub-folders indexed from 00 to FF. Reseting.";
        cleanUpCacheDiskStructure(cachePath);

        return false;
    }

    return true;
}

void
AppManagerPrivate::cleanUpCacheDiskStructure(const QString & cachePath)
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
    cacheFolder.mkpath(".");

    QStringList etr = cacheFolder.entryList(QDir::NoDotAndDotDot);
    // if not 256 subdirs, we re-create the cache
    if (etr.size() < 256) {
        Q_FOREACH (QString e, etr) {
            cacheFolder.rmdir(e);
        }
    }
    for (U32 i = 0x00; i <= 0xF; ++i) {
        for (U32 j = 0x00; j <= 0xF; ++j) {
            std::ostringstream oss;
            oss << std::hex <<  i;
            oss << std::hex << j;
            std::string str = oss.str();
            cacheFolder.mkdir( str.c_str() );
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

NATRON_NAMESPACE_EXIT;
