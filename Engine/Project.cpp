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

#include "Project.h"

#include <fstream>
#include <algorithm> // min, max
#include <ios>
#include <cstdlib> // strtoul
#include <cerrno> // errno
#include <cassert>
#include <stdexcept>

#if !defined(SBK_RUN) && !defined(Q_MOC_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif

#ifdef __NATRON_WIN32__
#include <stdio.h> //for _snprintf
#include <windows.h> //for GetUserName
#include <Lmcons.h> //for UNLEN
#define snprintf _snprintf
#elif defined(__NATRON_UNIX__)
#include <pwd.h> //for getpwuid
#endif


#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QDir>
#include <QtCore/QTemporaryFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtNetwork/QHostInfo>
#include <QtConcurrentRun> // QtCore on Qt4, QtConcurrent on Qt5

#include <ofxhXml.h> // OFX::XML::escape

#include "Global/StrUtils.h"
#include "Global/FStreamsSupport.h"
#ifdef DEBUG
#include "Global/FloatingPointExceptions.h"
#endif

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/BezierCPSerialization.h"
#include "Engine/EffectInstance.h"
#include "Engine/FormatSerialization.h"
#include "Engine/Hash64.h"
#include "Engine/KnobFile.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/RectDSerialization.h"
#include "Engine/RectISerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/Settings.h"
#include "Engine/StandardPaths.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewIdx.h"

NATRON_NAMESPACE_ENTER

using std::cout; using std::endl;
using std::make_pair;


static std::string
getUserName()
{
#ifdef __NATRON_WIN32__
    wchar_t user_name[UNLEN + 1];
    DWORD user_name_size = sizeof(user_name);
    GetUserNameW(user_name, &user_name_size);

    std::wstring wUserName(user_name);
    std::string sUserName = StrUtils::utf16_to_utf8(wUserName);
    return sUserName;
#elif defined(__NATRON_UNIX__)
    struct passwd *passwd;
    passwd = getpwuid( getuid() );

    return passwd->pw_name;
#endif
}

static std::string
generateGUIUserName()
{
    return getUserName() + '@' + QHostInfo::localHostName().toStdString();
}

static std::string
generateUserFriendlyNatronVersionName()
{
    std::string ret(NATRON_APPLICATION_NAME);

    ret.append(" v");
    ret.append(NATRON_VERSION_STRING);
    const std::string status(NATRON_DEVELOPMENT_STATUS);
    ret.append(" " NATRON_DEVELOPMENT_STATUS);
    if (status == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
        ret.append( QString::number(NATRON_BUILD_NUMBER).toStdString() );
    }

    return ret;
}


Project::Project(const AppInstancePtr& appInstance)
    : KnobHolder(appInstance)
    , NodeCollection(appInstance)
    , _imp( new ProjectPrivate(this) )
{
    QObject::connect( _imp->autoSaveTimer.get(), SIGNAL(timeout()), this, SLOT(onAutoSaveTimerTriggered()) );
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct Project::MakeSharedEnabler: public Project
{
    MakeSharedEnabler(const AppInstancePtr& appInstance) : Project(appInstance) {
    }
};


ProjectPtr
Project::create(const AppInstancePtr& appInstance)
{
    return boost::make_shared<Project::MakeSharedEnabler>(appInstance);
}


Project::~Project()
{
    ///wait for all autosaves to finish
    for (std::list<boost::shared_ptr<QFutureWatcher<void> > >::iterator it = _imp->autoSaveFutures.begin(); it != _imp->autoSaveFutures.end(); ++it) {
        (*it)->waitForFinished();
    }

    ///Don't clear autosaves if the program is shutting down by user request.
    ///Even if the user replied she/he didn't want to save the current work, we keep an autosave of it.
    //removeAutoSaves();
}

NATRON_NAMESPACE_ANONYMOUS_ENTER

class LoadProjectSplashScreen_RAII
{
    AppInstanceWPtr app;

public:

    LoadProjectSplashScreen_RAII(const AppInstancePtr& app,
                                 const QString& filename)
        : app(app)
    {
        if (app) {
            app->createLoadProjectSplashScreen(filename);
        }
    }

    ~LoadProjectSplashScreen_RAII()
    {
        AppInstancePtr a = app.lock();

        if (a) {
            a->closeLoadPRojectSplashScreen();
        }
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

bool
Project::loadProject(const QString & path,
                     const QString & name,
                     bool isUntitledAutosave,
                     bool attemptToLoadAutosave)
{
    reset(false, true);

    try {
        QString realPath = path;
        StrUtils::ensureLastPathSeparator(realPath);
        QString realName = name;
        bool isAutoSave = isUntitledAutosave;
        if (!getApp()->isBackground() && !isUntitledAutosave) {
            // In Gui mode, attempt to load an auto-save for this project if there's one.
            QString autosaveFileName;
            bool hasAutoSave = findAutoSaveForProject(realPath, name, &autosaveFileName);
            if (hasAutoSave) {
                StandardButtonEnum ret = eStandardButtonNo;
                if (attemptToLoadAutosave) {
                    QString text = tr("A recent auto-save of %1 was found.\n"
                                      "Would you like to use it instead? "
                                      "Clicking No will remove this auto-save.").arg(name);
                    ret = Dialogs::questionDialog(tr("Auto-save").toStdString(),
                                                  text.toStdString(), false, StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                  eStandardButtonYes);
                }
                if ( (ret == eStandardButtonNo) || (ret == eStandardButtonEscape) ) {
                    QFile::remove(realPath + autosaveFileName);
                } else {
                    realName = autosaveFileName;
                    isAutoSave = true;
                    {
                        QMutexLocker k(&_imp->projectLock);
                        _imp->lastAutoSaveFilePath = realPath + realName;
                    }
                }
            }
        }

        bool mustSave = false;
        if ( !loadProjectInternal(realPath, realName, isAutoSave, isUntitledAutosave, &mustSave) ) {
            appPTR->showErrorLog();
        } else if (mustSave) {
            saveProject(realPath, realName, 0);
        }
    } catch (const std::exception & e) {
        Dialogs::errorDialog( tr("Project loader").toStdString(), tr("Error while loading project: %1").arg( QString::fromUtf8( e.what() ) ).toStdString() );
        if ( !getApp()->isBackground() ) {
            CreateNodeArgs args(PLUGINID_NATRON_VIEWER, shared_from_this() );
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            getApp()->createNode(args);
        }

        return false;
    } catch (...) {
        Dialogs::errorDialog( tr("Project loader").toStdString(), tr("Unknown error while loading project.").toStdString() );
        if ( !getApp()->isBackground() ) {
            CreateNodeArgs args(PLUGINID_NATRON_VIEWER, shared_from_this() );
            args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
            args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
            getApp()->createNode(args);
        }

        return false;
    }

    refreshViewersAndPreviews();

    return true;
} // loadProject

bool
Project::loadProjectInternal(const QString & path,
                             const QString & name,
                             bool isAutoSave,
                             bool isUntitledAutosave,
                             bool* mustSave)
{
    FlagSetter loadingProjectRAII(true, &_imp->isLoadingProject, &_imp->isLoadingProjectMutex);
    QString filePath = path + name;
    std::cout << tr("Loading project: %1").arg(filePath).toStdString() << std::endl;

    if ( !QFile::exists(filePath) ) {
        throw std::invalid_argument( QString( filePath + QString::fromUtf8(" : no such file.") ).toStdString() );
    }

    bool ret = false;
    FStreamsSupport::ifstream ifile;
    FStreamsSupport::open( &ifile, filePath.toStdString() );
    if (!ifile) {
        throw std::runtime_error( tr("Failed to open %1").arg(filePath).toStdString() );
    }

    if ( (NATRON_VERSION_MAJOR == 1) && (NATRON_VERSION_MINOR == 0) && (NATRON_VERSION_REVISION == 0) ) {
        ///Try to determine if the project was made during Natron v1.0.0 - RC2 or RC3 to detect a bug we introduced at that time
        ///in the BezierCP class serialisation
        bool foundV = false;
        QFile f(filePath);
        f.open(QIODevice::ReadOnly);
        QTextStream fs(&f);
        while ( !fs.atEnd() ) {
            QString line = fs.readLine();

            if ( (line.indexOf( QString::fromUtf8("Natron v1.0.0 RC2") ) != -1) || (line.indexOf( QString::fromUtf8("Natron v1.0.0 RC3") ) != -1) ) {
                appPTR->setProjectCreatedDuringRC2Or3(true);
                foundV = true;
                break;
            }
        }
        if (!foundV) {
            appPTR->setProjectCreatedDuringRC2Or3(false);
        }
    }

    LoadProjectSplashScreen_RAII __raii_splashscreen__(getApp(), name);

    try {
        bool bgProject;
        boost::archive::xml_iarchive iArchive(ifile);
        {
            FlagSetter __raii_loadingProjectInternal__(true, &_imp->isLoadingProjectInternal, &_imp->isLoadingProjectMutex);

            iArchive >> boost::serialization::make_nvp("Background_project", bgProject);
            ProjectSerialization projectSerializationObj( getApp() );
            iArchive >> boost::serialization::make_nvp("Project", projectSerializationObj);
            ret = load(projectSerializationObj, name, path, mustSave);
        } // __raii_loadingProjectInternal__

        if (!bgProject) {
            getApp()->loadProjectGui(isAutoSave, iArchive);
        }
    } catch (const std::exception &e) {
        const ProjectBeingLoadedInfo& pInfo = getApp()->getProjectBeingLoadedInfo();
        if (pInfo.vMajor > NATRON_VERSION_MAJOR ||
            (pInfo.vMajor == NATRON_VERSION_MAJOR && pInfo.vMinor > NATRON_VERSION_MINOR) ||
            (pInfo.vMajor == NATRON_VERSION_MAJOR && pInfo.vMinor == NATRON_VERSION_MINOR && pInfo.vRev > NATRON_VERSION_REVISION)) {
            QString message = tr("This project was saved with a more recent version (%1.%2.%3) of %4. Projects are not forward compatible and may only be opened in a version of %4 equal or more recent than the version that saved it.").arg(pInfo.vMajor).arg(pInfo.vMinor).arg(pInfo.vRev).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            throw std::runtime_error(message.toStdString());
        }
        throw std::runtime_error( tr("Unrecognized or damaged project file:").toStdString() + ' ' + e.what());
    } catch (...) {
        const ProjectBeingLoadedInfo& pInfo = getApp()->getProjectBeingLoadedInfo();
        if (pInfo.vMajor > NATRON_VERSION_MAJOR ||
            (pInfo.vMajor == NATRON_VERSION_MAJOR && pInfo.vMinor > NATRON_VERSION_MINOR) ||
            (pInfo.vMajor == NATRON_VERSION_MAJOR && pInfo.vMinor == NATRON_VERSION_MINOR && pInfo.vRev > NATRON_VERSION_REVISION)) {
            QString message = tr("This project was saved with a more recent version (%1.%2.%3) of %4. Projects are not forward compatible and may only be opened in a version of %4 equal or more recent than the version that saved it.").arg(pInfo.vMajor).arg(pInfo.vMinor).arg(pInfo.vRev).arg(QString::fromUtf8(NATRON_APPLICATION_NAME));
            throw std::runtime_error(message.toStdString());
        }
        throw std::runtime_error( tr("Unrecognized or damaged project file").toStdString() );
    }

    Format f;
    getProjectDefaultFormat(&f);
    Q_EMIT formatChanged(f);

    _imp->natronVersion->setValue( generateUserFriendlyNatronVersionName() );
    if (isAutoSave) {
        _imp->autoSetProjectFormat = false;
        if (!isUntitledAutosave) {
            QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );
            int found = projectFilename.lastIndexOf( QString::fromUtf8(".autosave") );
            if (found != -1) {
                _imp->setProjectFilename( projectFilename.left(found).toStdString() );
            }
            _imp->hasProjectBeenSavedByUser = true;
        } else {
            _imp->hasProjectBeenSavedByUser = false;
            _imp->setProjectFilename(NATRON_PROJECT_UNTITLED);
            _imp->setProjectPath("");
        }
        _imp->lastAutoSave = QDateTime::currentDateTime();
        _imp->ageSinceLastSave = QDateTime();
        _imp->lastAutoSaveFilePath = filePath;

        QString projectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );
        QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );
        Q_EMIT projectNameChanged(projectPath + projectFilename, true);
    } else {
        Q_EMIT projectNameChanged(path + name, false);
    }

    ///Try to take the project lock by creating a lock file
    if (!isAutoSave) {
        QString lockFilePath = getLockAbsoluteFilePath();
        if ( !QFile::exists(lockFilePath) ) {
            createLockFile();
        }
    }

    _imp->runOnProjectLoadCallback();

    ///Process all events before flagging that we're no longer loading the project
    ///to avoid multiple renders being called because of reshape events of viewers
    {
#ifdef DEBUG
        boost_adaptbx::floating_point::exception_trapping trap(0);
#endif
        QCoreApplication::processEvents();
    }

    return ret;
} // Project::loadProjectInternal

