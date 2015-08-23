//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "Project.h"

#include <fstream>
#include <algorithm> // min, max
#include <ios>
#include <cstdlib> // strtoul
#include <cerrno> // errno
#include <stdexcept>

#ifdef __NATRON_WIN32__
#include <stdio.h> //for _snprintf
#include <windows.h> //for GetUserName
#include <Lmcons.h> //for UNLEN
#define snprintf _snprintf
#elif defined(__NATRON_UNIX__)
#include <pwd.h> //for getpwuid
#endif

#include <QtConcurrentRun>
#include <QCoreApplication>
#include <QTimer>
#include <QThreadPool>
#include <QDir>
#include <QTemporaryFile>
#include <QHostInfo>
#include <QFileInfo>
#include <QDebug>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/EffectInstance.h"
#include "Engine/Hash64.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/StandardPaths.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/RectISerialization.h"
#include "Engine/RectDSerialization.h"

using std::cout; using std::endl;
using std::make_pair;


static std::string getUserName()
{
#ifdef __NATRON_WIN32__
    char user_name[UNLEN+1];
    DWORD user_name_size = sizeof(user_name);
    GetUserName(user_name, &user_name_size);
    return user_name;
#elif defined(__NATRON_UNIX__)
    struct passwd *passwd;
    passwd = getpwuid( getuid() );
    return passwd->pw_name;
#endif
}

static std::string generateGUIUserName()
{
    return getUserName() + '@' + QHostInfo::localHostName().toStdString();
}

static std::string generateUserFriendlyNatronVersionName()
{
    std::string ret(NATRON_APPLICATION_NAME);
    ret.append(" v");
    ret.append(NATRON_VERSION_STRING);
    const std::string status(NATRON_DEVELOPMENT_STATUS);
    ret.append(" " NATRON_DEVELOPMENT_STATUS);
    if (status == NATRON_DEVELOPMENT_RELEASE_CANDIDATE) {
        ret.append(QString::number(NATRON_BUILD_NUMBER).toStdString());
    }
    return ret;
}

