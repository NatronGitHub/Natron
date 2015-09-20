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

#ifndef _Engine_AppManagerPrivate_h_
#define _Engine_AppManagerPrivate_h_

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QAtomicInt>

#include "Engine/AppManager.h"
#include "Engine/Cache.h"
#include "Engine/FrameEntry.h"
#include "Engine/Image.h"

class QProcess;
class QLocalServer;
class QLocalSocket;

#ifdef NATRON_USE_BREAKPAD
namespace google_breakpad {
    class ExceptionHandler;
}
#endif
class ProcessInputChannel;
namespace Natron {
    class OfxHost;
}



struct AppManagerPrivate
{
    AppManager::AppTypeEnum _appType; //< the type of app
    std::map<int,AppInstanceRef> _appInstances; //< the instances mapped against their ID
    int _availableID; //< the ID for the next instance
    int _topLevelInstanceID; //< the top level app ID
    boost::shared_ptr<Settings> _settings; //< app settings
    std::vector<Format*> _formats; //<a list of the "base" formats available in the application
    Natron::PluginsMap _plugins; //< list of the plugins
    boost::scoped_ptr<Natron::OfxHost> ofxHost; //< OpenFX host
    boost::scoped_ptr<KnobFactory> _knobFactory; //< knob maker
    boost::shared_ptr<Natron::Cache<Natron::Image> >  _nodeCache; //< Images cache
    boost::shared_ptr<Natron::Cache<Natron::Image> >  _diskCache; //< Images disk cache (used by DiskCache nodes)
    boost::shared_ptr<Natron::Cache<Natron::FrameEntry> > _viewerCache; //< Viewer textures cache
    
    mutable QMutex diskCachesLocationMutex;
    QString diskCachesLocation;
    
    ProcessInputChannel* _backgroundIPC; //< object used to communicate with the main app
    //if this app is background, see the ProcessInputChannel def
    bool _loaded; //< true when the first instance is completly loaded.
    QString _binaryPath; //< the path to the application's binary
    U64 _nodesGlobalMemoryUse; //< how much memory all the nodes are using (besides the cache)
    mutable QMutex _ofxLogMutex;
    QString _ofxLog;
    size_t maxCacheFiles; //< the maximum number of files the application can open for caching. This is the hard limit * 0.9
    size_t currentCacheFilesCount; //< the number of cache files currently opened in the application
    mutable QMutex currentCacheFilesCountMutex; //< protects currentCacheFilesCount
    

    std::string currentOCIOConfigPath; //< the currentOCIO config path
    
    int idealThreadCount; // return value of QThread::idealThreadCount() cached here
    
    int nThreadsToRender; // the value held by the corresponding Knob in the Settings, stored here for faster access (3 RW lock vs 1 mutex here)
    int nThreadsPerEffect;  // the value held by the corresponding Knob in the Settings, stored here for faster access (3 RW lock vs 1 mutex here)
    bool useThreadPool; // whether the multi-thread suite should use the global thread pool (of QtConcurrent) or not
    mutable QMutex nThreadsMutex; // protects nThreadsToRender & nThreadsPerEffect & useThreadPool
    
    //The idea here is to keep track of the number of threads launched by Natron (except the ones of the global thread pool of QtConcurrent)
    //So that we can properly have an estimation of how much the cores of the CPU are used.
    //This method has advantages and drawbacks:
    // Advantages:
    // - This is quick and fast
    // - This very well describes the render activity of Natron
    //
    // Disadvantages:
    // - This only takes into account the current Natron process and disregard completly CPU activity.
    // - We might count a thread that is actually waiting in a mutex as a running thread
    // Another method could be to analyse all cores running, but this is way more expensive and would impair performances.
    QAtomicInt runningThreadsCount;
    
     //To by-pass a bug introduced in RC2 / RC3 with the serialization of bezier curves
    bool lastProjectLoadedCreatedDuringRC2Or3;
    
    ///Python needs wide strings as from Python 3.x onwards everything is unicode based
#ifndef IS_PYTHON_2
    std::vector<wchar_t*> args;
#else
    std::vector<char*> args;
#endif
    
    PyObject* mainModule;
    PyThreadState* mainThreadState;
    
#ifdef NATRON_USE_BREAKPAD
    boost::shared_ptr<google_breakpad::ExceptionHandler> breakpadHandler;
    boost::shared_ptr<QProcess> crashReporter;
    QString crashReporterBreakpadPipe;
    boost::shared_ptr<QLocalServer> crashClientServer;
    QLocalSocket* crashServerConnection;
#endif
    
    QMutex natronPythonGIL;

#ifdef Q_OS_WIN32
	//On Windows only, track the UNC path we came across because the WIN32 API does not provide any function to map
	//from UNC path to path with drive letter.
	std::map<QChar,QString> uncPathMapping;
#endif
    
    AppManagerPrivate();
    
    ~AppManagerPrivate()
    {
        for (U32 i = 0; i < args.size() ; ++i) {
            free(args[i]);
        }
        args.clear();
    }

    void initProcessInputChannel(const QString & mainProcessServerName);

    void loadBuiltinFormats();

    void saveCaches();

    void restoreCaches();

    bool checkForCacheDiskStructure(const QString & cachePath);

    void cleanUpCacheDiskStructure(const QString & cachePath);

    /**
     * @brief Called on startup to initialize the max opened files
     **/
    void setMaxCacheFiles();
    
    Natron::Plugin* findPluginById(const QString& oldId,int major, int minor) const;
    
    void declareSettingsToPython();
    
#ifdef NATRON_USE_BREAKPAD
    void initBreakpad();
#endif
};


#endif // _Engine_AppManagerPrivate_h_