bool
Project::saveProject(const QString & path,
                     const QString & name,
                     QString* newFilePath)
{
    return saveProject_imp(path, name, false, true, newFilePath);
}

bool
Project::saveProject_imp(const QString & path,
                         const QString & name,
                         bool autoS,
                         bool updateProjectProperties,
                         QString* newFilePath)
{
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        if (_imp->isLoadingProject) {
            return false;
        }
    }

    {
        QMutexLocker l(&_imp->isSavingProjectMutex);
        if (_imp->isSavingProject) {
            return false;
        } else {
            _imp->isSavingProject = true;
        }
    }

    QString ret;

    try {
        if (!autoS) {
            //if  (!isSaveUpToDate() || !QFile::exists(path+name)) {
            //We are saving, do not autosave.
            _imp->autoSaveTimer->stop();

            ret = saveProjectInternal(path, name, false, updateProjectProperties);

            ///We just saved, remove the last auto-save which is now obsolete
            removeLastAutosave();

            //}
        } else {
            if (updateProjectProperties) {
                ///Replace the last auto-save with a more recent one
                removeLastAutosave();
            }

            ret = saveProjectInternal(path, name, true, updateProjectProperties);
        }
    } catch (const std::exception & e) {
        if (!autoS) {
            Dialogs::errorDialog( tr("Save").toStdString(), e.what() );
        } else {
            qDebug() << "Save failure: " << e.what();
        }
    }

    {
        QMutexLocker l(&_imp->isSavingProjectMutex);
        _imp->isSavingProject = false;
    }

    ///Save caches ToC
    appPTR->saveCaches();

    if (newFilePath) {
        *newFilePath = ret;
    }

    return true;
} // Project::saveProject_imp

static bool
fileCopy(const QString & source,
         const QString & dest)
{
    QFile sourceFile(source);
    QFile destFile(dest);
    bool success = true;

    success &= sourceFile.open( QFile::ReadOnly );
    success &= destFile.open( QFile::WriteOnly | QFile::Truncate );
    success &= destFile.write( sourceFile.readAll() ) >= 0;
    sourceFile.close();
    destFile.close();

    return success;
}

QString
Project::saveProjectInternal(const QString & path,
                             const QString & name,
                             bool autoSave,
                             bool updateProjectProperties)
{
    bool isRenderSave = name.contains( QString::fromUtf8("RENDER_SAVE") );
    QDateTime time = QDateTime::currentDateTime();
    QString timeStr = time.toString();
    QString filePath;

    if (autoSave) {
        bool appendTimeHash = false;
        if ( path.isEmpty() ) {
            filePath = autoSavesDir();

            //If the auto-save is saved in the AutoSaves directory, there will be other autosaves for other opened
            //projects. We uniquely identity it with the time hash of the current time
            appendTimeHash = true;
        } else {
            filePath = path;
        }
        filePath.append( QLatin1Char('/') );
        filePath.append(name);
        if (!isRenderSave) {
            filePath.append( QString::fromUtf8(".autosave") );
        }
        if (!isRenderSave) {
            if (appendTimeHash) {
                Hash64 timeHash;

                Q_FOREACH(QChar ch, timeStr) {
                    timeHash.append<unsigned short>( ch.unicode() );
                }
                timeHash.computeHash();
                QString timeHashStr = QString::number( timeHash.value() );
                filePath.append(QLatin1Char('.') + timeHashStr);
            }
        }

        if (updateProjectProperties) {
            QMutexLocker l(&_imp->projectLock);
            _imp->lastAutoSaveFilePath = filePath;
        }
    } else {
        filePath = path + name;
    }

    std::string newFilePath = _imp->runOnProjectSaveCallback(filePath.toStdString(), autoSave);
    filePath = QString::fromUtf8( newFilePath.c_str() );

    ///Use a temporary file to save, so if Natron crashes it doesn't corrupt the user save.
    QString tmpFilename = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
    StrUtils::ensureLastPathSeparator(tmpFilename);
    tmpFilename.append( QString::number( time.toMSecsSinceEpoch() ) );

    {
        FStreamsSupport::ofstream ofile;
        FStreamsSupport::open( &ofile, tmpFilename.toStdString() );
        if (!ofile) {
            throw std::runtime_error( tr("Failed to open file ").toStdString() + tmpFilename.toStdString() );
        }

        ///Fix file paths before saving.
        QString oldProjectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );

        if (!autoSave && updateProjectProperties) {
            _imp->autoSetProjectDirectory(path);
            _imp->saveDate->setValue( timeStr.toStdString() );
            _imp->lastAuthorName->setValue( generateGUIUserName() );
            _imp->natronVersion->setValue( generateUserFriendlyNatronVersionName() );
        }

        try {
            boost::archive::xml_oarchive oArchive(ofile);
            bool bgProject = getApp()->isBackground();
            oArchive << boost::serialization::make_nvp("Background_project", bgProject);
            ProjectSerialization projectSerializationObj( getApp() );
            save(&projectSerializationObj);
            oArchive << boost::serialization::make_nvp("Project", projectSerializationObj);
            if (!bgProject) {
                AppInstancePtr app = getApp();
                if (app) {
                    app->saveProjectGui(oArchive);
                }
            }
        } catch (...) {
            if (!autoSave && updateProjectProperties) {
                ///Reset the old project path in case of failure.
                _imp->autoSetProjectDirectory(oldProjectPath);
            }
            throw;
        }
    } // ofile

    if ( QFile::exists(filePath) ) {
        QFile::remove(filePath);
    }
    int nAttemps = 0;

    while ( nAttemps < 10 && !fileCopy(tmpFilename, filePath) ) {
        ++nAttemps;
    }

    if (nAttemps >= 10) {
        throw std::runtime_error( "Failed to save to " + filePath.toStdString() );
    }

    QFile::remove(tmpFilename);

    if (!autoSave && updateProjectProperties) {
        QString lockFilePath = getLockAbsoluteFilePath();
        if ( QFile::exists(lockFilePath) ) {
            ///Remove the previous lock file if there was any
            removeLockFile();
        }
        {
            _imp->setProjectFilename( name.toStdString() );
            _imp->setProjectPath( path.toStdString() );
            QMutexLocker l(&_imp->projectLock);
            _imp->hasProjectBeenSavedByUser = true;
            _imp->ageSinceLastSave = time;
        }
        Q_EMIT projectNameChanged(path + name, false); //< notify the gui so it can update the title

        //Create the lock file corresponding to the project
        createLockFile();
    } else if (updateProjectProperties) {
        if (!isRenderSave) {
            QString projectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );
            QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );
            Q_EMIT projectNameChanged(projectPath + projectFilename, true);
        }
    }
    if (updateProjectProperties) {
        _imp->lastAutoSave = time;
    }

    return filePath;
} // saveProjectInternal

void
Project::autoSave()
{
    ///don't autosave in background mode...
    if ( getApp()->isBackground() ) {
        return;
    }

    QString path = QString::fromUtf8( _imp->getProjectPath().c_str() );
    QString name = QString::fromUtf8( _imp->getProjectFilename().c_str() );
    saveProject_imp(path, name, true, true, 0);
}

void
Project::triggerAutoSave()
{
    ///Should only be called in the main-thread, that is upon user interaction.
    assert( QThread::currentThread() == qApp->thread() );

    if ( getApp()->isBackground() || !appPTR->isLoaded() || isProjectClosing() ) {
        return;
    }

    if ( !hasProjectBeenSavedByUser() && !appPTR->getCurrentSettings()->isAutoSaveEnabledForUnsavedProjects() ) {
        return;
    }

    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        if (_imp->isLoadingProject) {
            return;
        }
    }

    _imp->autoSaveTimer->start( appPTR->getCurrentSettings()->getAutoSaveDelayMS() );
}

void
Project::onAutoSaveTimerTriggered()
{
    assert( !getApp()->isBackground() );

    if ( !getApp() ) {
        return;
    }

    ///check that all schedulers are not working.
    ///If so launch an auto-save, otherwise, restart the timer.
    bool canAutoSave = !hasNodeRendering() && !getApp()->isShowingDialog();

    if (canAutoSave) {
        boost::shared_ptr<QFutureWatcher<void> > watcher = boost::make_shared<QFutureWatcher<void> >();
        QObject::connect( watcher.get(), SIGNAL(finished()), this, SLOT(onAutoSaveFutureFinished()) );
        watcher->setFuture( QtConcurrent::run(this, &Project::autoSave) );
        _imp->autoSaveFutures.push_back(watcher);
    } else {
        ///If the auto-save failed because a render is in progress, try every 2 seconds to auto-save.
        ///We don't use the user-provided timeout interval here because it could be an inapropriate value.
        _imp->autoSaveTimer->start(2000);
    }
}

void
Project::onAutoSaveFutureFinished()
{
    QFutureWatcherBase* future = qobject_cast<QFutureWatcherBase*>( sender() );

    assert(future);
    for (std::list<boost::shared_ptr<QFutureWatcher<void> > >::iterator it = _imp->autoSaveFutures.begin(); it != _imp->autoSaveFutures.end(); ++it) {
        if (it->get() == future) {
            _imp->autoSaveFutures.erase(it);
            break;
        }
    }
}

bool
Project::findAutoSaveForProject(const QString& projectPath,
                                const QString& projectName,
                                QString* autoSaveFileName)
{
    QString projectAbsFilePath = projectPath + projectName;
    QDir savesDir(projectPath);
    QStringList entries = savesDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    Q_FOREACH(const QString &entry, entries) {
        QString ntpExt( QLatin1Char('.') );

        ntpExt.append( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) );
        QString searchStr(ntpExt);
        QString autosaveSuffix( QString::fromUtf8(".autosave") );
        searchStr.append(autosaveSuffix);
        int suffixPos = entry.indexOf(searchStr);
        if ( (suffixPos == -1) || entry.contains( QString::fromUtf8("RENDER_SAVE") ) ) {
            continue;
        }
        QString filename = projectPath + entry.left( suffixPos + ntpExt.size() );
        if ( (filename == projectAbsFilePath) && QFile::exists(filename) ) {
            *autoSaveFileName = entry;

            return true;
        }
    }

    return false;
}