namespace Natron {
Project::Project(AppInstance* appInstance)
    : KnobHolder(appInstance)
    , NodeCollection(appInstance)
    , _imp( new ProjectPrivate(this) )
{
    QObject::connect( _imp->autoSaveTimer.get(), SIGNAL( timeout() ), this, SLOT( onAutoSaveTimerTriggered() ) );
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
    
class LoadProjectSplashScreen_RAII
{
    AppInstance* app;
public:
    
    LoadProjectSplashScreen_RAII(AppInstance* app,const QString& filename)
    : app(app)
    {
        app->createLoadProjectSplashScreen(filename);
    }
    
    ~LoadProjectSplashScreen_RAII()
    {
        app->closeLoadPRojectSplashScreen();
    }
};

  
bool
Project::loadProject(const QString & path,
                     const QString & name,
                     bool isUntitledAutosave)
{

    reset(false);

    try {
        QString realPath = path;
        if (!realPath.endsWith('/')) {
            realPath.push_back('/');
        }
        QString realName = name;
        
        bool isAutoSave = isUntitledAutosave;
        if (!appPTR->isBackground() && !isUntitledAutosave) {
            // In Gui mode, attempt to load an auto-save for this project if there's one.
            QString autosaveFileName;
            bool hasAutoSave = findAutoSaveForProject(realPath, name,&autosaveFileName);
            if (hasAutoSave) {
                QString text = tr("A recent auto-save of %1 was found.\n"
                                  "Would you like to use it instead? "
                                  "Clicking No will remove this auto-save.").arg(name);
                Natron::StandardButtonEnum ret = Natron::questionDialog(tr("Auto-save").toStdString(),
                                                                        text.toStdString(),false, Natron::StandardButtons(Natron::eStandardButtonYes | Natron::eStandardButtonNo),
                                                                        Natron::eStandardButtonYes);
                if ( (ret == Natron::eStandardButtonNo) || (ret == Natron::eStandardButtonEscape) ) {
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
        if (!loadProjectInternal(realPath,realName,isAutoSave,isUntitledAutosave,&mustSave)) {
            appPTR->showOfxLog();
        } else if (mustSave) {
            saveProject(realPath, realName, false);
        }
    } catch (const std::exception & e) {
        Natron::errorDialog( QObject::tr("Project loader").toStdString(), QObject::tr("Error while loading project").toStdString() + ": " + e.what() );
        if ( !appPTR->isBackground() ) {
            getApp()->createNode(  CreateNodeArgs(PLUGINID_NATRON_VIEWER,
                                                  "",
                                                  -1,-1,
                                                  true,
                                                  INT_MIN,INT_MIN,
                                                  true,
                                                  true,
                                                  false,
                                                  QString(),
                                                  CreateNodeArgs::DefaultValuesList(),
                                                  shared_from_this()) );
        }

        return false;
    } catch (...) {

        Natron::errorDialog( QObject::tr("Project loader").toStdString(), QObject::tr("Unkown error while loading project").toStdString() );
        if ( !appPTR->isBackground() ) {
            getApp()->createNode(  CreateNodeArgs(PLUGINID_NATRON_VIEWER,
                                                  "",
                                                  -1,-1,
                                                  true,
                                                  INT_MIN,INT_MIN,
                                                  true,
                                                  true,
                                                  false,
                                                  QString(),
                                                  CreateNodeArgs::DefaultValuesList(),
                                                  shared_from_this()) );
        }

        return false;
    }

    refreshViewersAndPreviews();
    return true;
} // loadProject

bool
Project::loadProjectInternal(const QString & path,
                             const QString & name,bool isAutoSave,bool isUntitledAutosave, bool* mustSave)
{
    
    Natron::FlagSetter loadingProjectRAII(true,&_imp->isLoadingProject,&_imp->isLoadingProjectMutex);
    
    QString filePath = path + name;
    qDebug() << "Loading project" << filePath;
    if ( !QFile::exists(filePath) ) {
        throw std::invalid_argument( QString(filePath + " : no such file.").toStdString() );
    }
    
    bool ret = false;
    std::ifstream ifile;
    try {
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filePath.toStdString().c_str(),std::ifstream::in);
    } catch (const std::ifstream::failure & e) {
        throw std::runtime_error( std::string("Exception occured when opening file ") + filePath.toStdString() + ": " + e.what() );
    }
    
    if (NATRON_VERSION_MAJOR == 1 && NATRON_VERSION_MINOR == 0 && NATRON_VERSION_REVISION == 0) {
        
        ///Try to determine if the project was made during Natron v1.0.0 - RC2 or RC3 to detect a bug we introduced at that time
        ///in the BezierCP class serialisation
        bool foundV = false;
        QFile f(filePath);
        f.open(QIODevice::ReadOnly);
        QTextStream fs(&f);
        while (!fs.atEnd()) {
            
            QString line = fs.readLine();

            if (line.indexOf("Natron v1.0.0 RC2") != -1 || line.indexOf("Natron v1.0.0 RC3") != -1) {
                appPTR->setProjectCreatedDuringRC2Or3(true);
                foundV = true;
                break;
            }
        }
        if (!foundV) {
            appPTR->setProjectCreatedDuringRC2Or3(false);
        }
    }
    
    LoadProjectSplashScreen_RAII __raii_splashscreen__(getApp(),name);
    
    try {
        bool bgProject;
        boost::archive::xml_iarchive iArchive(ifile);
        {
            FlagSetter __raii_loadingProjectInternal__(true,&_imp->isLoadingProjectInternal,&_imp->isLoadingProjectMutex);
            
            iArchive >> boost::serialization::make_nvp("Background_project", bgProject);
            ProjectSerialization projectSerializationObj( getApp() );
            iArchive >> boost::serialization::make_nvp("Project", projectSerializationObj);
            
            ret = load(projectSerializationObj,name,path, mustSave);
        } // __raii_loadingProjectInternal__
        
        if (!bgProject) {
            getApp()->loadProjectGui(iArchive);
        }
    } catch (const boost::archive::archive_exception & e) {
        ifile.close();
        throw std::runtime_error( e.what() );
    } catch (const std::ios_base::failure& e) {
        ifile.close();
        getApp()->endProgress(this);
        throw std::runtime_error( std::string("Failed to read the project file: I/O failure (") + e.what() + ")");
    } catch (const std::exception & e) {
        ifile.close();
        throw std::runtime_error( std::string("Failed to read the project file: ") + e.what() );
    } catch (...) {
        ifile.close();
        getApp()->endProgress(this);
        throw std::runtime_error("Failed to read the project file");
    }

    ifile.close();
    
    Format f;
    getProjectDefaultFormat(&f);
    Q_EMIT formatChanged(f);
    
    _imp->natronVersion->setValue(generateUserFriendlyNatronVersionName(),0);
    if (isAutoSave) {
        _imp->autoSetProjectFormat = false;
        if (!isUntitledAutosave) {
            QString projectName(_imp->getProjectFilename().c_str());
            int found = projectName.lastIndexOf(".autosave");
            if (found != -1) {
                _imp->setProjectFilename(projectName.left(found).toStdString());
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
        
        QString projectName(_imp->getProjectFilename().c_str());
        Q_EMIT projectNameChanged(projectName + " (*)");

    } else {
        Q_EMIT projectNameChanged(name);
    }
    
    ///Try to take the project lock by creating a lock file
    if (!isAutoSave) {
        QString lockFilePath = getLockAbsoluteFilePath();
        if (!QFile::exists(lockFilePath)) {
            createLockFile();
        }
    }
    
    _imp->runOnProjectLoadCallback();

    ///Process all events before flagging that we're no longer loading the project
    ///to avoid multiple renders being called because of reshape events of viewers
    QCoreApplication::processEvents();
    
    return ret;
}
    
    
QString
Project::saveProject(const QString & path,
                     const QString & name,
                     bool autoS)
{
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        if (_imp->isLoadingProject) {
            return QString();
        }
    }

    {
        QMutexLocker l(&_imp->isSavingProjectMutex);
        if (_imp->isSavingProject) {
            return QString();
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
            ret = saveProjectInternal(path,name);
            
            ///We just saved, remove the last auto-save which is now obsolete
            removeLastAutosave();

            //}
        } else {
            
            ///Replace the last auto-save with a more recent one
            removeLastAutosave();
            
            ret = saveProjectInternal(path,name,true);
        }
    } catch (const std::exception & e) {
        if (!autoS) {
            Natron::errorDialog( QObject::tr("Save").toStdString(), e.what() );
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
    
    return ret;
}

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
                             bool autoSave)
{
    
    bool isRenderSave = name.contains("RENDER_SAVE");
    QDateTime time = QDateTime::currentDateTime();
    QString timeStr = time.toString();

    QString filePath;
    if (autoSave) {
        bool appendTimeHash = false;
        if (path.isEmpty()) {
            filePath = autoSavesDir();
            
            //If the auto-save is saved in the AutoSaves directory, there will be other autosaves for other opened
            //projects. We uniquely identity it with the time hash of the current time
            appendTimeHash = true;
        } else {
            filePath = path;
        }
        filePath.append("/");
        filePath.append(name);
        if (!isRenderSave) {
            filePath.append(".autosave");
        }
        if (!isRenderSave) {
            if (appendTimeHash) {
                Hash64 timeHash;
                
                for (int i = 0; i < timeStr.size(); ++i) {
                    timeHash.append<unsigned short>( timeStr.at(i).unicode() );
                }
                timeHash.computeHash();
                QString timeHashStr = QString::number( timeHash.value() );
                filePath.append("." + timeHashStr);
            }
            
        }
        {
            QMutexLocker l(&_imp->projectLock);
            _imp->lastAutoSaveFilePath = filePath;
        }
    } else {
        filePath = path + name;
    }
    
    std::string newFilePath = _imp->runOnProjectSaveCallback(filePath.toStdString(), autoSave);
    filePath = QString(newFilePath.c_str());
    
    ///Use a temporary file to save, so if Natron crashes it doesn't corrupt the user save.
    QString tmpFilename = StandardPaths::writableLocation(StandardPaths::eStandardLocationTemp);
    tmpFilename.append( QDir::separator() );
    tmpFilename.append( QString::number( time.toMSecsSinceEpoch() ) );

    std::ofstream ofile;
    try {
        ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ofile.open(tmpFilename.toStdString().c_str(),std::ofstream::out);
    } catch (const std::ofstream::failure & e) {
        throw std::runtime_error( std::string("Exception occured when opening file ") + tmpFilename.toStdString() + ": " + e.what() );
    }

    if ( !ofile.good() ) {
        qDebug() << "Failed to open file " << tmpFilename.toStdString().c_str();
        throw std::runtime_error( "Failed to open file " + tmpFilename.toStdString() );
    }

    ///Fix file paths before saving.
    QString oldProjectPath(_imp->getProjectPath().c_str());
   
    if (!autoSave) {
        _imp->autoSetProjectDirectory(path);
        _imp->saveDate->setValue(timeStr.toStdString(), 0);
        _imp->lastAuthorName->setValue(generateGUIUserName(), 0);
        _imp->natronVersion->setValue(generateUserFriendlyNatronVersionName(),0);
    }
    
    try {
        boost::archive::xml_oarchive oArchive(ofile);
        bool bgProject = appPTR->isBackground();
        oArchive << boost::serialization::make_nvp("Background_project",bgProject);
        ProjectSerialization projectSerializationObj( getApp() );
        save(&projectSerializationObj);
        oArchive << boost::serialization::make_nvp("Project",projectSerializationObj);
        if (!bgProject) {
            getApp()->saveProjectGui(oArchive);
        }
    } catch (...) {
        ofile.close();
        if (!autoSave) {
            ///Reset the old project path in case of failure.
            _imp->autoSetProjectDirectory(oldProjectPath);
        }
        throw;
    }

    ofile.close();

    QFile::remove(filePath);
    int nAttemps = 0;

    while ( nAttemps < 10 && !fileCopy(tmpFilename, filePath) ) {
        ++nAttemps;
    }

    QFile::remove(tmpFilename);
    
    if (!autoSave) {
        
        QString lockFilePath = getLockAbsoluteFilePath();
        if (QFile::exists(lockFilePath)) {
            ///Remove the previous lock file if there was any
            removeLockFile();
        }
        {
            _imp->setProjectFilename(name.toStdString());
            _imp->setProjectPath(path.toStdString());
            QMutexLocker l(&_imp->projectLock);
            _imp->hasProjectBeenSavedByUser = true;
            _imp->ageSinceLastSave = time;
        }
        Q_EMIT projectNameChanged(name); //< notify the gui so it can update the title

        //Create the lock file corresponding to the project
        createLockFile();
    } else {
        if (!isRenderSave) {
            QString projectName(_imp->getProjectFilename().c_str());
            Q_EMIT projectNameChanged(projectName + " (*)");
        }
    }
    _imp->lastAutoSave = time;

    return filePath;
} // saveProjectInternal

void
Project::autoSave()
{
    ///don't autosave in background mode...
    if ( appPTR->isBackground() ) {
        return;
    }
    
    QString path(_imp->getProjectPath().c_str());
    QString name(_imp->getProjectFilename().c_str());
    saveProject(path, name, true);
}

void
Project::triggerAutoSave()
{
    ///Should only be called in the main-thread, that is upon user interaction.
    assert( QThread::currentThread() == qApp->thread() );

    if ( appPTR->isBackground() || !appPTR->isLoaded() || _imp->projectClosing ) {
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
    assert( !appPTR->isBackground() );

    if (!getApp()) {
        return;
    }
    
    ///check that all schedulers are not working.
    ///If so launch an auto-save, otherwise, restart the timer.
    bool canAutoSave = !hasNodeRendering() && !getApp()->isShowingDialog();

    if (canAutoSave) {
        boost::shared_ptr<QFutureWatcher<void> > watcher(new QFutureWatcher<void>);
        QObject::connect(watcher.get(), SIGNAL(finished()), this, SLOT(onAutoSaveFutureFinished()));
        watcher->setFuture(QtConcurrent::run(this,&Project::autoSave));
        _imp->autoSaveFutures.push_back(watcher);
    } else {
        ///If the auto-save failed because a render is in progress, try every 2 seconds to auto-save.
        ///We don't use the user-provided timeout interval here because it could be an inapropriate value.
        _imp->autoSaveTimer->start(2000);
    }
}
    
void Project::onAutoSaveFutureFinished()
{
    QFutureWatcherBase* future = qobject_cast<QFutureWatcherBase*>(sender());
    assert(future);
    for (std::list<boost::shared_ptr<QFutureWatcher<void> > >::iterator it = _imp->autoSaveFutures.begin(); it != _imp->autoSaveFutures.end(); ++it) {
        if (it->get() == future) {
            _imp->autoSaveFutures.erase(it);
            break;
        }
    }
}
    
bool Project::findAutoSaveForProject(const QString& projectPath,const QString& projectName,QString* autoSaveFileName)
{
    QString projectAbsFilePath = projectPath + projectName;
    QDir savesDir(projectPath);
    QStringList entries = savesDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    
    for (int i = 0; i < entries.size(); ++i) {
        const QString & entry = entries.at(i);
        QString ntpExt(".");
        ntpExt.append(NATRON_PROJECT_FILE_EXT);
        QString searchStr(ntpExt);
        QString autosaveSuffix(".autosave");
        searchStr.append(autosaveSuffix);
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos == -1 || entry.contains("RENDER_SAVE")) {
            continue;
        }
        QString filename = projectPath + entry.left(suffixPos + ntpExt.size());
        if (filename == projectAbsFilePath && QFile::exists(filename)) {
            *autoSaveFileName = entry;
            return true;
        }
        
    }
    return false;
}
    
void
Project::initializeKnobs()
{
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Settings");

    _imp->envVars = Natron::createKnob<Path_Knob>(this, "Project Paths");
    _imp->envVars->setName("projectPaths");
    _imp->envVars->setHintToolTip("Specify here project paths. Any path can be used "
                                  "in file paths and can be used between brackets, for example: \n"
                                  "[" NATRON_PROJECT_ENV_VAR_NAME "]MyProject.ntp \n"
                                  "You can add as many project paths as you want and can name them as you want. This way it "
                                  "makes it easy to share your projects and move files around."
                                  " You can chain up paths, like so:\n "
                                  "[" NATRON_PROJECT_ENV_VAR_NAME "] = <PathToProject> \n"
                                  "[Scene1] = [" NATRON_PROJECT_ENV_VAR_NAME "]/Rush/S_01 \n"
                                  "[Shot1] = [Scene1]/Shot001 \n"
                                  "By default if a file-path is NOT absolute (i.e: not starting with '/' "
                                  " on Unix or a drive name on Windows) "
                                  "then it will be expanded using the [" NATRON_PROJECT_ENV_VAR_NAME "] path. "
                                  "Absolute paths are treated as normal."
                                  " The [" NATRON_PROJECT_ENV_VAR_NAME "] path will be set automatically to "
                                  " the location of the project file when saving and loading a project."
                                  " The [" NATRON_OCIO_ENV_VAR_NAME "] path will also be set automatically for better sharing of projects with reader nodes.");
    _imp->envVars->setSecret(false);
    _imp->envVars->setMultiPath(true);
    
    
    
    ///Initialize the OCIO Config
    onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(),false);
    
    page->addKnob(_imp->envVars);
    
    _imp->formatKnob = Natron::createKnob<Choice_Knob>(this, "Output Format");
    _imp->formatKnob->setHintToolTip("The project output format is what is used as canvas on the viewers.");
    _imp->formatKnob->setName("outputFormat");

    const std::vector<Format*> & appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = Natron::generateStringFromFormat(*f);
        if ( (f->width() == 1920) && (f->height() == 1080) ) {
            _imp->formatKnob->setDefaultValue(i,0);
        }
        entries.push_back( formatStr.toStdString() );
        _imp->builtinFormats.push_back(*f);
    }
    _imp->formatKnob->setAddNewLine(false);

    _imp->formatKnob->populateChoices(entries);
    _imp->formatKnob->setAnimationEnabled(false);
    page->addKnob(_imp->formatKnob);
    _imp->addFormatKnob = Natron::createKnob<Button_Knob>(this, "New Format...");
    _imp->addFormatKnob->setName("newFormat");
    page->addKnob(_imp->addFormatKnob);

    _imp->viewsCount = Natron::createKnob<Int_Knob>(this, "Number of Views");
    _imp->viewsCount->setName("noViews");
    _imp->viewsCount->setAnimationEnabled(false);
    _imp->viewsCount->setMinimum(1);
    _imp->viewsCount->setDisplayMinimum(1);
    _imp->viewsCount->setDefaultValue(1,0);
    _imp->viewsCount->disableSlider();
    _imp->viewsCount->setEvaluateOnChange(false);
    _imp->viewsCount->setAddNewLine(false);
    page->addKnob(_imp->viewsCount);

    _imp->mainView = Natron::createKnob<Int_Knob>(this, "Main View");
    _imp->mainView->setName("mainView");
    _imp->mainView->disableSlider();
    _imp->mainView->setDefaultValue(0);
    _imp->mainView->setMinimum(0);
    _imp->mainView->setMaximum(0);
    _imp->mainView->setAnimationEnabled(false);
    page->addKnob(_imp->mainView);

    _imp->previewMode = Natron::createKnob<Bool_Knob>(this, "Auto Previews");
    _imp->previewMode->setName("autoPreviews");
    _imp->previewMode->setHintToolTip("When checked, preview images on the node graph will be "
                                      "refreshed automatically. You can uncheck this option to improve performances."
                                      "Press P in the node graph to refresh the previews yourself.");
    _imp->previewMode->setAnimationEnabled(false);
    _imp->previewMode->setEvaluateOnChange(false);
    page->addKnob(_imp->previewMode);
    bool autoPreviewEnabled = appPTR->getCurrentSettings()->isAutoPreviewOnForNewProjects();
    _imp->previewMode->setDefaultValue(autoPreviewEnabled,0);

    std::vector<std::string> colorSpaces;
    colorSpaces.push_back("sRGB");
    colorSpaces.push_back("Linear");
    colorSpaces.push_back("Rec.709");
    _imp->colorSpace8u = Natron::createKnob<Choice_Knob>(this, "Colorspace for 8-Bit Integer Images");
    _imp->colorSpace8u->setName("defaultColorSpace8u");
    _imp->colorSpace8u->setHintToolTip("Defines the color-space in which 8-bit images are assumed to be by default.");
    _imp->colorSpace8u->setAnimationEnabled(false);
    _imp->colorSpace8u->populateChoices(colorSpaces);
    _imp->colorSpace8u->setDefaultValue(0);
    page->addKnob(_imp->colorSpace8u);
    
    _imp->colorSpace16u = Natron::createKnob<Choice_Knob>(this, "Colorspace for 16-Bit Integer Images");
    _imp->colorSpace16u->setName("defaultColorSpace16u");
    _imp->colorSpace16u->setHintToolTip("Defines the color-space in which 16-bit integer images are assumed to be by default.");
    _imp->colorSpace16u->setAnimationEnabled(false);
    _imp->colorSpace16u->populateChoices(colorSpaces);
    _imp->colorSpace16u->setDefaultValue(2);
    page->addKnob(_imp->colorSpace16u);
    
    _imp->colorSpace32f = Natron::createKnob<Choice_Knob>(this, "Colorspace for 32-Bit Floating Point Images");
    _imp->colorSpace32f->setName("defaultColorSpace32f");
    _imp->colorSpace32f->setHintToolTip("Defines the color-space in which 32-bit floating point images are assumed to be by default.");
    _imp->colorSpace32f->setAnimationEnabled(false);
    _imp->colorSpace32f->populateChoices(colorSpaces);
    _imp->colorSpace32f->setDefaultValue(1);
    page->addKnob(_imp->colorSpace32f);
    
    _imp->frameRange = Natron::createKnob<Int_Knob>(this, "Frame Range",2);
    _imp->frameRange->setDefaultValue(1,0);
    _imp->frameRange->setDefaultValue(1,1);
    _imp->frameRange->setDimensionName(0, "first");
    _imp->frameRange->setDimensionName(1, "last");
    _imp->frameRange->setEvaluateOnChange(false);
    _imp->frameRange->setName("frameRange");
    _imp->frameRange->setHintToolTip("The frame range of the project as seen by the plug-ins. New viewers are created automatically "
                                     "with this frame-range. By default when a new Reader node is created, its frame range "
                                     "is unioned to this "
                                     "frame-range, unless the Lock frame range parameter is checked.");
    _imp->frameRange->setAnimationEnabled(false);
    _imp->frameRange->setAddNewLine(false);
    page->addKnob(_imp->frameRange);
    
    _imp->lockFrameRange = Natron::createKnob<Bool_Knob>(this, "Lock Range");
    _imp->lockFrameRange->setName("lockRange");
    _imp->lockFrameRange->setDefaultValue(false);
    _imp->lockFrameRange->setAnimationEnabled(false);
    _imp->lockFrameRange->setHintToolTip("By default when a new Reader node is created, its frame range is unioned to the "
                                         "project frame-range, unless this parameter is checked.");
    _imp->lockFrameRange->setEvaluateOnChange(false);
    page->addKnob(_imp->lockFrameRange);
    
    _imp->frameRate = Natron::createKnob<Double_Knob>(this, "Frame Rate");
    _imp->frameRate->setName("frameRate");
    _imp->frameRate->setHintToolTip("The frame rate of the project. This will serve as a default value for all effects that don't produce "
                                    "special frame rates.");
    _imp->frameRate->setAnimationEnabled(false);
    _imp->frameRate->setDefaultValue(24);
    _imp->frameRate->setDisplayMinimum(0.);
    _imp->frameRate->setDisplayMaximum(50.);
    page->addKnob(_imp->frameRate);
    
    boost::shared_ptr<Page_Knob> infoPage = Natron::createKnob<Page_Knob>(this, tr("Info").toStdString());
    
    _imp->projectName = Natron::createKnob<String_Knob>(this, "Project Name");
    _imp->projectName->setName("projectName");
    _imp->projectName->setIsPersistant(false);
    _imp->projectName->setAsLabel();
    _imp->projectName->setAnimationEnabled(false);
    _imp->projectName->setDefaultValue(NATRON_PROJECT_UNTITLED);
    infoPage->addKnob(_imp->projectName);
    
    _imp->projectPath = Natron::createKnob<String_Knob>(this, "Project path");
    _imp->projectPath->setName("projectPath");
    _imp->projectPath->setIsPersistant(false);
    _imp->projectPath->setAnimationEnabled(false);
    _imp->projectPath->setAsLabel();
    infoPage->addKnob(_imp->projectPath);
    
    _imp->natronVersion = Natron::createKnob<String_Knob>(this, "Saved With");
    _imp->natronVersion->setName("softwareVersion");
    _imp->natronVersion->setHintToolTip("The version of " NATRON_APPLICATION_NAME " that saved this project for the last time.");
    _imp->natronVersion->setAsLabel();
    _imp->natronVersion->setEvaluateOnChange(false);
    _imp->natronVersion->setAnimationEnabled(false);
    
    _imp->natronVersion->setDefaultValue(generateUserFriendlyNatronVersionName());
    infoPage->addKnob(_imp->natronVersion);
    
    _imp->originalAuthorName = Natron::createKnob<String_Knob>(this, "Original Author");
    _imp->originalAuthorName->setName("originalAuthor");
    _imp->originalAuthorName->setHintToolTip("The user name and host name of the original author of the project.");
    _imp->originalAuthorName->setAsLabel();
    _imp->originalAuthorName->setEvaluateOnChange(false);
    _imp->originalAuthorName->setAnimationEnabled(false);
    std::string authorName = generateGUIUserName();
    _imp->originalAuthorName->setDefaultValue(authorName);
    infoPage->addKnob(_imp->originalAuthorName);
    
    _imp->lastAuthorName = Natron::createKnob<String_Knob>(this, "Last Author");
    _imp->lastAuthorName->setName("lastAuthor");
    _imp->lastAuthorName->setHintToolTip("The user name and host name of the last author of the project.");
    _imp->lastAuthorName->setAsLabel();
    _imp->lastAuthorName->setEvaluateOnChange(false);
    _imp->lastAuthorName->setAnimationEnabled(false);
    _imp->lastAuthorName->setDefaultValue(authorName);
    infoPage->addKnob(_imp->lastAuthorName);


    _imp->projectCreationDate = Natron::createKnob<String_Knob>(this, "Created On");
    _imp->projectCreationDate->setName("creationDate");
    _imp->projectCreationDate->setHintToolTip("The creation date of the project.");
    _imp->projectCreationDate->setAsLabel();
    _imp->projectCreationDate->setEvaluateOnChange(false);
    _imp->projectCreationDate->setAnimationEnabled(false);
    _imp->projectCreationDate->setDefaultValue(QDateTime::currentDateTime().toString().toStdString());
    infoPage->addKnob(_imp->projectCreationDate);
    
    _imp->saveDate = Natron::createKnob<String_Knob>(this, "Last Saved On");
    _imp->saveDate->setName("lastSaveDate");
    _imp->saveDate->setHintToolTip("The date this project was last saved.");
    _imp->saveDate->setAsLabel();
    _imp->saveDate->setEvaluateOnChange(false);
    _imp->saveDate->setAnimationEnabled(false);
    infoPage->addKnob(_imp->saveDate);
    
    boost::shared_ptr<String_Knob> comments = Natron::createKnob<String_Knob>(this, "Comments");
    comments->setName("comments");
    comments->setHintToolTip("This area is a good place to write some informations about the project such as its authors, license "
                             "and anything worth mentionning about it.");
    comments->setAsMultiLine();
    comments->setAnimationEnabled(false);
    infoPage->addKnob(comments);
    
    boost::shared_ptr<Page_Knob> pythonPage = Natron::createKnob<Page_Knob>(this, "Python");
    _imp->onProjectLoadCB = Natron::createKnob<String_Knob>(this, "After Project Loaded");
    _imp->onProjectLoadCB->setName("afterProjectLoad");
    _imp->onProjectLoadCB->setHintToolTip("Add here the name of a Python-defined function that will be called each time this project "
                                          "is loaded either from an auto-save or by a user action. It will be called immediately after all "
                                          "nodes are re-created. This callback will not be called when creating new projects.\n "
                                          "The signature of the callback is: callback(app) where:\n"
                                          "- app: points to the current application instance\n");
    _imp->onProjectLoadCB->setAnimationEnabled(false);
    std::string onProjectLoad = appPTR->getCurrentSettings()->getDefaultOnProjectLoadedCB();
    _imp->onProjectLoadCB->setValue(onProjectLoad, 0);
    pythonPage->addKnob(_imp->onProjectLoadCB);
    
    
    _imp->onProjectSaveCB = Natron::createKnob<String_Knob>(this, "Before Project Save");
    _imp->onProjectSaveCB->setName("beforeProjectSave");
    _imp->onProjectSaveCB->setHintToolTip("Add here the name of a Python-defined function that will be called each time this project "
                                          "is saved by the user. This will be called prior to actually saving the project and can be used "
                                          "to change the filename of the file.\n"
                                          "The signature of the callback is: callback(filename,app,autoSave) where:\n"
                                          "- filename: points to the filename under which the project will be saved"
                                          "- app: points to the current application instance\n"
                                          "- autoSave: True if the save was called due to an auto-save, False otherwise\n"
                                          "You should return the new filename under which the project should be saved.");
    _imp->onProjectSaveCB->setAnimationEnabled(false);
    std::string onProjectSave = appPTR->getCurrentSettings()->getDefaultOnProjectSaveCB();
    _imp->onProjectSaveCB->setValue(onProjectSave, 0);
    pythonPage->addKnob(_imp->onProjectSaveCB);
    
    _imp->onProjectCloseCB = Natron::createKnob<String_Knob>(this, "Before Project Close");
    _imp->onProjectCloseCB->setName("beforeProjectClose");
    _imp->onProjectCloseCB->setHintToolTip("Add here the name of a Python-defined function that will be called each time this project "
                                          "is closed or if the user closes the application while this project is opened. This is called "
                                           "prior to removing anything from the project.\n"
                                           "The signature of the callback is: callback(app) where:\n"
                                           "- app: points to the current application instance\n");
    _imp->onProjectCloseCB->setAnimationEnabled(false);
    std::string onProjectClose = appPTR->getCurrentSettings()->getDefaultOnProjectCloseCB();
    _imp->onProjectCloseCB->setValue(onProjectClose, 0);
    pythonPage->addKnob(_imp->onProjectCloseCB);
    
    _imp->onNodeCreated = Natron::createKnob<String_Knob>(this, "After Node Created");
    _imp->onNodeCreated->setName("afterNodeCreated");
    _imp->onNodeCreated->setHintToolTip("Add here the name of a Python-defined function that will be called each time a node "
                                           "is created. The boolean variable userEdited will be set to True if the node was created "
                                        "by the user or False otherwise (such as when loading a project, or pasting a node).\n"
                                        "The signature of the callback is: callback(thisNode, app, userEdited) where:\n"
                                        "- thisNode: the node which has just been created\n"
                                        "- userEdited: a boolean indicating whether the node was created by user interaction or from "
                                        "a script/project load/copy-paste\n"
                                        "- app: points to the current application instance\n");
    _imp->onNodeCreated->setAnimationEnabled(false);
    std::string onNodeCreated = appPTR->getCurrentSettings()->getDefaultOnNodeCreatedCB();
    _imp->onNodeCreated->setValue(onNodeCreated, 0);
    pythonPage->addKnob(_imp->onNodeCreated);
    
    _imp->onNodeDeleted = Natron::createKnob<String_Knob>(this, "Before Node Removal");
    _imp->onNodeDeleted->setName("beforeNodeRemoval");
    _imp->onNodeDeleted->setHintToolTip("Add here the name of a Python-defined function that will be called each time a node "
                                        "is about to be deleted. This function will not be called when the project is closing.\n"
                                        "The signature of the callback is: callback(thisNode, app) where:\n"
                                        "- thisNode: the node about to be deleted\n"
                                        "- app: points to the current application instance\n");
    _imp->onNodeDeleted->setAnimationEnabled(false);
    std::string onNodeDelete = appPTR->getCurrentSettings()->getDefaultOnNodeDeleteCB();
    _imp->onNodeDeleted->setValue(onNodeDelete, 0);
    pythonPage->addKnob(_imp->onNodeDeleted);
    

    
    comments->setAsMultiLine();
    comments->setAnimationEnabled(false);
    infoPage->addKnob(comments);
    
    Q_EMIT knobsInitialized();
} // initializeKnobs

void
Project::evaluate(KnobI* /*knob*/,
                  bool /*isSignificant*/,
                  Natron::ValueChangedReasonEnum /*reason*/)
{
    assert(QThread::currentThread() == qApp->thread());

   /* if (isSignificant && knob != _imp->formatKnob.get()) {
        getCurrentNodes();
    
        NodeList nodes = getNodes();
        
        for (NodeList::iterator it = nodes.begin(); it != nodes.end() ; ++it) {
            assert(*it);
            (*it)->incrementKnobsAge();

            
            ViewerInstance* n = dynamic_cast<ViewerInstance*>( (*it)->getLiveInstance() );
            if (n) {
                n->renderCurrentFrame(true);
            }
        }
    }*/
}

// don't return a reference to a mutex-protected object!
void
Project::getProjectDefaultFormat(Format *f) const
{
    assert(f);
    QMutexLocker l(&_imp->formatMutex);
    int index = _imp->formatKnob->getValue();
    _imp->findFormat(index, f);
}


    
void
Project::ensureAllProcessingThreadsFinished()
{
    quitAnyProcessingForAllNodes();
    QThreadPool::globalInstance()->waitForDone();
}
int
Project::currentFrame() const
{
    return _imp->timeline->currentFrame();
}


int
Project::tryAddProjectFormat(const Format & f)
{
    //assert( !_imp->formatMutex.tryLock() );
    if ( ( f.left() >= f.right() ) || ( f.bottom() >= f.top() ) ) {
        return -1;
    }

    std::list<Format>::iterator foundFormat = std::find(_imp->builtinFormats.begin(), _imp->builtinFormats.end(), f);
    if ( foundFormat != _imp->builtinFormats.end() ) {
        return std::distance(_imp->builtinFormats.begin(),foundFormat);
    } else {
        foundFormat = std::find(_imp->additionalFormats.begin(), _imp->additionalFormats.end(), f);
        if ( foundFormat != _imp->additionalFormats.end() ) {
            return std::distance(_imp->additionalFormats.begin(),foundFormat) + _imp->builtinFormats.size();
        }
    }

    std::vector<std::string> entries;
    for (std::list<Format>::iterator it = _imp->builtinFormats.begin(); it != _imp->builtinFormats.end(); ++it) {
        const Format & f = *it;
        QString formatStr = generateStringFromFormat(f);
        entries.push_back( formatStr.toStdString() );
    }
    for (std::list<Format>::iterator it = _imp->additionalFormats.begin(); it != _imp->additionalFormats.end(); ++it) {
        const Format & f = *it;
        QString formatStr = generateStringFromFormat(f);
        entries.push_back( formatStr.toStdString() );
    }
    QString formatStr = generateStringFromFormat(f);
    entries.push_back( formatStr.toStdString() );
    _imp->additionalFormats.push_back(f);
    _imp->formatKnob->populateChoices(entries);

    return ( _imp->builtinFormats.size() + _imp->additionalFormats.size() ) - 1;
}

void
Project::setProjectDefaultFormat(const Format & f)
{
    //assert( !_imp->formatMutex.tryLock() );
    int index = tryAddProjectFormat(f);
    _imp->formatKnob->setValue(index,0);
    ///if locked it will trigger a deadlock because some parameters
    ///might respond to this signal by checking the content of the project format.
}

int
Project::getProjectViewsCount() const
{
    return _imp->viewsCount->getValue();
}

int
Project::getProjectMainView() const
{
    return _imp->mainView->getValue();
}

QString
Project::getProjectName() const
{
    return _imp->getProjectFilename().c_str();
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
    return _imp->getProjectPath().c_str();
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
    _imp->previewMode->setValue(!_imp->previewMode->getValue(),0);
}

boost::shared_ptr<TimeLine> Project::getTimeLine() const
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
Project::load(const ProjectSerialization & obj,const QString& name,const QString& path, bool* mustSave)
{
    return _imp->restoreFromSerialization(obj,name,path, mustSave);
}

void
Project::beginKnobsValuesChanged(Natron::ValueChangedReasonEnum /*reason*/)
{
}

void
Project::endKnobsValuesChanged(Natron::ValueChangedReasonEnum /*reason*/)
{
}

///this function is only called on the main thread
void
Project::onKnobValueChanged(KnobI* knob,
                            Natron::ValueChangedReasonEnum reason,
                            SequenceTime /*time*/,
                            bool /*originatedFromMainThread*/)
{
    if ( knob == _imp->viewsCount.get() ) {
        int viewsCount = _imp->viewsCount->getValue();
        getApp()->setupViewersForViews(viewsCount);

        int mainView = _imp->mainView->getValue();
        if (mainView >= viewsCount) {
            ///reset view to 0
            _imp->mainView->setValue(0, 0);
        }
        _imp->mainView->setMaximum(viewsCount - 1);
    } else if ( knob == _imp->formatKnob.get() ) {
        int index = _imp->formatKnob->getValue();
        Format frmt;
        bool found = _imp->findFormat(index, &frmt);
        if (found) {
            if (reason == Natron::eValueChangedReasonUserEdited) {
                ///Increase all nodes age in the project so all cache is invalidated: some effects images might rely on the project format
                NodeList nodes;
                getNodes_recursive(nodes);
                for (NodeList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
                    (*it)->incrementKnobsAge();
                }
                
                ///Format change, hence probably the PAR so run getClipPreferences again
                forceGetClipPreferencesOnAllTrees();
            }
            Q_EMIT formatChanged(frmt);
        }
    } else if ( knob == _imp->addFormatKnob.get() ) {
        Q_EMIT mustCreateFormat();
    } else if ( knob == _imp->previewMode.get() ) {
        Q_EMIT autoPreviewChanged( _imp->previewMode->getValue() );
    }  else if ( knob == _imp->frameRate.get() ) {
        forceGetClipPreferencesOnAllTrees();
    } else if (knob == _imp->frameRange.get()) {
        int first = _imp->frameRange->getValue(0);
        int last = _imp->frameRange->getValue(1);
        Q_EMIT frameRangeChanged(first, last);
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
    return (!hasEverAutoSaved() && !hasProjectBeenSavedByUser()) || !hasNodes();
}
    
QString
Project::getLockAbsoluteFilePath() const
{
    QString projectPath(_imp->getProjectPath().c_str());
    QString projectName(_imp->getProjectFilename().c_str());
  
    if (projectPath.isEmpty()) {
        return QString();
    }
    if (!projectPath.endsWith('/')) {
        projectPath.append('/');
    }
    QString lockFilePath = projectPath + projectName + ".lock";
    return lockFilePath;
}
    
void
Project::createLockFile()
{
    QString lastAuthor(_imp->lastAuthorName->getValue().c_str());
    QDateTime now = QDateTime::currentDateTime();
    QString lockFilePath = getLockAbsoluteFilePath();
    if (lockFilePath.isEmpty()) {
        return;
    }
    QFile f(lockFilePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return;
    }
    QTextStream ts(&f);
    QString curDateStr = now.toString();
    ts << curDateStr << '\n'
    << lastAuthor << '\n'
    << QCoreApplication::applicationPid();
}
    
void
Project::removeLockFile()
{
    QString lockFilePath = getLockAbsoluteFilePath();
    if (QFile::exists(lockFilePath)) {
        QFile::remove(lockFilePath);
    }
}
    
bool
Project::getLockFileInfos(const QString& projectPath,const QString& projectName,QString* authorName,QString* lastSaveDate,qint64* appPID) const
{
    QString realPath = projectPath;
    if (!realPath.endsWith('/')) {
        realPath.append('/');
    }
    QString lockFilePath = realPath + projectName + ".lock";
    QFile f(lockFilePath);
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    QTextStream ts(&f);
    if (!ts.atEnd()) {
        *lastSaveDate = ts.readLine();
    } else {
        return false;
    }
    if (!ts.atEnd()) {
        *authorName = ts.readLine();
    } else {
        return false;
    }
    if (!ts.atEnd()) {
        QString pidstr = ts.readLine();
        *appPID = pidstr.toLongLong();
    } else {
        return false;
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
    
    if (!filepath.isEmpty()) {
        QFile::remove(filepath);
    }
    
    /*
     * Since we may have saved the project to an old project, overwritting the existing file, there might be 
     * a oldProject.ntp.autosave file next to it that belonged to the old project, make sure it gets removed too
     */
    QString projectPath(_imp->getProjectPath().c_str());
    QString projectName(_imp->getProjectFilename().c_str());
    
    if (!projectPath.endsWith('/')) {
        projectPath.append('/');
    }
    
    QString autoSaveFilePath = projectPath + projectName + ".autosave";
    if (QFile::exists(autoSaveFilePath)) {
        QFile::remove(autoSaveFilePath);
    }
}

void
Project::clearAutoSavesDir()
{
    /*removing all autosave files*/
    QDir savesDir( autoSavesDir() );
    QStringList entries = savesDir.entryList();

    for (int i = 0; i < entries.size(); ++i) {
        const QString & entry = entries.at(i);
        
        ///Do not remove the RENDER_SAVE used by the background processes to render because otherwise they may fail to start rendering.
        /// @see AppInstance::startWritersRendering
        if (entry.contains("RENDER_SAVE")) {
            continue;
        }
        
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QFile::remove(savesDir.path() + QDir::separator() + entry);
        }
    }
}

QString
Project::autoSavesDir()
{
    return Natron::StandardPaths::writableLocation(Natron::StandardPaths::eStandardLocationData) + QDir::separator() + "Autosaves";
}
    
void
Project::resetProject()
{
    reset(false);
    if (!appPTR->isBackground()) {
        createViewer();
    }
}

void
Project::reset(bool aboutToQuit)
{
    assert(QThread::currentThread() == qApp->thread());
    
    _imp->projectClosing = true;

    _imp->runOnProjectCloseCallback();
    
    QString lockFilePath = getLockAbsoluteFilePath();
    QString projectPath(_imp->getProjectPath().c_str());
    QString projectName(_imp->getProjectFilename().c_str());
    ///Remove the lock file if we own it
    if (QFile::exists(lockFilePath)) {
        QString author,lastsave;
        qint64 pid;
        if (getLockFileInfos(projectPath, projectName, &author, &lastsave, &pid)) {
            if (pid == QCoreApplication::applicationPid()) {
                QFile::remove(lockFilePath);
            }
        }
    }
    clearNodes(true);

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
    _imp->timeline->removeAllKeyframesIndicators();
    
    Q_EMIT projectNameChanged(NATRON_PROJECT_UNTITLED);
    
    if (!aboutToQuit) {
        const std::vector<boost::shared_ptr<KnobI> > & knobs = getKnobs();
        
        beginChanges();
        for (U32 i = 0; i < knobs.size(); ++i) {
            for (int j = 0; j < knobs[i]->getDimension(); ++j) {
                knobs[i]->resetToDefaultValue(j);
            }
        }
        
        
        onOCIOConfigPathChanged(appPTR->getOCIOConfigPath(),true);
        
        endChanges(true);
    }
    
    _imp->projectClosing = false;
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

    Format dispW;
    {
        QMutexLocker l(&_imp->formatMutex);

        if (_imp->autoSetProjectFormat) {
            _imp->autoSetProjectFormat = false;
            dispW = frmt;

            Format* df = appPTR->findExistingFormat(dispW.width(), dispW.height(), dispW.getPixelAspectRatio());
            if (df) {
                dispW.setName(df->getName());
                setProjectDefaultFormat(dispW);
            } else {
                setProjectDefaultFormat(dispW);
            }
        } else if (!skipAdd) {
            dispW = frmt;
            tryAddProjectFormat(dispW);
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

Natron::ViewerColorSpaceEnum
Project::getDefaultColorSpaceForBitDepth(Natron::ImageBitDepthEnum bitdepth) const
{
    switch (bitdepth) {
    case Natron::eImageBitDepthByte:

        return (Natron::ViewerColorSpaceEnum)_imp->colorSpace8u->getValue();
    case Natron::eImageBitDepthShort:

        return (Natron::ViewerColorSpaceEnum)_imp->colorSpace16u->getValue();
    case Natron::eImageBitDepthFloat:

        return (Natron::ViewerColorSpaceEnum)_imp->colorSpace32f->getValue();
    case Natron::eImageBitDepthNone:
        assert(false);
        break;
    }

    return eViewerColorSpaceLinear;
}

// Functions to escape / unescape characters from XML strings
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
                // Escape even the whitespace characters '\n' '\r' '\t', although they are valid
                // XML, because they would be converted to space when re-read.
                // See http://www.w3.org/TR/xml/#AVNormalize
                unsigned char c = (unsigned char)(str[i]);
                // Escape even the whitespace characters '\n' '\r' '\t', although they are valid
                // XML, because they would be converted to space when re-read.
                // See http://www.w3.org/TR/xml/#AVNormalize
                if ((0x01 <= c && c <= 0x1f) || (0x7F <= c && c <= 0x9F)) {
                    // these characters must be escaped in XML 1.1
                    // http://www.w3.org/TR/xml/#sec-references
                    std::string ns = "&#x";
                    if (c > 0xf) {
                        int d = c / 0x10;
                        ns += d < 10 ? ('0' + d) : ('A' + d - 10);
                    }
                    int d = c & 0xf;
                    ns += d < 10 ? ('0' + d) : ('A' + d - 10);
                    ns += ';';
                    str.replace(i, 1, ns);
                    i += ns.size() + 1;
                }
            }   break;
        }
    }
    return str;
}

std::string
Project::unescapeXML(const std::string &istr)
{
    size_t i;
    std::string str = istr;
    i = str.find_first_of("&");
    while (i != std::string::npos) {
        assert(str[i] == '&');
        if (!str.compare(i + 1, 3, "lt;")) {
            str.replace(i, 4, 1, '<');
        } else if (!str.compare(i + 1, 3, "gt;")) {
            str.replace(i, 4, 1, '>');
        } else if (!str.compare(i + 1, 4, "amp;")) {
            str.replace(i, 5, 1, '&');
        } else if (!str.compare(i + 1, 5, "apos;")) {
            str.replace(i, 6, 1, '\'');
        } else if (!str.compare(i + 1, 5, "quot;")) {
            str.replace(i, 6, 1, '"');
        } else if (!str.compare(i + 1, 1, "#")) {
            size_t end = str.find_first_of(";", i + 2);
            if (end == std::string::npos) {
                // malformed XML
                return str;
            }
            char *tail = NULL;
            int errno_save = errno;
            bool hex = str[i+2] == 'x' || str[i+2] == 'X';
            char *head = &str[i+ (hex ? 3 : 2)];

            errno = 0;
            unsigned long cp = std::strtoul(head, &tail, hex ? 16 : 10);

            bool fail = errno || (tail - &str[0])!= (long)end || cp > 0xff; // only handle 0x01-0xff
            errno = errno_save;
            if (fail) {
                return str;
            }
            // replace from '&' to ';' (thus the +1)
            str.replace(i, tail - head + 1, 1, (char)cp);
        }
        i = str.find_first_of("&", i + 1);
    }
    return str;
}

void
Project::makeEnvMap(const std::string& encoded,std::map<std::string,std::string>& variables)
{
    std::string startNameTag(NATRON_ENV_VAR_NAME_START_TAG);
    std::string endNameTag(NATRON_ENV_VAR_NAME_END_TAG);
    std::string startValueTag(NATRON_ENV_VAR_VALUE_START_TAG);
    std::string endValueTag(NATRON_ENV_VAR_VALUE_END_TAG);
    
    size_t i = encoded.find(startNameTag);
    while (i != std::string::npos) {
        i += startNameTag.size();
        assert(i < encoded.size());
        size_t endNamePos = encoded.find(endNameTag,i);
        assert(endNamePos != std::string::npos && endNamePos < encoded.size());
        
        std::string name,value;
        while (i < endNamePos) {
            name.push_back(encoded[i]);
            ++i;
        }
        
        i = encoded.find(startValueTag,i);
        i += startValueTag.size();
        assert(i != std::string::npos && i < encoded.size());
        
        size_t endValuePos = encoded.find(endValueTag,i);
        assert(endValuePos != std::string::npos && endValuePos < encoded.size());
        
        while (i < endValuePos) {
            value.push_back(encoded.at(i));
            ++i;
        }
        
        // In order to use XML tags, the text inside the tags has to be unescaped.
        variables.insert(std::make_pair(unescapeXML(name), unescapeXML(value)));
        
        i = encoded.find(startNameTag,i);
    }
}

void
Project::getEnvironmentVariables(std::map<std::string,std::string>& env) const
{
    std::string raw = _imp->envVars->getValue();
    makeEnvMap(raw, env);
}
    
void
Project::expandVariable(const std::map<std::string,std::string>& env,std::string& str)
{
    ///Loop while we can still expand variables, up to NATRON_PROJECT_ENV_VAR_MAX_RECURSION recursions
    for (int i = 0; i < NATRON_PROJECT_ENV_VAR_MAX_RECURSION; ++i) {
        for (std::map<std::string,std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            if (expandVariable(it->first,it->second, str)) {
                break;
            }
        }
    }
}
    
bool
Project::expandVariable(const std::string& varName,const std::string& varValue,std::string& str)
{
    if (str.size() > (varName.size() + 2) && ///can contain the environment variable name
        str[0] == '[' && /// env var name is bracketed
        str.substr(1,varName.size()) == varName && /// starts with the environment variable name
        str[varName.size() + 1] == ']') { /// env var name is bracketed
        
        str.erase(str.begin() + varName.size() + 1);
        str.erase(str.begin());
        str.replace(0,varName.size(),varValue);
        return true;
    }
    return false;

}
   
void
Project::findReplaceVariable(const std::map<std::string,std::string>& env,std::string& str)
{
    std::string longestName;
    std::string longestVar;
    for (std::map<std::string,std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
        if (str.size() >= it->second.size() &&
            it->second.size() > longestVar.size() &&
            str.substr(0,it->second.size()) == it->second) {
            longestName = it->first;
            longestVar = it->second;
        }
    }
    if (!longestName.empty() && !longestVar.empty()) {
        std::string replaceStr;
        replaceStr += '[';
        replaceStr += longestName;
        replaceStr += ']';
        str.replace(0, longestVar.size(),replaceStr);
    }
    
}
    
void
Project::makeRelativeToVariable(const std::string& varName,const std::string& varValue,std::string& str)
{
    
    bool hasTrailingSep = !varValue.empty() && (varValue[varValue.size() - 1] == '/' || varValue[varValue.size() - 1] == '\\');
    if (str.size() > varValue.size() && str.substr(0,varValue.size()) == varValue) {
        if (hasTrailingSep) {
            str = '[' + varName + ']' + str.substr(varValue.size(),str.size());
        } else {
            str = '[' + varName + "]/" + str.substr(varValue.size() + 1,str.size());
        }
    } else {
        QDir dir(varValue.c_str());
        QString relative = dir.relativeFilePath(str.c_str());
        if (hasTrailingSep) {
            str = '[' + varName + ']' + relative.toStdString();
        } else {
            str = '[' + varName + "]/" + relative.toStdString();
        }
    }

    
    
}
    
bool
Project::fixFilePath(const std::string& projectPathName,const std::string& newProjectPath,
                    std::string& filePath)
{
    if (filePath.size() < (projectPathName.size() + 2) //< filepath doesn't have enough space to  contain the variable
        || filePath[0] != '['
        || filePath[projectPathName.size() + 1] != ']'
        || filePath.substr(1,projectPathName.size()) != projectPathName) {
        return false;
    }
    
    canonicalizePath(filePath);
    
    if (newProjectPath.empty()) {
        return true; //keep it absolute if the variables points to nothing
    } else {
        QDir dir(newProjectPath.c_str());
        if (!dir.exists()) {
            return false;
        }
        
        filePath = dir.relativeFilePath(filePath.c_str()).toStdString();
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
    return (str.empty() || (!str.empty() && (str[0] != '/')
                            && (!(str.size() >= 2 && str[1] == ':'))));
#else  //Unix
    return (str.empty() || (str[0] != '/'));
#endif
}
    
    
    
void
Project::canonicalizePath(std::string& str)
{
    std::map<std::string,std::string> envvar;
    getEnvironmentVariables(envvar);
    
    expandVariable(envvar, str);
    
    if ( !str.empty() ) {
        
        ///Now check if the string is relative
        
        if (isRelative(str)) {
            ///If it doesn't start with an env var but is relative, prepend the project env var
            std::map<std::string,std::string>::iterator foundProject = envvar.find(NATRON_PROJECT_ENV_VAR_NAME);
            if (foundProject != envvar.end()) {
                if (foundProject->second.empty()) {
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
        QFileInfo info(str.c_str());
        QString canonical =  info.canonicalFilePath();
        if ( canonical.isEmpty() && !str.empty() ) {
            return;
        } else {
            str = canonical.toStdString();
        }
        
    }

}
    
    
void
Project::simplifyPath(std::string& str)
{
    std::map<std::string,std::string> envvar;
    getEnvironmentVariables(envvar);
    
    Natron::Project::findReplaceVariable(envvar,str);
}
    
    
void
Project::makeRelativeToProject(std::string& str)
{
    std::map<std::string,std::string> envvar;
    getEnvironmentVariables(envvar);

    std::map<std::string,std::string>::iterator found = envvar.find(NATRON_PROJECT_ENV_VAR_NAME);
    if (found != envvar.end() && !found->second.empty()) {
        makeRelativeToVariable(found->first, found->second, str);
    }
}

    
void
Project::onOCIOConfigPathChanged(const std::string& path,bool block)
{
    beginChanges();
    
    try {
        std::string env = _imp->envVars->getValue();
        std::map<std::string, std::string> envMap;
        makeEnvMap(env, envMap);

        ///If there was already a OCIO variable, update it, otherwise create it

        std::map<std::string, std::string>::iterator foundOCIO = envMap.find(NATRON_OCIO_ENV_VAR_NAME);
        if (foundOCIO != envMap.end()) {
            foundOCIO->second = path;
        } else {
            envMap.insert(std::make_pair(NATRON_OCIO_ENV_VAR_NAME, path));
        }

        std::string newEnv;
        for (std::map<std::string, std::string>::iterator it = envMap.begin(); it != envMap.end(); ++it) {
            // In order to use XML tags, the text inside the tags has to be escaped.
            newEnv += NATRON_ENV_VAR_NAME_START_TAG;
            newEnv += Project::escapeXML(it->first);
            newEnv += NATRON_ENV_VAR_NAME_END_TAG;
            newEnv += NATRON_ENV_VAR_VALUE_START_TAG;
            newEnv += Project::escapeXML(it->second);
            newEnv += NATRON_ENV_VAR_VALUE_END_TAG;
        }
        if (env != newEnv) {
            if (appPTR->getCurrentSettings()->isAutoFixRelativeFilePathEnabled()) {
                fixRelativeFilePaths(NATRON_OCIO_ENV_VAR_NAME, path,block);
            }
            _imp->envVars->setValue(newEnv, 0);
        }
        endChanges(block);
        
    } catch (std::logic_error) {
        // ignore
    }
}

double
Project::getProjectFrameRate() const
{
    return _imp->frameRate->getValue();
}
    
boost::shared_ptr<Path_Knob>
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
    assert(QThread::currentThread() == qApp->thread());
    return _imp->projectClosing;
}
    
bool
Project::isFrameRangeLocked() const
{
    return _imp->lockFrameRange->getValue();
}
    
void
Project::getFrameRange(double* first,double* last) const
{
    *first = _imp->frameRange->getValue(0);
    *last = _imp->frameRange->getValue(1);
}
    
void
Project::unionFrameRangeWith(int first,int last)
{
    
    int curFirst,curLast;
    bool mustSet = !_imp->frameRange->hasModifications();
    curFirst = _imp->frameRange->getValue(0);
    curLast = _imp->frameRange->getValue(1);
    curFirst = !mustSet ? std::min(first, curFirst) : first;
    curLast = !mustSet ? std::max(last, curLast) : last;
    beginChanges();
    _imp->frameRange->setValue(curFirst, 0);
    _imp->frameRange->setValue(curLast, 1);
    endChanges();

}
    
void
Project::recomputeFrameRangeFromReaders()
{
    int first = 1,last = 1;
    recomputeFrameRangeForAllReaders(&first, &last);
    
    beginChanges();
    _imp->frameRange->setValue(first, 0);
    _imp->frameRange->setValue(last, 1);
    endChanges();
}
    
void
Project::createViewer()
{
    if (appPTR->isBackground()) {
        return;
    }
    getApp()->createNode( CreateNodeArgs(PLUGINID_NATRON_VIEWER,
                                         "",
                                         -1,-1,
                                         true,
                                         INT_MIN,INT_MIN,
                                         false,
                                         true,
                                         false,
                                         QString(),
                                         CreateNodeArgs::DefaultValuesList(),
                                         shared_from_this()) );
}
    
static bool hasNodeOutputsInList(const std::list<boost::shared_ptr<Natron::Node> >& nodes,const boost::shared_ptr<Natron::Node>& node)
{
    const std::list<Natron::Node*>& outputs = node->getGuiOutputs();
    
    bool foundOutput = false;
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            std::list<Natron::Node*>::const_iterator found = std::find(outputs.begin(),outputs.end(),it->get());
            if (found != outputs.end()) {
                foundOutput = true;
                break;
            }
        }
    }
    return foundOutput;
}
    
static bool hasNodeInputsInList(const std::list<boost::shared_ptr<Natron::Node> >& nodes,const boost::shared_ptr<Natron::Node>& node)
{
    const std::vector<boost::shared_ptr<Natron::Node> >& inputs = node->getGuiInputs();
    
    bool foundInput = false;
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        if (*it != node) {
            std::vector<boost::shared_ptr<Natron::Node> >::const_iterator found = std::find(inputs.begin(),inputs.end(),*it);
            if (found != inputs.end()) {
                foundInput = true;
                break;
            }
        }
    }
    return foundInput;
}
    
static void addTreeInputs(const std::list<boost::shared_ptr<Natron::Node> >& nodes,const boost::shared_ptr<Natron::Node>& node,
                              Project::NodesTree& tree,
                              std::list<boost::shared_ptr<Natron::Node> >& markedNodes)
{
    if (std::find(markedNodes.begin(), markedNodes.end(), node) != markedNodes.end()) {
        return;
    }
    
    if (std::find(nodes.begin(), nodes.end(), node) == nodes.end()) {
        return;
    }
    
    if (!hasNodeInputsInList(nodes,node)) {
        Project::TreeInput input;
        input.node = node;
        input.inputs = node->getGuiInputs();
        tree.inputs.push_back(input);
        markedNodes.push_back(node);
    } else {
        tree.inbetweenNodes.push_back(node);
        markedNodes.push_back(node);
        const std::vector<boost::shared_ptr<Natron::Node> >& inputs = node->getGuiInputs();
        for (std::vector<boost::shared_ptr<Natron::Node> >::const_iterator it2 = inputs.begin() ; it2!=inputs.end(); ++it2) {
            if (*it2) {
                addTreeInputs(nodes, *it2, tree, markedNodes);
            }
        }
    }
}
    
void Project::extractTreesFromNodes(const std::list<boost::shared_ptr<Natron::Node> >& nodes,std::list<Project::NodesTree>& trees)
{
    std::list<boost::shared_ptr<Natron::Node> > markedNodes;
    
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isOutput = !hasNodeOutputsInList(nodes, *it);
        if (isOutput) {
            NodesTree tree;
            tree.output.node = *it;

            const std::list<Natron::Node* >& outputs = (*it)->getGuiOutputs();
            for (std::list<Natron::Node*>::const_iterator it2 = outputs.begin(); it2!=outputs.end(); ++it2) {
                int idx = (*it2)->inputIndex(it->get());
                tree.output.outputs.push_back(std::make_pair(idx,*it2));
            }
            
            const std::vector<boost::shared_ptr<Natron::Node> >& inputs = (*it)->getGuiInputs();
            for (U32 i = 0; i < inputs.size(); ++i) {
                if (inputs[i]) {
                    addTreeInputs(nodes, inputs[i], tree, markedNodes);
                }
            }
            
            if (tree.inputs.empty()) {
                TreeInput input;
                input.node = *it;
                input.inputs = (*it)->getGuiInputs();
                tree.inputs.push_back(input);
            }
            
            trees.push_back(tree);
        }
    }
    
}
  
bool Project::addFormat(const std::string& formatSpec)
{
    Format f;
    if (generateFormatFromString(formatSpec.c_str(), &f)) {
        QMutexLocker k(&_imp->formatMutex);
        tryAddProjectFormat(f);
        return true;
    } else {
        return false;
    }
}
    
} //namespace Natron