void
Project::initializeKnobs()
{
    KnobPagePtr page = AppManager::createKnob<KnobPage>( this, tr("Settings") );

    _imp->envVars = AppManager::createKnob<KnobPath>( this, tr("Project Paths") );
    _imp->envVars->setName("projectPaths");
    _imp->envVars->setHintToolTip( tr("Specify here project paths. Any path can be used "
                                      "in file paths and can be used between brackets, for example: \n"
                                      "[%1]MyProject.ntp \n"
                                      "You can add as many project paths as you want and can name them as you want. This way it "
                                      "makes it easy to share your projects and move files around."
                                      " You can chain up paths, like so:\n "
                                      "[%1] = <PathToProject> \n"
                                      "[Scene1] = [%1]/Rush/S_01 \n"
                                      "[Shot1] = [Scene1]/Shot001 \n"
                                      "By default if a file-path is NOT absolute (i.e: not starting with '/' "
                                      " on Unix or a drive name on Windows) "
                                      "then it will be expanded using the [%1] path. "
                                      "Absolute paths are treated as normal."
                                      " The [%1] path will be set automatically to "
                                      " the location of the project file when saving and loading a project."
                                      " The [%2] path will also be set automatically for better sharing of projects with reader nodes.").arg( QString::fromUtf8(NATRON_PROJECT_ENV_VAR_NAME) ).arg( QString::fromUtf8(NATRON_OCIO_ENV_VAR_NAME) ) );
    _imp->envVars->setSecret(false);
    _imp->envVars->setMultiPath(true);


    ///Initialize the OCIO Config
    onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(), false);

    page->addKnob(_imp->envVars);

    _imp->formatKnob = AppManager::createKnob<KnobChoice>( this, tr("Project Format") );
    _imp->formatKnob->setHintToolTip( tr("The project format is used as the default workspace size for viewers, the default format for generators, and the default output format for writers.  In most cases this should match your plate resolution.") );
    _imp->formatKnob->setName("outputFormat");

    const std::vector<Format> & appFormats = appPTR->getFormats();
    std::vector<ChoiceOption> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        const Format& f = appFormats[i];
        QString formatStr = ProjectPrivate::generateStringFromFormat(f);
        if ( (f.width() == 1920) && (f.height() == 1080) && (f.getPixelAspectRatio() == 1) ) {
            _imp->formatKnob->setDefaultValue(i, 0);
        }
        if ( !f.getName().empty() ) {
            entries.push_back( ChoiceOption(f.getName(), formatStr.toStdString(), "") );
        } else {
            entries.push_back( ChoiceOption( formatStr.toStdString() ) );
        }

        _imp->builtinFormats.push_back(f);
    }
    _imp->formatKnob->setAddNewLine(false);

    _imp->formatKnob->populateChoices(entries);
    _imp->formatKnob->setAnimationEnabled(false);
    page->addKnob(_imp->formatKnob);

    QObject::connect( _imp->formatKnob.get(), SIGNAL(populated()), this, SLOT(onProjectFormatPopulated()) );

    _imp->addFormatKnob = AppManager::createKnob<KnobButton>( this, tr("New Format...") );
    _imp->addFormatKnob->setName("newFormat");
    page->addKnob(_imp->addFormatKnob);

    _imp->previewMode = AppManager::createKnob<KnobBool>( this, tr("Auto Previews") );
    _imp->previewMode->setName("autoPreviews");
    _imp->previewMode->setHintToolTip( tr("When checked, preview images in the node graph will be "
                                          "refreshed automatically. Disabling this will improve performance.") );
    _imp->previewMode->setAnimationEnabled(false);
    _imp->previewMode->setEvaluateOnChange(false);
    page->addKnob(_imp->previewMode);
    bool autoPreviewEnabled = appPTR->getCurrentSettings()->isAutoPreviewOnForNewProjects();
    _imp->previewMode->setDefaultValue(autoPreviewEnabled, 0);


    _imp->frameRange = AppManager::createKnob<KnobInt>(this, tr("Frame Range"), 2);
    _imp->frameRange->setDefaultValue(1, 0);
    _imp->frameRange->setDefaultValue(250, 1);
    _imp->frameRange->setDimensionName(0, "first");
    _imp->frameRange->setDimensionName(1, "last");
    _imp->frameRange->setEvaluateOnChange(false);
    _imp->frameRange->setName("frameRange");
    _imp->frameRange->setHintToolTip( tr("The frame range of the project as seen by plug-ins. New viewers are created automatically "
                                         "with this frame-range. By default when the first Read node is created, this range "
                                         "will be set to the "
                                         "sequence legnth, unless the Lock Range parameter is checked.") );
    _imp->frameRange->setAnimationEnabled(false);
    _imp->frameRange->setAddNewLine(false);
    page->addKnob(_imp->frameRange);

    _imp->lockFrameRange = AppManager::createKnob<KnobBool>( this, tr("Lock Range") );
    _imp->lockFrameRange->setName("lockRange");
    _imp->lockFrameRange->setDefaultValue(false);
    _imp->lockFrameRange->setAnimationEnabled(false);
    _imp->lockFrameRange->setHintToolTip( tr("By default when the first Read node is created, this range will be set to the "
                                             "sequence length, unless this parameter is checked.") );
    _imp->lockFrameRange->setEvaluateOnChange(false);
    page->addKnob(_imp->lockFrameRange);

    _imp->frameRate = AppManager::createKnob<KnobDouble>( this, tr("Frame Rate") );
    _imp->frameRate->setName("frameRate");
    _imp->frameRate->setHintToolTip( tr("The frame rate of the project. This will serve as a default value for all effects that don't produce "
                                        "special frame rates.") );
    _imp->frameRate->setAnimationEnabled(false);
    _imp->frameRate->setDefaultValue(24);
    _imp->frameRate->setDisplayMinimum(0.);
    _imp->frameRate->setDisplayMaximum(50.);
    page->addKnob(_imp->frameRate);

    _imp->gpuSupport = AppManager::createKnob<KnobChoice>( this, tr("GPU Rendering") );
    _imp->gpuSupport->setName("gpuRendering");
    {
        std::vector<ChoiceOption> entries;
        entries.push_back(ChoiceOption("Enabled","",tr("Enable GPU rendering if required resources are available and the plugin supports it.").toStdString()));
        entries.push_back(ChoiceOption("Disabled", "", tr("Disable GPU rendering for all plug-ins.").toStdString()));
        entries.push_back(ChoiceOption("Disabled if background","",tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString()));
        _imp->gpuSupport->populateChoices(entries);
    }
    _imp->gpuSupport->setAnimationEnabled(false);
    _imp->gpuSupport->setHintToolTip( tr("Select when to activate GPU rendering for plug-ins. Note that if the OpenGL Rendering parameter in the Preferences/GPU Rendering is set to disabled then GPU rendering will not be activated regardless of that value.") );
    _imp->gpuSupport->setEvaluateOnChange(false);
    if (!appPTR->getCurrentSettings()->isOpenGLRenderingEnabled()) {
        _imp->gpuSupport->setAllDimensionsEnabled(false);
    }
    page->addKnob(_imp->gpuSupport);

    KnobPagePtr viewsPage = AppManager::createKnob<KnobPage>( this, tr("Views") );

    _imp->viewsList = AppManager::createKnob<KnobPath>( this, tr("Views List") );
    _imp->viewsList->setName("viewsList");
    _imp->viewsList->setHintToolTip( tr("The list of the views in the project") );
    _imp->viewsList->setAnimationEnabled(false);
    _imp->viewsList->setEvaluateOnChange(false);
    _imp->viewsList->setAsStringList(true);
    std::list<std::string> defaultViews;
    defaultViews.push_back("Main");
    std::string encodedDefaultViews = _imp->viewsList->encodeToKnobTableFormatSingleCol(defaultViews);
    _imp->viewsList->setDefaultValue(encodedDefaultViews);
    viewsPage->addKnob(_imp->viewsList);

    _imp->setupForStereoButton = AppManager::createKnob<KnobButton>( this, tr("Setup views for stereo") );
    _imp->setupForStereoButton->setName("setupForStereo");
    _imp->setupForStereoButton->setHintToolTip( tr("Quickly setup the views list for stereo") );
    _imp->setupForStereoButton->setEvaluateOnChange(false);
    viewsPage->addKnob(_imp->setupForStereoButton);


    KnobPagePtr LayersPage = AppManager::createKnob<KnobPage>( this, tr("Layers") );

    _imp->defaultLayersList = AppManager::createKnob<KnobLayers>( this, tr("Default Layers") );
    _imp->defaultLayersList->setName("defaultLayers");
    _imp->defaultLayersList->setHintToolTip( tr("The list of the default layers available in layers menus on nodes.") );
    _imp->defaultLayersList->setAnimationEnabled(false);
    _imp->defaultLayersList->setEvaluateOnChange(false);
    std::list<std::vector<std::string> > defaultLayers;
    {
        std::vector<ImagePlaneDesc> defaultComponents;
        defaultComponents.push_back(ImagePlaneDesc::getRGBAComponents());
        defaultComponents.push_back(ImagePlaneDesc::getDisparityLeftComponents());
        defaultComponents.push_back(ImagePlaneDesc::getDisparityRightComponents());
        defaultComponents.push_back(ImagePlaneDesc::getBackwardMotionComponents());
        defaultComponents.push_back(ImagePlaneDesc::getForwardMotionComponents());


        for (std::size_t i = 0; i < defaultComponents.size(); ++i) {
            const ImagePlaneDesc& comps = defaultComponents[i];
            std::vector<std::string> row(3);
            row[0] = comps.getPlaneLabel();
            std::string channelsStr;
            const std::vector<std::string>& channels = comps.getChannels();
            for (std::size_t c = 0; c < channels.size(); ++c) {
                if (c > 0) {
                    channelsStr += ' ';
                }
                channelsStr += channels[c];
            }
            row[1] = channelsStr;
            row[2] = comps.getChannelsLabel();
            defaultLayers.push_back(row);
        }
    }
    std::string encodedDefaultLayers = _imp->defaultLayersList->encodeToKnobTableFormat(defaultLayers);
    _imp->defaultLayersList->setDefaultValue(encodedDefaultLayers);
    LayersPage->addKnob(_imp->defaultLayersList);

    KnobPagePtr lutPages = AppManager::createKnob<KnobPage>( this, tr("LUT") );
    std::vector<ChoiceOption> colorSpaces;
    // Keep it in sync with ViewerColorSpaceEnum
    colorSpaces.push_back(ChoiceOption("Linear","",""));
    colorSpaces.push_back(ChoiceOption("sRGB","",""));
    colorSpaces.push_back(ChoiceOption("Rec.709","",""));

    _imp->colorSpace8u = AppManager::createKnob<KnobChoice>( this, tr("8-Bit LUT") );
    _imp->colorSpace8u->setName("defaultColorSpace8u");
    _imp->colorSpace8u->setHintToolTip( tr("Defines the 1D LUT used to convert to 8-bit image data if an effect cannot process floating-point images.") );
    _imp->colorSpace8u->setAnimationEnabled(false);
    _imp->colorSpace8u->populateChoices(colorSpaces);
    _imp->colorSpace8u->setDefaultValue(1);
    lutPages->addKnob(_imp->colorSpace8u);

    _imp->colorSpace16u = AppManager::createKnob<KnobChoice>( this, tr("16-Bit LUT") );
    _imp->colorSpace16u->setName("defaultColorSpace16u");
    _imp->colorSpace16u->setHintToolTip( tr("Defines the 1D LUT used to convert to 16-bit image data if an effect cannot process floating-point images.") );
    _imp->colorSpace16u->setAnimationEnabled(false);
    _imp->colorSpace16u->populateChoices(colorSpaces);
    _imp->colorSpace16u->setDefaultValue(2);
    lutPages->addKnob(_imp->colorSpace16u);

    _imp->colorSpace32f = AppManager::createKnob<KnobChoice>( this, tr("32-Bit Floating Point LUT ") );
    _imp->colorSpace32f->setName("defaultColorSpace32f");
    _imp->colorSpace32f->setHintToolTip( tr("Defines the 1D LUT used to convert from 32-bit floating-point image data if an effect cannot process floating-point images.") );
    _imp->colorSpace32f->setAnimationEnabled(false);
    _imp->colorSpace32f->populateChoices(colorSpaces);
    _imp->colorSpace32f->setDefaultValue(0);
    lutPages->addKnob(_imp->colorSpace32f);

    KnobPagePtr infoPage = AppManager::createKnob<KnobPage>( this, tr("Info").toStdString() );

    _imp->projectName = AppManager::createKnob<KnobString>( this, tr("Project Name") );
    _imp->projectName->setName("projectName");
    _imp->projectName->setIsPersistent(false);
//    _imp->projectName->setAsLabel();
    _imp->projectName->setDefaultEnabled(0, false);
    _imp->projectName->setAnimationEnabled(false);
    _imp->projectName->setDefaultValue(NATRON_PROJECT_UNTITLED);
    infoPage->addKnob(_imp->projectName);

    _imp->projectPath = AppManager::createKnob<KnobString>( this, tr("Project path") );
    _imp->projectPath->setName("projectPath");
    _imp->projectPath->setIsPersistent(false);
    _imp->projectPath->setAnimationEnabled(false);
    _imp->projectPath->setDefaultEnabled(0, false);
    // _imp->projectPath->setAsLabel();
    infoPage->addKnob(_imp->projectPath);

    _imp->natronVersion = AppManager::createKnob<KnobString>( this, tr("Saved With") );
    _imp->natronVersion->setName("softwareVersion");
    _imp->natronVersion->setHintToolTip( tr("The version of %1 that saved this project for the last time.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    // _imp->natronVersion->setAsLabel();
    _imp->natronVersion->setDefaultEnabled(0, false);
    _imp->natronVersion->setEvaluateOnChange(false);
    _imp->natronVersion->setAnimationEnabled(false);

    _imp->natronVersion->setDefaultValue( generateUserFriendlyNatronVersionName() );
    infoPage->addKnob(_imp->natronVersion);

    _imp->originalAuthorName = AppManager::createKnob<KnobString>( this, tr("Original Author") );
    _imp->originalAuthorName->setName("originalAuthor");
    _imp->originalAuthorName->setHintToolTip( tr("The user name and host name of the original author of the project.") );
    //_imp->originalAuthorName->setAsLabel();
    _imp->originalAuthorName->setDefaultEnabled(0, false);
    _imp->originalAuthorName->setEvaluateOnChange(false);
    _imp->originalAuthorName->setAnimationEnabled(false);
    std::string authorName = generateGUIUserName();
    _imp->originalAuthorName->setDefaultValue(authorName);
    infoPage->addKnob(_imp->originalAuthorName);

    _imp->lastAuthorName = AppManager::createKnob<KnobString>( this, tr("Last Author") );
    _imp->lastAuthorName->setName("lastAuthor");
    _imp->lastAuthorName->setHintToolTip( tr("The user name and host name of the last author of the project.") );
    // _imp->lastAuthorName->setAsLabel();
    _imp->lastAuthorName->setDefaultEnabled(0, false);
    _imp->lastAuthorName->setEvaluateOnChange(false);
    _imp->lastAuthorName->setAnimationEnabled(false);
    _imp->lastAuthorName->setDefaultValue(authorName);
    infoPage->addKnob(_imp->lastAuthorName);


    _imp->projectCreationDate = AppManager::createKnob<KnobString>( this, tr("Created On") );
    _imp->projectCreationDate->setName("creationDate");
    _imp->projectCreationDate->setHintToolTip( tr("The creation date of the project.") );
    //_imp->projectCreationDate->setAsLabel();
    _imp->projectCreationDate->setDefaultEnabled(0, false);
    _imp->projectCreationDate->setEvaluateOnChange(false);
    _imp->projectCreationDate->setAnimationEnabled(false);
    _imp->projectCreationDate->setDefaultValue( QDateTime::currentDateTime().toString().toStdString() );
    infoPage->addKnob(_imp->projectCreationDate);

    _imp->saveDate = AppManager::createKnob<KnobString>( this, tr("Last Saved On") );
    _imp->saveDate->setName("lastSaveDate");
    _imp->saveDate->setHintToolTip( tr("The date this project was last saved.") );
    //_imp->saveDate->setAsLabel();
    _imp->saveDate->setDefaultEnabled(0, false);
    _imp->saveDate->setEvaluateOnChange(false);
    _imp->saveDate->setAnimationEnabled(false);
    infoPage->addKnob(_imp->saveDate);

    KnobStringPtr comments = AppManager::createKnob<KnobString>( this, tr("Comments") );
    comments->setName("comments");
    comments->setHintToolTip( tr("This area is a good place to write some information about the project such as its authors, license "
                                 "and anything worth mentioning about it.") );
    comments->setAsMultiLine();
    comments->setAnimationEnabled(false);
    infoPage->addKnob(comments);

    KnobPagePtr pythonPage = AppManager::createKnob<KnobPage>( this, tr("Python") );
    _imp->onProjectLoadCB = AppManager::createKnob<KnobString>( this, tr("After Project Loaded") );
    _imp->onProjectLoadCB->setName("afterProjectLoad");
    _imp->onProjectLoadCB->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time this project "
                                              "is loaded either from an auto-save or by a user action. It will be called immediately after all "
                                              "nodes are re-created. This callback will not be called when creating new projects.\n "
                                              "The signature of the callback is: callback(app) where:\n"
                                              "- app: points to the current application instance.") );
    _imp->onProjectLoadCB->setAnimationEnabled(false);
    std::string onProjectLoad = appPTR->getCurrentSettings()->getDefaultOnProjectLoadedCB();
    _imp->onProjectLoadCB->setDefaultValue(onProjectLoad);
    pythonPage->addKnob(_imp->onProjectLoadCB);


    _imp->onProjectSaveCB = AppManager::createKnob<KnobString>( this, tr("Before Project Save") );
    _imp->onProjectSaveCB->setName("beforeProjectSave");
    _imp->onProjectSaveCB->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time this project "
                                              "is saved by the user. This will be called prior to actually saving the project and can be used "
                                              "to change the filename of the file.\n"
                                              "The signature of the callback is: callback(filename,app,autoSave) where:\n"
                                              "- filename: points to the filename under which the project will be saved"
                                              "- app: points to the current application instance\n"
                                              "- autoSave: True if the save was called due to an auto-save, False otherwise\n"
                                              "You should return the new filename under which the project should be saved.") );
    _imp->onProjectSaveCB->setAnimationEnabled(false);
    std::string onProjectSave = appPTR->getCurrentSettings()->getDefaultOnProjectSaveCB();
    _imp->onProjectSaveCB->setDefaultValue(onProjectSave);
    pythonPage->addKnob(_imp->onProjectSaveCB);

    _imp->onProjectCloseCB = AppManager::createKnob<KnobString>( this, tr("Before Project Close") );
    _imp->onProjectCloseCB->setName("beforeProjectClose");
    _imp->onProjectCloseCB->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time this project "
                                               "is closed or if the user closes the application while this project is opened. This is called "
                                               "prior to removing anything from the project.\n"
                                               "The signature of the callback is: callback(app) where:\n"
                                               "- app: points to the current application instance.") );
    _imp->onProjectCloseCB->setAnimationEnabled(false);
    std::string onProjectClose = appPTR->getCurrentSettings()->getDefaultOnProjectCloseCB();
    _imp->onProjectCloseCB->setValue(onProjectClose);
    pythonPage->addKnob(_imp->onProjectCloseCB);

    _imp->onNodeCreated = AppManager::createKnob<KnobString>( this, tr("After Node Created") );
    _imp->onNodeCreated->setName("afterNodeCreated");
    _imp->onNodeCreated->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                            "is created. The boolean variable userEdited will be set to True if the node was created "
                                            "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                            "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                            "- thisNode: the node which has just been created\n"
                                            "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                            "a script/project load/copy-paste\n"
                                            "- app: points to the current application instance.") );
    _imp->onNodeCreated->setAnimationEnabled(false);
    std::string onNodeCreated = appPTR->getCurrentSettings()->getDefaultOnNodeCreatedCB();
    _imp->onNodeCreated->setDefaultValue(onNodeCreated);
    pythonPage->addKnob(_imp->onNodeCreated);

    _imp->onNodeDeleted = AppManager::createKnob<KnobString>( this, tr("Before Node Removal") );
    _imp->onNodeDeleted->setName("beforeNodeRemoval");
    _imp->onNodeDeleted->setHintToolTip( tr("Add here the name of a Python-defined function that will be called each time a node "
                                            "is about to be deleted. This function will not be called when the project is closing.\n"
                                            "The signature of the callback is: callback(thisNode, app) where:\n"
                                            "- thisNode: the node about to be deleted\n"
                                            "- app: points to the current application instance.") );
    _imp->onNodeDeleted->setAnimationEnabled(false);
    std::string onNodeDelete = appPTR->getCurrentSettings()->getDefaultOnNodeDeleteCB();
    _imp->onNodeDeleted->setDefaultValue(onNodeDelete);
    pythonPage->addKnob(_imp->onNodeDeleted);


    comments->setAsMultiLine();
    comments->setAnimationEnabled(false);
    infoPage->addKnob(comments);

    Q_EMIT knobsInitialized();
} // initializeKnobs

// don't return a reference to a mutex-protected object!
void
Project::getProjectDefaultFormat(Format *f) const
{
    assert(f);
    QMutexLocker l(&_imp->formatMutex);
    ChoiceOption formatSpec = _imp->formatKnob->getActiveEntry();
    // use the label here, because the id does not contain the format specifications.
    // see ProjectPrivate::generateStringFromFormat()
#pragma message WARN("TODO: can't we store the format somewhere instead of parsing the label???")
    if ( !formatSpec.label.empty() ) {
        ProjectPrivate::generateFormatFromString(QString::fromUtf8( formatSpec.label.c_str() ), f);
    } else {
        _imp->findFormat(_imp->formatKnob->getValue(), f);
    }
}

bool
Project::getProjectFormatAtIndex(int index,
                                 Format* f) const
{
    assert(f);
    QMutexLocker l(&_imp->formatMutex);

    return _imp->findFormat(index, f);
}

bool
Project::isOpenGLRenderActivated() const
{
    if (!appPTR->getCurrentSettings()->isOpenGLRenderingEnabled()) {
        return false;
    }
    int index =  _imp->gpuSupport->getValue();
    return index == 0 || (index == 2 && !getApp()->isBackground());
}

int
Project::currentFrame() const
{
    return _imp->timeline->currentFrame();
}

int
Project::tryAddProjectFormat(const Format & f, bool* existed)
{
    //assert( !_imp->formatMutex.tryLock() );
    if ( ( f.left() >= f.right() ) || ( f.bottom() >= f.top() ) ) {
        return -1;
    }

    std::list<Format>::iterator foundFormat = std::find(_imp->builtinFormats.begin(), _imp->builtinFormats.end(), f);
    if ( foundFormat != _imp->builtinFormats.end() ) {
        *existed = true;
        return std::distance(_imp->builtinFormats.begin(), foundFormat);
    } else {
        foundFormat = std::find(_imp->additionalFormats.begin(), _imp->additionalFormats.end(), f);
        if ( foundFormat != _imp->additionalFormats.end() ) {
            *existed = true;
            return std::distance(_imp->additionalFormats.begin(), foundFormat) + _imp->builtinFormats.size();
        }
    }

    std::vector<ChoiceOption> entries;
    for (std::list<Format>::iterator it = _imp->builtinFormats.begin(); it != _imp->builtinFormats.end(); ++it) {
        const Format & f = *it;
        QString formatStr = ProjectPrivate::generateStringFromFormat(f);
        if ( !f.getName().empty() ) {
            entries.push_back( ChoiceOption(f.getName(), formatStr.toStdString(), "") );
        } else {
            entries.push_back( ChoiceOption( formatStr.toStdString() ) );
        }
    }
    for (std::list<Format>::iterator it = _imp->additionalFormats.begin(); it != _imp->additionalFormats.end(); ++it) {
        const Format & f = *it;
        QString formatStr = ProjectPrivate::generateStringFromFormat(f);
        if ( !f.getName().empty() ) {
            entries.push_back( ChoiceOption(f.getName(), formatStr.toStdString(), "") );
        } else {
            entries.push_back( ChoiceOption( formatStr.toStdString() ) );
        }
    }
    QString formatStr = ProjectPrivate::generateStringFromFormat(f);
    _imp->additionalFormats.push_back(f);
    if ( !f.getName().empty() ) {
        _imp->formatKnob->appendChoice( ChoiceOption(f.getName(), formatStr.toStdString(), "") );
    } else {
        _imp->formatKnob->appendChoice( ChoiceOption( formatStr.toStdString() ) );
    }

    return ( _imp->builtinFormats.size() + _imp->additionalFormats.size() ) - 1;
}

void
Project::setProjectDefaultFormat(const Format & f)
{
    //assert( !_imp->formatMutex.tryLock() );
    bool existed;
    int index = tryAddProjectFormat(f, &existed);

    _imp->formatKnob->setValue(index);
    ///if locked it will trigger a deadlock because some parameters
    ///might respond to this signal by checking the content of the project format.
}

void
Project::getProjectFormatEntries(std::vector<ChoiceOption>* formatStrings,
                                 int* currentValue) const
{
    *formatStrings = _imp->formatKnob->getEntries_mt_safe();
    *currentValue = _imp->formatKnob->getValue();
}

bool
Project::getFormatNameFromRect(const RectI& rect, double par, std::string* name) const
{
    // First try to find the format in the project formats, they have a good chance to be named
    bool foundFormat = false;
    Format f;
    {
        QMutexLocker k(&_imp->formatMutex);

        for (std::list<Format>::const_iterator it = _imp->builtinFormats.begin(); it!=_imp->builtinFormats.end(); ++it) {
            if (it->x1 == rect.x1 && it->y1 == rect.x1 && it->x2 == rect.x2 && it->y2 == rect.y2 && par == it->getPixelAspectRatio()) {
                f = *it;
                foundFormat = true;
                break;
            }
        }
        if (!foundFormat) {
            for (std::list<Format>::const_iterator it = _imp->additionalFormats.begin(); it!=_imp->additionalFormats.end(); ++it) {
                if (it->x1 == rect.x1 && it->y1 == rect.x1 && it->x2 == rect.x2 && it->y2 == rect.y2 && par == it->getPixelAspectRatio()) {
                    f = *it;
                    foundFormat = true;
                    break;
                }
            }
        }
    }
    name->clear();
    if (foundFormat) {
        *name = f.getName();
    }
    return !name->empty();
}

int
Project::getProjectViewsCount() const
{
    std::list<std::vector<std::string> > pairs;

    _imp->viewsList->getTable(&pairs);

    return (int)pairs.size();
}

bool
Project::isGPURenderingEnabledInProject() const
{
    if (!appPTR->getCurrentSettings()->isOpenGLRenderingEnabled()) {
        return false;
    }
    int index = _imp->gpuSupport->getValue();

    if (index == 0) {
        return true;
    } else if (index == 1) {
        return false;
    } else if (index == 2) {
        return !getApp()->isBackground();
    }
    assert(false);

    return false;
}

std::list<ImagePlaneDesc>
Project::getProjectDefaultLayers() const
{
    std::list<ImagePlaneDesc> ret;
    std::list<std::vector<std::string> > table;

    _imp->defaultLayersList->getTable(&table);
    for (std::list<std::vector<std::string> >::iterator it = table.begin();
         it != table.end(); ++it) {

        const std::string& planeLabel = (*it)[0];
        std::string planeID = planeLabel;
        std::string componentsLabel;

        // The layers knob only propose the user to display the label of the plane desc,
        // but we need to recover the ID for the built-in planes to ensure compatibility
        // with the old Nuke multi-plane suite.
        if (planeID == kNatronColorPlaneLabel) {
            planeID = kNatronColorPlaneID;
        } else if (planeID == kNatronBackwardMotionVectorsPlaneLabel) {
            planeID = kNatronBackwardMotionVectorsPlaneID;
            componentsLabel = kNatronMotionComponentsLabel;
        } else if (planeID == kNatronForwardMotionVectorsPlaneLabel) {
            planeID = kNatronForwardMotionVectorsPlaneID;
            componentsLabel = kNatronMotionComponentsLabel;
        } else if (planeID == kNatronDisparityLeftPlaneLabel) {
            planeID = kNatronDisparityLeftPlaneID;
            componentsLabel = kNatronDisparityComponentsLabel;
        } else if (planeID == kNatronDisparityRightPlaneLabel) {
            planeID = kNatronDisparityRightPlaneID;
            componentsLabel = kNatronDisparityComponentsLabel;
        }

        bool found = false;
        for (std::list<ImagePlaneDesc>::const_iterator it2 = ret.begin(); it2 != ret.end(); ++it2) {
            if (it2->getPlaneID() == planeID) {
                found = true;
                break;
            }
        }
        if (!found) {
            std::vector<std::string> componentsName;
            QString str = QString::fromUtf8( (*it)[1].c_str() );
            QStringList channels = str.split( QLatin1Char(' ') );
            componentsName.resize( channels.size() );
            for (int i = 0; i < channels.size(); ++i) {
                componentsName[i] = channels[i].toStdString();
            }
            ImagePlaneDesc c( planeID, planeLabel, componentsLabel, componentsName );
            ret.push_back(c);
        }
    }

    return ret;
}

void
Project::addProjectDefaultLayer(const ImagePlaneDesc& comps)
{
    const std::vector<std::string>& channels = comps.getChannels();
    std::vector<std::string> row(2);

    row[0] = comps.getPlaneLabel();
    std::string channelsStr;
    for (std::size_t i = 0; i < channels.size(); ++i) {
        channelsStr += channels[i];
        if ( i < (channels.size() - 1) ) {
            channelsStr += ' ';
        }
    }
    row[1] = channelsStr;
    _imp->defaultLayersList->appendRow(row);
}

std::vector<std::string>
Project::getProjectDefaultLayerNames() const
{
    std::vector<std::string> ret;
    std::list<std::vector<std::string> > pairs;

    _imp->defaultLayersList->getTable(&pairs);
    for (std::list<std::vector<std::string> >::iterator it = pairs.begin();
         it != pairs.end(); ++it) {
        bool found = false;
        for (std::size_t i = 0; i < ret.size(); ++i) {
            if (ret[i] == (*it)[0]) {
                found = true;
                break;
            }
        }
        if (!found) {
            ret.push_back( (*it)[0] );
        }
    }

    return ret;
}

const std::vector<std::string>&
Project::getProjectViewNames() const
{
    ///Tls is needed to implement getViewName in the multi-plane suite
    std::vector<std::string>& tls = _imp->tlsData->getOrCreateTLSData()->viewNames;

    tls.clear();

    std::list<std::vector<std::string> > pairs;
    _imp->viewsList->getTable(&pairs);
    for (std::list<std::vector<std::string> >::iterator it = pairs.begin();
         it != pairs.end(); ++it) {
        tls.push_back( (*it)[0] );
    }

    return tls;
}

void
Project::setupProjectForStereo()
{
    std::list<std::string> views;

    views.push_back("Left");
    views.push_back("Right");
    std::string encoded = _imp->viewsList->encodeToKnobTableFormatSingleCol(views);
    _imp->viewsList->setValue(encoded);
}

#if 0 // dead code
// replaced by boost::to_lower(str)
static std::string
toLowerString(const std::string& str)
{
    std::string ret;
    std::locale loc;

    for (std::size_t i = 0; i < str.size(); ++i) {
        ret.push_back( std::tolower(str[i], loc) );
    }

    return ret;
}

// replaced by boost::iequals(lhs, rhs)
static bool
caseInsensitiveCompare(const std::string& lhs,
                       const std::string& rhs)
{
    std::string lowerLhs = toLowerString(lhs);
    std::string lowerRhs = toLowerString(rhs);

    return lowerLhs == lowerRhs;
}

#endif

void
Project::createProjectViews(const std::vector<std::string>& views)
{
    std::list<std::string> pairs;

    _imp->viewsList->getTableSingleCol(&pairs);

    for (std::size_t i = 0; i < views.size(); ++i) {
        bool found = false;
        for (std::list<std::string>::iterator it = pairs.begin(); it != pairs.end(); ++it) {
            if ( boost::iequals(*it, views[i]) ) {
                found = true;
                break;
            }
        }
        if ( found || views[i].empty() ) {
            continue;
        }
        std::string view = views[i];
        view[0] = std::toupper(view[0]);
        pairs.push_back(view);
    }
    std::string encoded = _imp->viewsList->encodeToKnobTableFormatSingleCol(pairs);
    _imp->viewsList->setValue(encoded);
}

QString
Project::getProjectFilename() const
{
    return QString::fromUtf8( _imp->getProjectFilename().c_str() );
}

QString
Project::getLastAutoSaveFilePath() const
{
    QMutexLocker l(&_imp->projectLock);

    return _imp->lastAutoSaveFilePath;
}

bool
Project::hasEverAutoSaved() const
{
    return !getLastAutoSaveFilePath().isEmpty();
}

QString
Project::getProjectPath() const
{
    return QString::fromUtf8( _imp->getProjectPath().c_str() );
}

bool
Project::hasProjectBeenSavedByUser() const
{
    QMutexLocker l(&_imp->projectLock);

    return _imp->hasProjectBeenSavedByUser;
}

bool
Project::isAutoPreviewEnabled() const
{
    return _imp->previewMode->getValue();
}

void
Project::toggleAutoPreview()
{
    _imp->previewMode->setValue( !_imp->previewMode->getValue() );
}

TimeLinePtr Project::getTimeLine() const
{
    return _imp->timeline;
}

void
Project::getAdditionalFormats(std::list<Format> *formats) const
{
    assert(formats);
    QMutexLocker l(&_imp->formatMutex);
    *formats = _imp->additionalFormats;
}

bool
Project::isSaveUpToDate() const
{
    QMutexLocker l(&_imp->projectLock);

    return _imp->ageSinceLastSave == _imp->lastAutoSave;
}

void
Project::save(ProjectSerialization* serializationObject) const
{
    serializationObject->initialize(this);
}

bool
Project::load(const ProjectSerialization & obj,
              const QString& name,
              const QString& path,
              bool* mustSave)
{
    return _imp->restoreFromSerialization(obj, name, path, mustSave);
}

void
Project::beginKnobsValuesChanged(ValueChangedReasonEnum /*reason*/)
{
}

void
Project::endKnobsValuesChanged(ValueChangedReasonEnum /*reason*/)
{
}

///this function is only called on the main thread
bool
Project::onKnobValueChanged(KnobI* knob,
                            ValueChangedReasonEnum reason,
                            double /*time*/,
                            ViewSpec /*view*/,
                            bool /*originatedFromMainThread*/)
{
    bool ret = true;

    if ( knob == _imp->viewsList.get() ) {
        /**
         * All cache entries are linked to a view index which may no longer be correct since the user changed the project settings.
         * The only way to overcome this is to wipe the cache.
         **/
        appPTR->clearAllCaches();

        std::vector<std::string> viewNames = getProjectViewNames();
        getApp()->setupViewersForViews(viewNames);
        if (reason == eValueChangedReasonUserEdited) {
            ///views change, notify all OneView nodes via getClipPreferences
            forceComputeInputDependentDataOnAllTrees();
        }
        Q_EMIT projectViewsChanged();
    } else if  ( knob == _imp->defaultLayersList.get() ) {
        if (reason == eValueChangedReasonUserEdited) {
            ///default layers change, notify all nodes so they rebuild their layers menus
            forceComputeInputDependentDataOnAllTrees();
        }
    } else if ( knob == _imp->setupForStereoButton.get() ) {
        setupProjectForStereo();
    } else if ( knob == _imp->formatKnob.get() ) {
        int index = _imp->formatKnob->getValue();
        Format frmt;
        bool found = _imp->findFormat(index, &frmt);
        NodesList nodes;
        getNodes_recursive(nodes, true);

        // Refresh nodes with a format parameter
        std::vector<ChoiceOption> entries = _imp->formatKnob->getEntries_mt_safe();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->refreshFormatParamChoice(entries, index, false);
        }
        if (found) {
            if (reason == eValueChangedReasonUserEdited) {
                ///Increase all nodes age in the project so all cache is invalidated: some effects images might rely on the project format
                for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                    (*it)->incrementKnobsAge();
                }


            }
            ///Format change, hence probably the PAR so run getClipPreferences again
            forceComputeInputDependentDataOnAllTrees();
            Q_EMIT formatChanged(frmt);
        }
    } else if ( knob == _imp->addFormatKnob.get() ) {
        Q_EMIT mustCreateFormat();
    } else if ( knob == _imp->previewMode.get() ) {
        Q_EMIT autoPreviewChanged( _imp->previewMode->getValue() );
    }  else if ( knob == _imp->frameRate.get() ) {
        forceComputeInputDependentDataOnAllTrees();
    } else if ( knob == _imp->frameRange.get() ) {
        int first = _imp->frameRange->getValue(0);
        int last = _imp->frameRange->getValue(1);
        Q_EMIT frameRangeChanged(first, last);
    } else if ( knob == _imp->gpuSupport.get() ) {

        refreshOpenGLRenderingFlagOnNodes();
    } else {
        ret = false;
    }

    return ret;
} // Project::onKnobValueChanged

void
Project::refreshOpenGLRenderingFlagOnNodes()
{
    bool activated = appPTR->getCurrentSettings()->isOpenGLRenderingEnabled();
    if (!activated) {
        _imp->gpuSupport->setAllDimensionsEnabled(false);
    } else {
        _imp->gpuSupport->setAllDimensionsEnabled(true);
        int index =  _imp->gpuSupport->getValue();
        activated = index == 0 || (index == 2 && !getApp()->isBackground());
    }
    NodesList allNodes;
    getNodes_recursive(allNodes, false);
    for (NodesList::iterator it = allNodes.begin(); it!=allNodes.end(); ++it) {
        (*it)->onOpenGLEnabledKnobChangedOnProject(activated);
    }
}

bool
Project::isLoadingProject() const
{
    QMutexLocker l(&_imp->isLoadingProjectMutex);

    return _imp->isLoadingProject;
}

bool
Project::isLoadingProjectInternal() const
{
    QMutexLocker l(&_imp->isLoadingProjectMutex);

    return _imp->isLoadingProjectInternal;
}

bool
Project::isGraphWorthLess() const
{
    /*
       bool worthLess = true;
       for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (!_imp->currentNodes[i]->isOutputNode() && _imp->currentNodes[i]->isActivated()) {
            worthLess = false;
            break;
        }
       }
       return worthLess;
     */

    ///If it has never auto-saved, then the user didn't do anything, hence the project is worthless.
    return ( !hasEverAutoSaved() && !hasProjectBeenSavedByUser() ) || !hasNodes();
}

QString
Project::getLockAbsoluteFilePath() const
{
    QString projectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );
    QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );

    if ( projectPath.isEmpty() ) {
        return QString();
    }
    if ( !projectPath.endsWith( QLatin1Char('/') ) ) {
        projectPath.append( QLatin1Char('/') );
    }
    QString lockFilePath = projectPath + projectFilename + QString::fromUtf8(".lock");

    return lockFilePath;
}

void
Project::createLockFile()
{
    QString lastAuthor = QString::fromUtf8( _imp->lastAuthorName->getValue().c_str() );
    QDateTime now = QDateTime::currentDateTime();
    QString lockFilePath = getLockAbsoluteFilePath();

    if ( lockFilePath.isEmpty() ) {
        return;
    }
    QFile f(lockFilePath);
    if ( !f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) ) {
        return;
    }
    QTextStream ts(&f);
    QString curDateStr = now.toString();
    ts << curDateStr << '\n'
       << lastAuthor << '\n'
       << (qint64)QCoreApplication::applicationPid() << '\n'
       << QHostInfo::localHostName();
}

void
Project::removeLockFile()
{
    QString lockFilePath = getLockAbsoluteFilePath();

    if ( QFile::exists(lockFilePath) ) {
        QFile::remove(lockFilePath);
    }
}

bool
Project::getLockFileInfos(const QString& projectPath,
                          const QString& projectName,
                          QString* authorName,
                          QString* lastSaveDate,
                          QString* host,
                          qint64* appPID) const
{
    QString realPath = projectPath;
    StrUtils::ensureLastPathSeparator(realPath);
    QString lockFilePath = realPath + projectName + QString::fromUtf8(".lock");
    QFile f(lockFilePath);

    if ( !f.open(QIODevice::ReadOnly) ) {
        return false;
    }
    QTextStream ts(&f);
    if ( !ts.atEnd() ) {
        *lastSaveDate = ts.readLine();
    } else {
        return false;
    }
    if ( !ts.atEnd() ) {
        *authorName = ts.readLine();
    } else {
        return false;
    }
    if ( !ts.atEnd() ) {
        QString pidstr = ts.readLine();
        *appPID = pidstr.toLongLong();
    } else {
        return false;
    }
    // host is optional and was added later
    if ( !ts.atEnd() ) {
        *host = ts.readLine();
    } else {
        *host = tr("unknown host");
    }

    return true;
}

void
Project::removeLastAutosave()
{
    /*
     * First remove the last auto-save registered for this project.
     */
    QString filepath = getLastAutoSaveFilePath();

    if ( !filepath.isEmpty() ) {
        QFile::remove(filepath);
    }

    /*
     * Since we may have saved the project to an old project, overwriting the existing file, there might be
     * a oldProject.ntp.autosave file next to it that belonged to the old project, make sure it gets removed too
     */
    QString projectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );
    QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );
    StrUtils::ensureLastPathSeparator(projectPath);
    QString autoSaveFilePath = projectPath + projectFilename + QString::fromUtf8(".autosave");
    if ( QFile::exists(autoSaveFilePath) ) {
        QFile::remove(autoSaveFilePath);
    }
}

void
Project::clearAutoSavesDir()
{
    /*removing all autosave files*/
    QDir savesDir( autoSavesDir() );
    QStringList entries = savesDir.entryList();

    Q_FOREACH(const QString &entry, entries) {
        ///Do not remove the RENDER_SAVE used by the background processes to render because otherwise they may fail to start rendering.
        /// @see AppInstance::startWritersRendering
        if ( entry.contains( QString::fromUtf8("RENDER_SAVE") ) ) {
            continue;
        }

        QString searchStr( QLatin1Char('.') );
        searchStr.append( QString::fromUtf8(NATRON_PROJECT_FILE_EXT) );
        searchStr.append( QLatin1Char('.') );
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QString dirToRemove = savesDir.path();
            if ( !dirToRemove.endsWith( QLatin1Char('/') ) ) {
                dirToRemove += QLatin1Char('/');
            }
            dirToRemove += entry;
            QFile::remove(dirToRemove);
        }
    }
}

QString
Project::autoSavesDir()
{
    QString str = StandardPaths::writableLocation(StandardPaths::eStandardLocationData);
    StrUtils::ensureLastPathSeparator(str);

    str += QString::fromUtf8("Autosaves");

    return str;
}

void
Project::resetProject()
{
    reset(false, true);
    if ( !getApp()->isBackground() ) {
        createViewer();
    }
}

class ResetWatcherArgs
    : public GenericWatcherCallerArgs
{
public:

    bool aboutToQuit;

    ResetWatcherArgs()
        : GenericWatcherCallerArgs()
        , aboutToQuit(false)
    {
    }

    virtual ~ResetWatcherArgs()
    {
    }
};

typedef boost::shared_ptr<ResetWatcherArgs> ResetWatcherArgsPtr;

void
Project::reset(bool aboutToQuit, bool blocking)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker k(&_imp->projectClosingMutex);
        _imp->projectClosing = true;
    }

    if (!blocking) {
        ResetWatcherArgsPtr args = boost::make_shared<ResetWatcherArgs>();
        args->aboutToQuit = aboutToQuit;
        if ( !quitAnyProcessingForAllNodes(this, args) ) {
            doResetEnd(aboutToQuit);
        }
    } else {
        {
            NodesList nodesToWatch;
            getNodes_recursive(nodesToWatch, false);
            for (NodesList::const_iterator it = nodesToWatch.begin(); it != nodesToWatch.end(); ++it) {
                (*it)->quitAnyProcessing_blocking(false);
            }
        }
        doResetEnd(aboutToQuit);
    }
} // Project::reset

void
Project::closeProject_blocking(bool aboutToQuit)
{
    assert( QThread::currentThread() == qApp->thread() );
    {
        QMutexLocker k(&_imp->projectClosingMutex);
        _imp->projectClosing = true;
    }
    NodesList nodesToWatch;
    getNodes_recursive(nodesToWatch, false);
    for (NodesList::iterator it = nodesToWatch.begin(); it != nodesToWatch.end(); ++it) {
        (*it)->quitAnyProcessing_blocking(false);
    }

    doResetEnd(aboutToQuit);
}

void
Project::doResetEnd(bool aboutToQuit)
{
    _imp->runOnProjectCloseCallback();

    QString lockFilePath = getLockAbsoluteFilePath();
    QString projectPath = QString::fromUtf8( _imp->getProjectPath().c_str() );
    QString projectFilename = QString::fromUtf8( _imp->getProjectFilename().c_str() );
    ///Remove the lock file if we own it
    if ( QFile::exists(lockFilePath) ) {
        QString author;
        QString lastsave;
        QString host;
        qint64 pid;
        if ( getLockFileInfos(projectPath, projectFilename, &author, &lastsave, &host, &pid) ) {
            if ( pid == QCoreApplication::applicationPid() ) {
                QFile::remove(lockFilePath);
            }
        }
    }


    if (aboutToQuit) {
        clearNodesBlocking();
    } else {
        clearNodesNonBlocking();
    }


    if (!aboutToQuit) {
        {
            QMutexLocker l(&_imp->projectLock);
            _imp->autoSetProjectFormat = appPTR->getCurrentSettings()->isAutoProjectFormatEnabled();
            _imp->hasProjectBeenSavedByUser = false;
            _imp->projectCreationTime = QDateTime::currentDateTime();
            _imp->setProjectFilename(NATRON_PROJECT_UNTITLED);
            _imp->setProjectPath("");
            _imp->autoSaveTimer->stop();
            _imp->additionalFormats.clear();
        }
        getApp()->removeAllKeyframesIndicators();

        Q_EMIT projectNameChanged(QString::fromUtf8(NATRON_PROJECT_UNTITLED), false);
        const KnobsVec & knobs = getKnobs();

        beginChanges();
        for (U32 i = 0; i < knobs.size(); ++i) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                knobs[i]->resetToDefaultValue(j);
            }
        }


        onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(), true);

        endChanges(true);
    }

    {
        QMutexLocker k(&_imp->projectClosingMutex);
        _imp->projectClosing = false;
    }
} // Project::doResetEnd

bool
Project::quitAnyProcessingForAllNodes(AfterQuitProcessingI* receiver,
                                      const GenericWatcherCallerArgsPtr& args)
{
    NodesList nodesToWatch;

    getNodes_recursive(nodesToWatch, false);
    if ( nodesToWatch.empty() ) {
        return false;
    }
    NodeRenderWatcherPtr renderWatcher = boost::make_shared<NodeRenderWatcher>(nodesToWatch);
    QObject::connect(renderWatcher.get(), SIGNAL(taskFinished(int,GenericWatcherCallerArgsPtr)), this, SLOT(onQuitAnyProcessingWatcherTaskFinished(int,GenericWatcherCallerArgsPtr)), Qt::UniqueConnection);
    ProjectPrivate::RenderWatcher p;
    p.receiver = receiver;
    p.watcher = renderWatcher;
    _imp->renderWatchers.push_back(p);
    renderWatcher->scheduleBlockingTask(NodeRenderWatcher::eBlockingTaskQuitAnyProcessing, args);

    return true;
}

void
Project::onQuitAnyProcessingWatcherTaskFinished(int taskID,
                                                const GenericWatcherCallerArgsPtr& args)
{
    NodeRenderWatcher* watcher = dynamic_cast<NodeRenderWatcher*>( sender() );

    assert(watcher);
    if (!watcher) {
        return;
    }
    assert(taskID == (int)NodeRenderWatcher::eBlockingTaskQuitAnyProcessing);
    Q_UNUSED(taskID);

    std::list<ProjectPrivate::RenderWatcher>::iterator found = _imp->renderWatchers.end();
    for (std::list<ProjectPrivate::RenderWatcher>::iterator it = _imp->renderWatchers.begin(); it != _imp->renderWatchers.end(); ++it) {
        if (it->watcher.get() == watcher) {
            found = it;
            break;
        }
    }
    assert( found != _imp->renderWatchers.end() );

    if ( found != _imp->renderWatchers.end() ) {
        AfterQuitProcessingI* receiver = found->receiver;
        assert(receiver);
        // Erase before calling the callback, because the callback might destroy this object
        _imp->renderWatchers.erase(found);
        receiver->afterQuitProcessingCallback(args);
    }
}

void
Project::afterQuitProcessingCallback(const GenericWatcherCallerArgsPtr& args)
{
    ResetWatcherArgs* inArgs = dynamic_cast<ResetWatcherArgs*>( args.get() );

    if (inArgs) {
        doResetEnd(inArgs->aboutToQuit);
    }
}

bool
Project::isAutoSetProjectFormatEnabled() const
{
    QMutexLocker l(&_imp->formatMutex);

    return _imp->autoSetProjectFormat;
}

void
Project::setAutoSetProjectFormatEnabled(bool enabled)
{
    QMutexLocker l(&_imp->formatMutex);

    _imp->autoSetProjectFormat = enabled;
}

void
Project::setOrAddProjectFormat(const Format & frmt,
                               bool skipAdd)
{
    if ( frmt.isNull() ) {
        return;
    }

    bool mustRefreshNodeFormats = false;
    Format dispW;
    {
        QMutexLocker l(&_imp->formatMutex);

        if (_imp->autoSetProjectFormat) {
            _imp->autoSetProjectFormat = false;
            dispW = frmt;

            Format df = appPTR->findExistingFormat( dispW.width(), dispW.height(), dispW.getPixelAspectRatio() );
            if ( !df.isNull() ) {
                dispW.setName( df.getName() );
                setProjectDefaultFormat(dispW);
            } else {
                setProjectDefaultFormat(dispW);
            }
        } else if (!skipAdd) {
            dispW = frmt;
            bool existed = false;
            tryAddProjectFormat(dispW, &existed);
            if (!existed) {
                mustRefreshNodeFormats = true;
            }
        }
    }
    if (mustRefreshNodeFormats) {
        // Refresh nodes with a format parameter
        NodesList nodes;
        getNodes_recursive(nodes, true);
        int index = _imp->formatKnob->getValue();
        std::vector<ChoiceOption> entries = _imp->formatKnob->getEntries_mt_safe();
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            (*it)->refreshFormatParamChoice(entries, index, false);
        }

    }
}

bool
Project::tryLock() const
{
    return _imp->projectLock.tryLock();
}

void
Project::unlock() const
{
    assert( !_imp->projectLock.tryLock() );
    _imp->projectLock.unlock();
}

qint64
Project::getProjectCreationTime() const
{
    QMutexLocker l(&_imp->projectLock);

    return _imp->projectCreationTime.toMSecsSinceEpoch();
}

inline ViewerColorSpaceEnum colorspaceParamIndexToEnum(int index)
{
    switch (index) {
        case 0:
            return eViewerColorSpaceLinear;
        case 1:
            return eViewerColorSpaceSRGB;
        case 2:
            return eViewerColorSpaceRec709;
        default:
            return eViewerColorSpaceLinear;
    }
}

ViewerColorSpaceEnum
Project::getDefaultColorSpaceForBitDepth(ImageBitDepthEnum bitdepth) const
{
    switch (bitdepth) {
    case eImageBitDepthByte:

        return colorspaceParamIndexToEnum(_imp->colorSpace8u->getValue());
    case eImageBitDepthShort:

        return colorspaceParamIndexToEnum(_imp->colorSpace16u->getValue());
    case eImageBitDepthHalf: // same colorspace as float
    case eImageBitDepthFloat:

        return colorspaceParamIndexToEnum(_imp->colorSpace32f->getValue());
    case eImageBitDepthNone:
        assert(false);
        break;
    }

    return eViewerColorSpaceLinear;
}

// Functions to escape / unescape characters from UTF-8 XML strings.
// Note that the unescape function matches the escape function,
// and cannot decode any HTML entity (such as Unicode chars).
// A far more complete decoding function can be found at:
// https://bitbucket.org/cggaertner/cstuff/src/master/entities.c
std::string
Project::escapeXML(const std::string &istr)
{
    std::string str( istr );

    for (size_t i = 0; i < str.size(); ++i) {
        switch (str[i]) {
        case '<':
            str.replace(i, 1, "&lt;");
            i += 3;
            break;

        case '>':
            str.replace(i, 1, "&gt;");
            i += 3;
            break;
        case '&':
            str.replace(i, 1, "&amp;");
            i += 4;
            break;
        case '"':
            str.replace(i, 1, "&quot;");
            i += 5;
            break;
        case '\'':
            str.replace(i, 1, "&apos;");
            i += 5;
            break;
        default: {
            unsigned char c = (unsigned char)(str[i]);
            // Escape even the whitespace characters '\n' '\r' '\t', although they are valid
            // XML, because they would be converted to space when re-read.
            // See http://www.w3.org/TR/xml/#AVNormalize
            if ( ( ( 0x01 <= c) && ( c <= 0x1f) ) || ( ( 0x7F <= c) && ( c <= 0x9F) ) ) {
                // these characters must be escaped in XML 1.1
                // http://www.w3.org/TR/xml/#sec-references
                std::string ns = "&#x";
                if (c > 0xf) {
                    int d = c / 0x10;
                    ns += ('0' + d);     // d cannot be more than 9 (because c <= 0x9F)
                }
                int d = c & 0xf;
                ns += d < 10 ? ('0' + d) : ('A' + d - 10);
                ns += ';';
                str.replace(i, 1, ns);
                i += ns.size() + 1;
            } else if (0xC0 <= c && c < 0xE0) { // 2-byte UTF-8
                i += 1;
            } else if (0xE0 <= c && c < 0xF0) { // 3-byte UTF-8
                i += 2;
            } else if (0xF0 <= c && c < 0xF8) { // 4-byte UTF-8
                i += 3;
            }

            break;
        }
        }
    }
    assert( str == OFX::XML::escape(istr) ); // check that this escaped string is consistent with the one in HostSupport
    return str;
} // Project::escapeXML

std::string
Project::unescapeXML(const std::string &istr)
{
    std::string str = istr;

    for (size_t i = 0; i < str.size(); ++i) {
        unsigned char c = (unsigned char)(str[i]);
        if (0xC0 <= c && c < 0xE0) { // 2-byte UTF-8
            i += 1;
        } else if (0xE0 <= c && c < 0xF0) { // 3-byte UTF-8
            i += 2;
        } else if (0xF0 <= c && c < 0xF8) { // 4-byte UTF-8
            i += 3;
        } else if (str[i] == '&') {
            if ( !str.compare(i + 1, 3, "lt;") ) {
                str.replace(i, 4, 1, '<');
            } else if ( !str.compare(i + 1, 3, "gt;") ) {
                str.replace(i, 4, 1, '>');
            } else if ( !str.compare(i + 1, 4, "amp;") ) {
                str.replace(i, 5, 1, '&');
            } else if ( !str.compare(i + 1, 5, "apos;") ) {
                str.replace(i, 6, 1, '\'');
            } else if ( !str.compare(i + 1, 5, "quot;") ) {
                str.replace(i, 6, 1, '"');
            } else if ( !str.compare(i + 1, 1, "#") ) {
                size_t end = str.find_first_of(";", i + 2);
                if (end == std::string::npos) {
                    // malformed XML
                    return str;
                }
                char *tail = NULL;
                int errno_save = errno;
                bool hex = str[i + 2] == 'x' || str[i + 2] == 'X';
                int prefix = hex ? 3 : 2; // prefix length: "&#" or "&#x"
                char *head = &str[i + prefix];

                errno = 0;
                unsigned long cp = std::strtoul(head, &tail, hex ? 16 : 10);
                bool fail = errno || (tail - &str[0]) != (long)end || cp > 0xff; // only handle 0x01-0xff
                errno = errno_save;
                if (fail) {
                    return str;
                }
                // replace from '&' to ';' (thus the +1)
                str.replace(i, tail - head + 1 + prefix, 1, (char)cp);
            }
        }
    }

    return str;
}

void
Project::getEnvironmentVariables(std::map<std::string, std::string>& env) const
{
    std::list<std::vector<std::string> > table;

    _imp->envVars->getTable(&table);
    for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
        assert(it->size() == 2);
        env[(*it)[0]] = (*it)[1];
    }
}

void
Project::expandVariable(const std::map<std::string, std::string>& env,
                        std::string& str)
{
    ///Loop while we can still expand variables, up to NATRON_PROJECT_ENV_VAR_MAX_RECURSION recursions
    for (int i = 0; i < NATRON_PROJECT_ENV_VAR_MAX_RECURSION; ++i) {
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            if ( expandVariable(it->first, it->second, str) ) {
                break;
            }
        }
    }
}

bool
Project::expandVariable(const std::string& varName,
                        const std::string& varValue,
                        std::string& str)
{
    if ( ( str.size() > (varName.size() + 2) ) && ///can contain the environment variable name
         ( str[0] == '[') && /// env var name is bracketed
         ( str.substr( 1, varName.size() ) == varName) && /// starts with the environment variable name
         ( str[varName.size() + 1] == ']') ) { /// env var name is bracketed
        str.erase(str.begin() + varName.size() + 1);
        str.erase( str.begin() );
        str.replace(0, varName.size(), varValue);

        return true;
    }

    return false;
}

void
Project::findReplaceVariable(const std::map<std::string, std::string>& env,
                             std::string& str)
{
    std::string longestName;
    std::string longestVar;

    for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
        if ( ( str.size() >= it->second.size() ) &&
             ( it->second.size() > longestVar.size() ) &&
             ( str.substr( 0, it->second.size() ) == it->second) ) {
            longestName = it->first;
            longestVar = it->second;
        }
    }
    if ( !longestName.empty() && !longestVar.empty() ) {
        std::string replaceStr;
        replaceStr += '[';
        replaceStr += longestName;
        replaceStr += ']';
        str.replace(0, longestVar.size(), replaceStr);
    }
}

void
Project::makeRelativeToVariable(const std::string& varName,
                                const std::string& varValue,
                                std::string& str)
{
    bool hasTrailingSep = !varValue.empty() && (varValue[varValue.size() - 1] == '/' || varValue[varValue.size() - 1] == '\\');

    if ( ( str.size() > varValue.size() ) && (str.substr( 0, varValue.size() ) == varValue) ) {
        if (hasTrailingSep) {
            str = '[' + varName + ']' + str.substr( varValue.size(), str.size() );
        } else {
            str = '[' + varName + "]/" + str.substr( varValue.size() + 1, str.size() );
        }
    } else {
        QDir dir( QString::fromUtf8( varValue.c_str() ) );
        QString relative = dir.relativeFilePath( QString::fromUtf8( str.c_str() ) );
        if (hasTrailingSep) {
            str = '[' + varName + ']' + relative.toStdString();
        } else {
            str = '[' + varName + "]/" + relative.toStdString();
        }
    }
}

bool
Project::fixFilePath(const std::string& projectPathName,
                     const std::string& newProjectPath,
                     std::string& filePath)
{
    if ( ( filePath.size() < (projectPathName.size() + 2) ) //< filepath doesn't have enough space to  contain the variable
         || ( filePath[0] != '[')
         || ( filePath[projectPathName.size() + 1] != ']')
         || ( filePath.substr( 1, projectPathName.size() ) != projectPathName) ) {
        return false;
    }

    canonicalizePath(filePath);

    if ( newProjectPath.empty() ) {
        return true; //keep it absolute if the variables points to nothing
    } else {
        QDir dir( QString::fromUtf8( newProjectPath.c_str() ) );
        if ( !dir.exists() ) {
            return false;
        }

        filePath = dir.relativeFilePath( QString::fromUtf8( filePath.c_str() ) ).toStdString();
        if (newProjectPath[newProjectPath.size() - 1] == '/') {
            filePath = '[' + projectPathName + ']' + filePath;
        } else {
            filePath = '[' + projectPathName + "]/" + filePath;
        }

        return true;
    }
}

bool
Project::isRelative(const std::string& str)
{
#ifdef __NATRON_WIN32__

    return ( str.empty() || ( !str.empty() && (str[0] != '/')
                              && ( !(str.size() >= 2 && str[1] == ':') ) ) );
#else  //Unix
    return ( str.empty() || (str[0] != '/') );
#endif
}

void
Project::canonicalizePath(std::string& str)
{
    std::map<std::string, std::string> envvar;

    getEnvironmentVariables(envvar);

    expandVariable(envvar, str);

    if ( !str.empty() ) {
        ///Now check if the string is relative

        if ( isRelative(str) ) {
            ///If it doesn't start with an env var but is relative, prepend the project env var
            std::map<std::string, std::string>::iterator foundProject = envvar.find(NATRON_PROJECT_ENV_VAR_NAME);
            if ( foundProject != envvar.end() ) {
                if ( foundProject->second.empty() ) {
                    return;
                }
                const char& c = foundProject->second[foundProject->second.size() - 1];
                bool addTrailingSlash = c != '/' && c != '\\';
                std::string copy = foundProject->second;
                if (addTrailingSlash) {
                    copy += '/';
                }
                copy += str;
                str = copy;
            }
        }

        ///Canonicalize
        QFileInfo info( QString::fromUtf8( str.c_str() ) );
        QString canonical =  info.canonicalFilePath();
        if ( canonical.isEmpty() ) {
            return;
        } else {
            str = canonical.toStdString();
        }
    }
}

void
Project::simplifyPath(std::string& str)
{
    std::map<std::string, std::string> envvar;

    getEnvironmentVariables(envvar);

    Project::findReplaceVariable(envvar, str);
}

void
Project::makeRelativeToProject(std::string& str)
{
    std::map<std::string, std::string> envvar;

    getEnvironmentVariables(envvar);

    std::map<std::string, std::string>::iterator found = envvar.find(NATRON_PROJECT_ENV_VAR_NAME);
    if ( ( found != envvar.end() ) && !found->second.empty() ) {
        makeRelativeToVariable(found->first, found->second, str);
    }
}

void
Project::onOCIOConfigPathChanged(const std::string& path,
                                 bool block)
{
    beginChanges();

    try {
        std::string oldEnv;
        try {
            oldEnv = _imp->envVars->getValue();
        } catch (...) {
            // ignore
        }
        std::list<std::vector<std::string> > table;
        _imp->envVars->decodeFromKnobTableFormat(oldEnv, &table);

        ///If there was already a OCIO variable, update it, otherwise create it
        bool found = false;
        for (std::list<std::vector<std::string> >::iterator it = table.begin(); it != table.end(); ++it) {
            if ( (*it)[0] == NATRON_OCIO_ENV_VAR_NAME ) {
                (*it)[1] = path;
                found = true;
                break;
            }
        }
        if (!found) {
            std::vector<std::string> vec(2);
            vec[0] = NATRON_OCIO_ENV_VAR_NAME;
            vec[1] = path;
            table.push_back(vec);
        }

        std::string newEnv = _imp->envVars->encodeToKnobTableFormat(table);

        if (oldEnv != newEnv) {
            if ( appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled() ) {
                fixRelativeFilePaths(NATRON_OCIO_ENV_VAR_NAME, path, block);
            }
            _imp->envVars->setValue(newEnv);
        }
        endChanges(block);
    } catch (std::logic_error&) {
        // ignore
    }
}

double
Project::getProjectFrameRate() const
{
    return _imp->frameRate->getValue();
}

KnobPathPtr
Project::getEnvVarKnob() const
{
    return _imp->envVars;
}

std::string
Project::getOnProjectLoadCB() const
{
    return _imp->onProjectLoadCB->getValue();
}

std::string
Project::getOnProjectSaveCB() const
{
    return _imp->onProjectSaveCB->getValue();
}

std::string
Project::getOnProjectCloseCB() const
{
    return _imp->onProjectCloseCB->getValue();
}

std::string
Project::getOnNodeCreatedCB() const
{
    return _imp->onNodeCreated->getValue();
}

std::string
Project::getOnNodeDeleteCB() const
{
    return _imp->onNodeDeleted->getValue();
}

bool
Project::isProjectClosing() const
{
    QMutexLocker k(&_imp->projectClosingMutex);

    return _imp->projectClosing;
}

bool
Project::isFrameRangeLocked() const
{
    return _imp->lockFrameRange->getValue();
}

void
Project::getFrameRange(double* first,
                       double* last) const
{
    *first = _imp->frameRange->getValue(0);
    *last = _imp->frameRange->getValue(1);
}

void
Project::unionFrameRangeWith(int first,
                             int last)
{
    int curFirst, curLast;
    bool mustSet = !_imp->frameRange->hasModifications() && first != last;

    curFirst = _imp->frameRange->getValue(0);
    curLast = _imp->frameRange->getValue(1);
    curFirst = !mustSet ? std::min(first, curFirst) : first;
    curLast = !mustSet ? std::max(last, curLast) : last;
    beginChanges();
    _imp->frameRange->setValue(curFirst, ViewSpec::all(), 0);
    _imp->frameRange->setValue(curLast, ViewSpec::all(), 1);
    endChanges();
}

void
Project::recomputeFrameRangeFromReaders()
{
    int first = 1, last = 1;

    recomputeFrameRangeForAllReaders(&first, &last);

    beginChanges();
    _imp->frameRange->setValue(first, ViewSpec::all(), 0);
    _imp->frameRange->setValue(last,  ViewSpec::all(), 1);
    endChanges();
}

void
Project::createViewer()
{
    if ( getApp()->isBackground() ) {
        return;
    }

    CreateNodeArgs args( PLUGINID_NATRON_VIEWER, shared_from_this() );
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    getApp()->createNode(args);
}

static bool
hasNodeOutputsInList(const NodesList& nodes,
                     const NodePtr& node)
{
    const NodesWList& outputs = node->getGuiOutputs();
    bool foundOutput = false;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                if (it2->lock() == *it) {
                    foundOutput = true;
                    break;
                }
            }
            if (foundOutput) {
                break;
            }
        }
    }

    return foundOutput;
}

static bool
hasNodeInputsInList(const NodesList& nodes,
                    const NodePtr& node)
{
    const std::vector<NodeWPtr>& inputs = node->getGuiInputs();
    bool foundInput = false;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            for (std::size_t i = 0; i < inputs.size(); ++i) {
                NodePtr input = inputs[i].lock();
                if (input == *it) {
                    foundInput = true;
                    break;
                }
            }
            if (foundInput) {
                break;
            }
        }
    }

    return foundInput;
}

static void
addTreeInputs(const NodesList& nodes,
              const NodePtr& node,
              Project::NodesTree& tree,
              NodesList& markedNodes)
{
    if ( std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end() ) {
        return;
    }

    if ( std::find(nodes.begin(), nodes.end(), node) == nodes.end() ) {
        return;
    }

    if ( !hasNodeInputsInList(nodes, node) ) {
        Project::TreeInput input;
        input.node = node;
        const std::vector<NodeWPtr>& inputs = node->getGuiInputs();
        input.inputs.resize( inputs.size() );
        for (std::size_t i = 0; i < inputs.size(); ++i) {
            input.inputs[i] = inputs[i].lock();
        }
        tree.inputs.push_back(input);
        markedNodes.push_back(node);
    } else {
        tree.inbetweenNodes.push_back(node);
        markedNodes.push_back(node);
        const std::vector<NodeWPtr>& inputs = node->getGuiInputs();
        for (std::vector<NodeWPtr>::const_iterator it2 = inputs.begin(); it2 != inputs.end(); ++it2) {
            NodePtr input = it2->lock();
            if (input) {
                addTreeInputs(nodes, input, tree, markedNodes);
            }
        }
    }
}

void
Project::extractTreesFromNodes(const NodesList& nodes,
                               std::list<Project::NodesTree>& trees)
{
    NodesList markedNodes;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isOutput = !hasNodeOutputsInList(nodes, *it);
        if (isOutput) {
            NodesTree tree;
            tree.output.node = *it;
            const NodesWList& outputs = (*it)->getGuiOutputs();
            for (NodesWList::const_iterator it2 = outputs.begin(); it2 != outputs.end(); ++it2) {
                NodePtr output = it2->lock();
                if (!output) {
                    continue;
                }
                int idx = output->inputIndex(*it);
                tree.output.outputs.push_back( std::make_pair(idx, *it2) );
            }

            const std::vector<NodeWPtr>& inputs = (*it)->getGuiInputs();
            for (U32 i = 0; i < inputs.size(); ++i) {
                NodePtr input = inputs[i].lock();
                if (input) {
                    addTreeInputs(nodes, input, tree, markedNodes);
                }
            }

            if ( tree.inputs.empty() ) {
                TreeInput input;
                input.node = *it;

                const std::vector<NodeWPtr>& inputs = (*it)->getGuiInputs();
                input.inputs.resize( inputs.size() );
                for (std::size_t i = 0; i < inputs.size(); ++i) {
                    input.inputs[i] = inputs[i].lock();
                }
                tree.inputs.push_back(input);
            }

            trees.push_back(tree);
        }
    }
}

bool
Project::addFormat(const std::string& formatSpec)
{
    Format f;

    if ( ProjectPrivate::generateFormatFromString(QString::fromUtf8( formatSpec.c_str() ), &f) ) {
        QMutexLocker k(&_imp->formatMutex);
        bool existed;
        tryAddProjectFormat(f, &existed);

        return true;
    } else {
        return false;
    }
}

void
Project::onProjectFormatPopulated()
{
    int index = _imp->formatKnob->getValue();
    NodesList nodes;

    getNodes_recursive(nodes, true);
    std::vector<ChoiceOption> entries = _imp->formatKnob->getEntries_mt_safe();
    for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->refreshFormatParamChoice(entries, index, false);
    }
}

void
Project::setTimeLine(const TimeLinePtr& timeline)
{
    _imp->timeline = timeline;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Project.cpp"
