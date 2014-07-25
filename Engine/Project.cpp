//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "Project.h"

#include <fstream>
#include <algorithm>

#include <QtConcurrentRun>
#include <QCoreApplication>
#include <QTimer>
#include <QThreadPool>
#include <QTemporaryFile>

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/ProjectPrivate.h"
#include "Engine/VideoEngine.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ProjectSerialization.h"
#include "Engine/Settings.h"
#include "Engine/KnobFile.h"
#include "Engine/StandardPaths.h"

using std::cout; using std::endl;
using std::make_pair;


namespace Natron{


Project::Project(AppInstance* appInstance)
    : KnobHolder(appInstance)
    , _imp(new ProjectPrivate(this))
{
    QObject::connect(_imp->autoSaveTimer.get(), SIGNAL(timeout()), this, SLOT(onAutoSaveTimerTriggered()));
}

Project::~Project() {
    clearNodes(false);
    
    ///Don't clear autosaves if the program is shutting down by user request.
    ///Even if the user replied she/he didn't want to save the current work, we keep an autosave of it.
    //removeAutoSaves();
}

bool Project::loadProject(const QString& path,const QString& name){
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        assert(!_imp->isLoadingProject);
        _imp->isLoadingProject = true;
    }

    reset();
    
    try {
        loadProjectInternal(path,name);
    } catch (const std::exception& e) {
        {
            QMutexLocker l(&_imp->isLoadingProjectMutex);
            _imp->isLoadingProject = false;
        }
        Natron::errorDialog(QObject::tr("Project loader").toStdString(), QObject::tr("Error while loading project").toStdString() + ": " + e.what());
        if(!appPTR->isBackground()) {
            getApp()->createNode(CreateNodeArgs("Viewer"));
        }
        return false;
    } catch (...) {
        {
            QMutexLocker l(&_imp->isLoadingProjectMutex);
            _imp->isLoadingProject = false;
        }
        Natron::errorDialog(QObject::tr("Project loader").toStdString(), QObject::tr("Unkown error while loading project").toStdString());
        if(!appPTR->isBackground()) {
            getApp()->createNode(CreateNodeArgs("Viewer"));
        }
        return false;
    }
    
    ///Process all events before flagging that we're no longer loading the project
    ///to avoid multiple renders being called because of reshape events of viewers
    QCoreApplication::processEvents();
    
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        _imp->isLoadingProject = false;
    }
    
    refreshViewersAndPreviews();
    
    ///We successfully loaded the project, remove auto-saves of previous projects.
    removeAutoSaves();
    
    return true;
}

void Project::loadProjectInternal(const QString& path,const QString& name) {



    QString filePath = path+name;
    if(!QFile::exists(filePath)){
        throw std::invalid_argument(QString(filePath + " : no such file.").toStdString());
    }
    std::ifstream ifile;
    try {
        ifile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ifile.open(filePath.toStdString().c_str(),std::ifstream::in);
    } catch (const std::ifstream::failure& e) {
        throw std::runtime_error(std::string("Exception occured when opening file ") + filePath.toStdString() + ": " + e.what());
    }
    try {
        boost::archive::xml_iarchive iArchive(ifile);
        bool bgProject;
        iArchive >> boost::serialization::make_nvp("Background_project",bgProject);
        ProjectSerialization projectSerializationObj(getApp());
        iArchive >> boost::serialization::make_nvp("Project",projectSerializationObj);
        load(projectSerializationObj);
        if (!bgProject) {
            getApp()->loadProjectGui(iArchive);
        }
    } catch(const boost::archive::archive_exception& e) {
        ifile.close();
        throw std::runtime_error(e.what());
    } catch(const std::exception& e) {
        ifile.close();
        throw std::runtime_error(std::string("Failed to read the project file: ") + std::string(e.what()));
    }
    ifile.close();

    QDateTime time = QDateTime::currentDateTime();
    _imp->autoSetProjectFormat = false;
    _imp->hasProjectBeenSavedByUser = true;
    _imp->projectName = name;
    _imp->projectPath = path;
    _imp->ageSinceLastSave = time;
    _imp->lastAutoSave = time;
    emit projectNameChanged(name);
}

void Project::refreshViewersAndPreviews() {
    /*Refresh all previews*/
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (_imp->currentNodes[i]->isPreviewEnabled()) {
            _imp->currentNodes[i]->computePreviewImage(_imp->timeline->currentFrame());
        }
    }

    /*Refresh all viewers as it was*/
    if (!appPTR->isBackground()) {
        const std::vector<boost::shared_ptr<Natron::Node> >& nodes = getCurrentNodes();
        for (U32 i = 0; i < nodes.size(); ++i) {
            assert(nodes[i]);
            if (nodes[i]->pluginID() == "Viewer") {
                ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
                assert(n);
                n->getVideoEngine()->render(1,//< frame count
                                            true, //<seek timeline
                                            true, //< refresh tree
                                            true, //< forward
                                            false, //< same frame
                                            true); //< force preview
            }
        }
    }
}

void Project::saveProject(const QString& path,const QString& name,bool autoS){
    {
        QMutexLocker l(&_imp->isLoadingProjectMutex);
        if(_imp->isLoadingProject){
            return;
        }
    }
    
    {
        QMutexLocker l(&_imp->isSavingProjectMutex);
        if (_imp->isSavingProject) {
            return;
        } else {
            _imp->isSavingProject = true;
        }
    }

    try {
        if (!autoS) {
            //if  (!isSaveUpToDate() || !QFile::exists(path+name)) {

                saveProjectInternal(path,name);
                ///also update the auto-save
                removeAutoSaves();

            //}
        } else {
            
            removeAutoSaves();
            saveProjectInternal(path,name,true);
            
        }
    } catch (const std::exception& e) {
        
        if(!autoS) {
            Natron::errorDialog(QObject::tr("Save").toStdString(), e.what());
        } else {
            qDebug() << "Save failure: " << e.what();
        }
    }
    {
        QMutexLocker l(&_imp->isSavingProjectMutex);
        _imp->isSavingProject = false;
    }
}
    
static bool fileCopy(const QString& source,const QString& dest) {
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

QDateTime Project::saveProjectInternal(const QString& path,const QString& name,bool autoSave) {

    QDateTime time = QDateTime::currentDateTime();
    QString timeStr = time.toString();
    Hash64 timeHash;
    for(int i = 0 ; i < timeStr.size();++i) {
        timeHash.append<unsigned short>(timeStr.at(i).unicode());
    }
    timeHash.computeHash();
    QString timeHashStr = QString::number(timeHash.value());

    QString actualFileName = name;
    if(autoSave){
        QString pathCpy = path;

#ifdef __NATRON_WIN32__
        ///on windows, we must also modifiy the root name otherwise it would fail to save with a filename containing for example C:/
        QFileInfoList roots = QDir::drives();
        QString root;
        for (int i = 0; i < roots.size(); ++i) {
            QString rootPath = roots[i].absolutePath();
            rootPath = rootPath.remove(QChar('\\'));
            rootPath = rootPath.remove(QChar('/'));
            if (pathCpy.startsWith(rootPath)) {
                root = rootPath;
                QString rootToPrepend("_ROOT_");
                rootToPrepend.append(root.at(0)); //< append the root character, e.g the 'C' of C:
                rootToPrepend.append("_N_ROOT_");
                pathCpy.replace(rootPath, rootToPrepend);
                break;
            }
        }

#endif
        pathCpy = pathCpy.replace("/", "_SEP_");
        pathCpy = pathCpy.replace("\\", "_SEP_");
        actualFileName.prepend(pathCpy);
        actualFileName.append("."+timeHashStr);
    }
    QString filePath;
    if (autoSave) {
        filePath = Project::autoSavesDir() + QDir::separator() + actualFileName;
        _imp->lastAutoSaveFilePath = filePath;
    } else {
        filePath = path+actualFileName;
    }
    
    ///Use a temporary file to save, so if Natron crashes it doesn't corrupt the user save.
    QString tmpFilename = StandardPaths::writableLocation(StandardPaths::TempLocation);
    tmpFilename.append(QDir::separator());
    tmpFilename.append(QString::number(time.toMSecsSinceEpoch()));
    
    std::ofstream ofile;
    try {
        ofile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        ofile.open(tmpFilename.toStdString().c_str(),std::ofstream::out);
    } catch (const std::ofstream::failure& e) {
        throw std::runtime_error(std::string("Exception occured when opening file ") + filePath.toStdString() + ": " + e.what());
    }
    if (!ofile.good()) {
        qDebug() << "Failed to open file " << filePath.toStdString().c_str();
        ofile.close();
        throw std::runtime_error("Failed to open file " + filePath.toStdString());
    }
    
    try {
        boost::archive::xml_oarchive oArchive(ofile);
        bool bgProject = appPTR->isBackground();
        oArchive << boost::serialization::make_nvp("Background_project",bgProject);
        ProjectSerialization projectSerializationObj(getApp());
        save(&projectSerializationObj);
        oArchive << boost::serialization::make_nvp("Project",projectSerializationObj);
        if(!bgProject){
            getApp()->saveProjectGui(oArchive);
        }
    } catch (...) {
        ofile.close();
        throw;
    }
    ofile.close();

    QFile::remove(filePath);
    int nAttemps = 0;
    
    while (nAttemps < 10 && !fileCopy(tmpFilename, filePath)) {
        ++nAttemps;
    }
    
    QFile::remove(tmpFilename);
    
    _imp->projectName = name;
    if (!autoSave) {
        emit projectNameChanged(name); //< notify the gui so it can update the title
    } else {
        emit projectNameChanged(name + " (*)");
    }
    _imp->projectPath = path;
    if(!autoSave){
        _imp->hasProjectBeenSavedByUser = true;
        _imp->ageSinceLastSave = time;
    }
    _imp->lastAutoSave = time;
    return time;
}

void Project::autoSave(){

    ///don't autosave in background mode...
    if (appPTR->isBackground()) {
        return;
    }

    saveProject(_imp->projectPath, _imp->projectName, true);
}

void Project::triggerAutoSave() {

    ///Should only be called in the main-thread, that is upon user interaction.
    assert(QThread::currentThread() == qApp->thread());
    
    if (appPTR->isBackground()) {
        return;
    }
    
    _imp->autoSaveTimer->start(appPTR->getCurrentSettings()->getAutoSaveDelayMS());
}
    
void Project::onAutoSaveTimerTriggered()
{
    assert(!appPTR->isBackground());
    
    ///check that all VideoEngine(s) are not working.
    ///If so launch an auto-save, otherwise, restart the timer.
    bool canAutoSave = true;
    {
        QMutexLocker l(&_imp->nodesLock);
        for (std::vector< boost::shared_ptr<Natron::Node> >::iterator it = _imp->currentNodes.begin(); it!=_imp->currentNodes.end(); ++it) {
            if ((*it)->isOutputNode()) {
                Natron::OutputEffectInstance* effect = dynamic_cast<Natron::OutputEffectInstance*>((*it)->getLiveInstance());
                assert(effect);
                if (effect->getVideoEngine()->isWorking()) {
                    canAutoSave = false;
                    break;
                }
            }
        }
    }
    
    if (canAutoSave) {
        QtConcurrent::run(this,&Project::autoSave);
    } else {
        ///If the auto-save failed because a render is in progress, try every 2 seconds to auto-save.
        ///We don't use the user-provided timeout interval here because it could be an inapropriate value.
        _imp->autoSaveTimer->start(2000);
    }
}

bool Project::findAndTryLoadAutoSave() {
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for (int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {

            QString filename = entry.left(suffixPos+searchStr.size()-1);
            bool exists = false;

            if(!filename.contains(NATRON_PROJECT_UNTITLED)){
#ifdef __NATRON_WIN32__
                ///on windows we must extract the root of the filename (@see saveProjectInternal)
                int rootPos = filename.indexOf("_ROOT_");
                int endRootPos =  filename.indexOf("_N_ROOT_");
                QString rootName;
                if (rootPos != -1) {
                    assert(endRootPos != -1);//< if we found _ROOT_ then _N_ROOT must exist too
                    int startRootNamePos = rootPos + 6;
                    rootName = filename.mid(startRootNamePos,endRootPos - startRootNamePos);
                }
                filename.replace("_ROOT" + rootName + "_N_ROOT_",rootName + ':');
#endif
                filename = filename.replace("_SEP_",QDir::separator());
                exists = QFile::exists(filename);
            }

            QString text;

            if (exists) {
                text = tr("A recent auto-save of %1 was found.\n"
                                  "Would you like to restore it entirely? "
                                  "Clicking No will remove this auto-save.").arg(filename);;
            } else {
                text = tr("An auto-save was restored successfully. It didn't belong to any project\n"
                          "Would you like to restore it ? Clicking No will remove this auto-save forever.");
            }

            appPTR->hideSplashScreen();

            Natron::StandardButton ret = Natron::questionDialog(tr("Auto-save").toStdString(),
                                                                text.toStdString(),Natron::StandardButtons(Natron::Yes | Natron::No),
                                                                Natron::Yes);
            if (ret == Natron::No || ret == Natron::Escape) {
                removeAutoSaves();
                reset();
                return false;
            } else {
                {
                    QMutexLocker l(&_imp->isLoadingProjectMutex);
                    assert(!_imp->isLoadingProject);
                    _imp->isLoadingProject = true;
                }
                try {
                    loadProjectInternal(savesDir.path()+QDir::separator(), entry);
                } catch (const std::exception& e) {
                    Natron::errorDialog(QObject::tr("Project loader").toStdString(), QObject::tr("Error while loading auto-saved project").toStdString() + ": " + e.what());
                    getApp()->createNode(CreateNodeArgs("Viewer"));
                } catch (...) {
                    Natron::errorDialog(QObject::tr("Project loader").toStdString(), QObject::tr("Error while loading auto-saved project").toStdString());
                    getApp()->createNode(CreateNodeArgs("Viewer"));
                }
                
                ///Process all events before flagging that we're no longer loading the project
                ///to avoid multiple renders being called because of reshape events of viewers
                QCoreApplication::processEvents();
                
                {
                    QMutexLocker l(&_imp->isLoadingProjectMutex);
                    _imp->isLoadingProject = false;
                }

                _imp->autoSetProjectFormat = false;

                if (exists) {
                    _imp->hasProjectBeenSavedByUser = true;
                    QString path = filename.left(filename.lastIndexOf(QDir::separator())+1);
                    filename = filename.remove(path);
                    _imp->projectName = filename;
                    _imp->projectPath = path;

                } else {
                    _imp->hasProjectBeenSavedByUser = false;
                    _imp->projectName = NATRON_PROJECT_UNTITLED;
                    _imp->projectPath.clear();
                }
                _imp->lastAutoSave = QDateTime::currentDateTime();
                _imp->ageSinceLastSave = QDateTime();

                emit projectNameChanged(_imp->projectName + " (*)");
                
                refreshViewersAndPreviews();
                
                return true;
            }
        }
    }
    removeAutoSaves();
    reset();
    return false;
}



void Project::initializeKnobs(){
    
    boost::shared_ptr<Page_Knob> page = Natron::createKnob<Page_Knob>(this, "Settings");
    
    _imp->formatKnob = Natron::createKnob<Choice_Knob>(this, "Output Format");
    const std::vector<Format*>& appFormats = appPTR->getFormats();
    std::vector<std::string> entries;
    for (U32 i = 0; i < appFormats.size(); ++i) {
        Format* f = appFormats[i];
        QString formatStr = Natron::generateStringFromFormat(*f);
        if(f->width() == 1920 && f->height() == 1080){
            _imp->formatKnob->setDefaultValue(i,0);
        }
        entries.push_back(formatStr.toStdString());
        _imp->builtinFormats.push_back(*f);
    }
    _imp->formatKnob->turnOffNewLine();

    _imp->formatKnob->populateChoices(entries);
    _imp->formatKnob->setAnimationEnabled(false);
    page->addKnob(_imp->formatKnob);
    _imp->addFormatKnob = Natron::createKnob<Button_Knob>(this,"New format...");
    page->addKnob(_imp->addFormatKnob);

    _imp->viewsCount = Natron::createKnob<Int_Knob>(this,"Number of views");
    _imp->viewsCount->setAnimationEnabled(false);
    _imp->viewsCount->setMinimum(1);
    _imp->viewsCount->setDisplayMinimum(1);
    _imp->viewsCount->setDefaultValue(1,0);
    _imp->viewsCount->disableSlider();
    _imp->viewsCount->turnOffNewLine();
    page->addKnob(_imp->viewsCount);
    
    _imp->mainView = Natron::createKnob<Int_Knob>(this, "Main view");
    _imp->mainView->disableSlider();
    _imp->mainView->setDefaultValue(0);
    _imp->mainView->setMinimum(0);
    _imp->mainView->setMaximum(0);
    _imp->mainView->setAnimationEnabled(false);
    page->addKnob(_imp->mainView);
    
    _imp->previewMode = Natron::createKnob<Bool_Knob>(this, "Auto previews");
    _imp->previewMode->setHintToolTip("When checked, preview images on the node graph will be "
                                      "refreshed automatically. You can uncheck this option to improve performances."
                                      "Press P in the node graph to refresh the previews yourself.");
    _imp->previewMode->setAnimationEnabled(false);
    page->addKnob(_imp->previewMode);
    bool autoPreviewEnabled = appPTR->getCurrentSettings()->isAutoPreviewOnForNewProjects();
    _imp->previewMode->setDefaultValue(autoPreviewEnabled,0);
    
    std::vector<std::string> colorSpaces;
    colorSpaces.push_back("sRGB");
    colorSpaces.push_back("Linear");
    colorSpaces.push_back("Rec.709");
    _imp->colorSpace8bits = Natron::createKnob<Choice_Knob>(this, "Colorspace for 8 bits images");
    _imp->colorSpace8bits->setHintToolTip("Defines the color-space in which 8 bits images are assumed to be by default.");
    _imp->colorSpace8bits->setAnimationEnabled(false);
    _imp->colorSpace8bits->populateChoices(colorSpaces);
    _imp->colorSpace8bits->setDefaultValue(0);
    
    _imp->colorSpace16bits = Natron::createKnob<Choice_Knob>(this, "Colorspace for 16 bits images");
    _imp->colorSpace16bits->setHintToolTip("Defines the color-space in which 16 bits images are assumed to be by default.");
    _imp->colorSpace16bits->setAnimationEnabled(false);
    _imp->colorSpace16bits->populateChoices(colorSpaces);
    _imp->colorSpace16bits->setDefaultValue(2);
    
    _imp->colorSpace32bits = Natron::createKnob<Choice_Knob>(this, "Colorspace for 32 bits fp images");
    _imp->colorSpace32bits->setHintToolTip("Defines the color-space in which 32 bits floating point images are assumed to be by default.");
    _imp->colorSpace32bits->setAnimationEnabled(false);
    _imp->colorSpace32bits->populateChoices(colorSpaces);
    _imp->colorSpace32bits->setDefaultValue(1);
    
    emit knobsInitialized();
    
}


void Project::evaluate(KnobI* /*knob*/,bool isSignificant,Natron::ValueChangedReason /*reason*/) {
    if(isSignificant){
        getApp()->checkViewersConnection();
    }
}

// don't return a reference to a mutex-protected object!
void Project::getProjectDefaultFormat(Format *f) const
{
    assert(f);
    QMutexLocker l(&_imp->formatMutex);
    int index = _imp->formatKnob->getValue();
    bool found = _imp->findFormat(index, f);
    assert(found);
}

///only called on the main thread
void Project::initNodeCountersAndSetName(Node* n) {
    assert(n);
    QMutexLocker l(&_imp->nodesLock);
    std::map<std::string,int>::iterator it = _imp->nodeCounters.find(n->pluginID());
    if(it != _imp->nodeCounters.end()){
        it->second++;
        n->setName(QString(QString(n->pluginLabel().c_str()) + QString::number(it->second)));
    }else{
        _imp->nodeCounters.insert(make_pair(n->pluginID(), 1));
        n->setName(QString(QString(n->pluginLabel().c_str()) + QString::number(1)));
    }
}
    
void Project::getNodeCounters(std::map<std::string,int>* counters) const {
    QMutexLocker l(&_imp->nodesLock);
    *counters = _imp->nodeCounters;
}

void Project::addNodeToProject(boost::shared_ptr<Natron::Node> n) {
    QMutexLocker l(&_imp->nodesLock);
    _imp->currentNodes.push_back(n);
}
    
void Project::removeNodeFromProject(const boost::shared_ptr<Natron::Node>& n)
{
    assert(QThread::currentThread() == qApp->thread());
    {
        QMutexLocker l(&_imp->nodesLock);
        for (std::vector<boost::shared_ptr<Natron::Node> >::iterator it = _imp->currentNodes.begin(); it!=_imp->currentNodes.end(); ++it) {
            if (*it == n) {
                _imp->currentNodes.erase(it);
                break;
            }
        }
    }
    n->removeReferences();
}
    
void Project::clearNodes(bool emitSignal) {
    std::vector<boost::shared_ptr<Natron::Node> > nodesToDelete;
    {
        QMutexLocker l(&_imp->nodesLock);
        nodesToDelete = _imp->currentNodes;
    }
    
    ///First quit any processing
    for (U32 i = 0; i < nodesToDelete.size(); ++i) {
        nodesToDelete[i]->quitAnyProcessing();
    }
    ///Kill thread pool so threads are killed before killing thread storage
    QThreadPool::globalInstance()->waitForDone();

    ///Kill effects
    for (U32 i = 0; i < nodesToDelete.size(); ++i) {
        nodesToDelete[i]->deactivate(std::list< boost::shared_ptr<Natron::Node> >(),true,false);
        nodesToDelete[i]->removeReferences();
    }
    {
        QMutexLocker l(&_imp->nodesLock);
        _imp->currentNodes.clear();
    }

    nodesToDelete.clear();

    if (emitSignal) {
        emit nodesCleared();
    }
}

void Project::setFrameRange(int first, int last){
    QMutexLocker l(&_imp->timelineMutex);
    _imp->timeline->setFrameRange(first,last);
}


int Project::currentFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->currentFrame();
}

int Project::firstFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->firstFrame();
}

int Project::lastFrame() const {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->lastFrame();
}

int Project::leftBound() const  {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->leftBound();
}

int Project::rightBound() const  {
    QMutexLocker l(&_imp->timelineMutex);
    return _imp->timeline->rightBound();
}

int Project::tryAddProjectFormat(const Format& f){
    assert(!_imp->formatMutex.tryLock());
    if(f.left() >= f.right() || f.bottom() >= f.top()){
        return -1;
    }
    
    std::list<Format>::iterator foundFormat = std::find(_imp->builtinFormats.begin(), _imp->builtinFormats.end(), f);
    if (foundFormat != _imp->builtinFormats.end()) {
        return std::distance(_imp->builtinFormats.begin(),foundFormat);
    } else {
        foundFormat = std::find(_imp->additionalFormats.begin(), _imp->additionalFormats.end(), f);
        if (foundFormat != _imp->additionalFormats.end()) {
            return std::distance(_imp->additionalFormats.begin(),foundFormat);
        }
    }
 
    std::vector<std::string> entries;
    for (std::list<Format>::iterator it = _imp->builtinFormats.begin(); it!=_imp->builtinFormats.end();++it) {
        const Format& f = *it;
        QString formatStr = generateStringFromFormat(f);
        entries.push_back(formatStr.toStdString());
    }
    for (std::list<Format>::iterator it = _imp->additionalFormats.begin(); it!=_imp->additionalFormats.end();++it) {
        const Format& f = *it;
        QString formatStr = generateStringFromFormat(f);
        entries.push_back(formatStr.toStdString());
    }
    QString formatStr = generateStringFromFormat(f);
    entries.push_back(formatStr.toStdString());
    _imp->additionalFormats.push_back(f);
    _imp->formatKnob->populateChoices(entries);
    return (_imp->builtinFormats.size() + _imp->additionalFormats.size()) - 1;
}

void Project::setProjectDefaultFormat(const Format& f) {
    assert(!_imp->formatMutex.tryLock());
    int index = tryAddProjectFormat(f);
    _imp->formatKnob->setValue(index,0);
    ///if locked it will trigger a deadlock because some parameters
    ///might respond to this signal by checking the content of the project format.
}


int Project::getProjectViewsCount() const {
    return _imp->viewsCount->getValue();
}

int Project::getProjectMainView() const
{
    return _imp->mainView->getValue();
}
    
std::vector<boost::shared_ptr<Natron::Node> > Project::getCurrentNodes() const {
    QMutexLocker l(&_imp->nodesLock);
    return _imp->currentNodes;
}

bool Project::hasNodes() const
{
    QMutexLocker l(&_imp->nodesLock);
    return !_imp->currentNodes.empty();
}
    
QString Project::getProjectName() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->projectName;
}


QString Project::getLastAutoSaveFilePath() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->lastAutoSaveFilePath;
}

bool Project::hasEverAutoSaved() const
{
    return !getLastAutoSaveFilePath().isEmpty();
}
    
QString Project::getProjectPath() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->projectPath;
}


bool Project::hasProjectBeenSavedByUser() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->hasProjectBeenSavedByUser;
}

#if 0 // dead code
QDateTime Project::getProjectAgeSinceLastSave() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->ageSinceLastSave;
}

QDateTime Project::getProjectAgeSinceLastAutosave() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->lastAutoSave;
}
#endif

bool Project::isAutoPreviewEnabled() const {
    QMutexLocker l(&_imp->previewModeMutex);
    return _imp->previewMode->getValue();
}

void Project::toggleAutoPreview() {
    QMutexLocker l(&_imp->previewModeMutex);
    _imp->previewMode->setValue(!_imp->previewMode->getValue(),0);
}

boost::shared_ptr<TimeLine> Project::getTimeLine() const  {return _imp->timeline;}

void
Project::getAdditionalFormats(std::list<Format> *formats) const
{
    assert(formats);
    QMutexLocker l(&_imp->formatMutex);
    *formats = _imp->additionalFormats;
}


void Project::setLastTimelineSeekCaller(Natron::OutputEffectInstance* output) {
    QMutexLocker l(&_imp->projectLock);
    _imp->lastTimelineSeekCaller = output;
}

Natron::OutputEffectInstance* Project::getLastTimelineSeekCaller() const
{
    QMutexLocker l(&_imp->projectLock);
    return _imp->lastTimelineSeekCaller;
}
    
bool Project::isSaveUpToDate() const{
    QMutexLocker l(&_imp->projectLock);
    return _imp->ageSinceLastSave == _imp->lastAutoSave;
}


void Project::save(ProjectSerialization* serializationObject) const
{
    serializationObject->initialize(this);
}

void Project::load(const ProjectSerialization& obj)
{
    _imp->nodeCounters.clear();
    _imp->restoreFromSerialization(obj);
    Format f;
    getProjectDefaultFormat(&f);
    emit formatChanged(f);
}

void Project::beginProjectWideValueChanges(Natron::ValueChangedReason reason,KnobHolder* caller)
{
    assert(QThread::currentThread() == qApp->thread());
    
    ///increase the begin calls count
    ++_imp->beginEndBracketsCount;
    
    ///If begin was already called on this caller, increase the calls count for this caller, otherwise
    ///insert a new entry in the holdersWhoseBeginWasCalled map with a call count of 1 and the reason.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);

    if(found == _imp->holdersWhoseBeginWasCalled.end()){
        
        //getApp()->unlockProject();
        caller->beginKnobsValuesChanged_public(reason);
        //getApp()->lockProject();
        
        ///insert the caller in the map
        _imp->holdersWhoseBeginWasCalled.insert(std::make_pair(caller,std::make_pair(1,reason)));
    }else{
        
        ///increase the begin calls count of 1
        ++found->second.first;
    }


}

void Project::stackEvaluateRequest(Natron::ValueChangedReason reason,KnobHolder* caller,KnobI* k,bool isSignificant)
{
    assert(QThread::currentThread() == qApp->thread());

    ///This function may be called outside of a begin/end bracket call, in which case we call them ourselves.
    
    bool mustEndBracket = false;

    ///if begin was not called for this caller, call it ourselves
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    if (found == _imp->holdersWhoseBeginWasCalled.end() || _imp->beginEndBracketsCount == 0) {
        beginProjectWideValueChanges(reason,caller);
        ///flag that we called begin
        mustEndBracket = true;
    } else {
        ///THIS IS WRONG AND COMMENTED OUT : THIS LEADS TO INFINITE RECURSION.
        ///if we found a call made to begin already for this caller, adjust the reason to the reason of the
        /// outermost begin call
        //   reason = found->second.second;
    }

    ///if the evaluation is significant , set the flag isSignificantChange to true
    _imp->isSignificantChange |= isSignificant;

    ///increase the count of evaluation requests
    ++_imp->evaluationsCount;
    
    ///remember the last caller, this is the one on which we will call evaluate
    _imp->lastKnobChanged = k;

    ///if the reason of the outermost begin call is OTHER_REASON then we don't call
    ///the onKnobValueChanged. This way the plugin can avoid infinite recursions by doing so:
    /// beginValueChange(PROJECT_LOADING)
    /// ...
    /// endValueChange()
    if (reason != Natron::PROJECT_LOADING) {
        caller->onKnobValueChanged_public(k,reason);
    }
    
    ////if begin was not call prior to calling this function, call the end bracket oruselves
    if (mustEndBracket) {
        endProjectWideValueChanges(caller);
    }
}

void Project::endProjectWideValueChanges(KnobHolder* caller){

    assert(QThread::currentThread() == qApp->thread());
    
    ///decrease the beginEndBracket count
    --_imp->beginEndBracketsCount;
    
    ///Get the count of begin calls made to this caller.
    ///It must absolutely be present in holdersWhoseBeginWasCalled because we either added it by
    ///calling stackEvaluateRequest or beginProjectWideValueChanges
    ///This means that calling endProjectWideValueChanges twice in a row will crash in the assertion below.
    ProjectPrivate::KnobsValueChangedMap::iterator found = _imp->holdersWhoseBeginWasCalled.find(caller);
    assert(found != _imp->holdersWhoseBeginWasCalled.end());
    
    Natron::ValueChangedReason outerMostReason = found->second.second;
    
    ///If we're closing the last bracket, call the caller portion of endKnobsValuesChanged
    if(found->second.first == 1){
        
        ///remove the caller from the holdersWhoseBeginWasCalled map
        _imp->holdersWhoseBeginWasCalled.erase(found);
        
        caller->endKnobsValuesChanged_public(outerMostReason);

        
    }else{
        ///we're not on the last begin/end bracket for this caller, just decrease the begin calls count
        --found->second.first;
    }
    
    ///if we're not in the last begin/end bracket (globally to all callers) then return
    if(_imp->beginEndBracketsCount != 0){
        return;
    }
    
    ///If we reach here that means that we're closing the last global bracket.
    ///If the bracket was empty of evaluation we don't have to do anything.
    if(_imp->evaluationsCount != 0){
        
        ///reset the evaluation count to 0
        _imp->evaluationsCount = 0;
        
        ///if the outermost bracket reason was USER_EDITED and the knob was not a button, trigger an auto-save.
        if(outerMostReason == Natron::USER_EDITED && !dynamic_cast<Button_Knob*>(_imp->lastKnobChanged)){
            getApp()->triggerAutoSave();
        }
        
        ///if the outermost bracket reason was not PROJECT_LOADING or TIME_CHANGED, then call evaluate
        ///on the last caller with
        ///the significant param recorded in the stackEvaluateRequest function.
        if(outerMostReason != Natron::PROJECT_LOADING && outerMostReason != Natron::TIME_CHANGED){
            
            caller->evaluate_public(_imp->lastKnobChanged,_imp->isSignificantChange,outerMostReason);
            
        }
    }
    
    ////reset back the isSignificantChange flag to false, otherwise next insignificant evaluations
    ////would be evaluated.
    _imp->isSignificantChange = false;
    
    ///the stack must be empty, i.e: crash if the user didn't correctly end the brackets
    assert(_imp->holdersWhoseBeginWasCalled.empty());
    
}

void Project::beginKnobsValuesChanged(Natron::ValueChangedReason /*reason*/){}

void Project::endKnobsValuesChanged(Natron::ValueChangedReason /*reason*/) {}



///this function is only called on the main thread
void Project::onKnobValueChanged(KnobI* knob,Natron::ValueChangedReason /*reason*/) {
    if (knob == _imp->viewsCount.get()) {
        int viewsCount = _imp->viewsCount->getValue();
        getApp()->setupViewersForViews(viewsCount);
        
        int mainView = _imp->mainView->getValue();
        if (mainView >= viewsCount) {
            ///reset view to 0
            _imp->mainView->setValue(0, 0);
        }
        _imp->mainView->setMaximum(viewsCount - 1);
    } else if(knob == _imp->formatKnob.get()) {
        int index = _imp->formatKnob->getValue();
        Format frmt;
        bool found = _imp->findFormat(index, &frmt);
        if (found) {
            emit formatChanged(frmt);
        }
    } else if(knob == _imp->addFormatKnob.get()) {
        emit mustCreateFormat();
    } else if (knob == _imp->previewMode.get()) {
        emit autoPreviewChanged(_imp->previewMode->getValue());
    }
    
}


bool Project::isLoadingProject() const {
    QMutexLocker l(&_imp->isLoadingProjectMutex);
    return _imp->isLoadingProject;
}

bool Project::isGraphWorthLess() const {
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
    return !hasEverAutoSaved() && !hasProjectBeenSavedByUser();
}

void Project::removeAutoSaves() {
    /*removing all autosave files*/
    QDir savesDir(autoSavesDir());
    QStringList entries = savesDir.entryList();
    for(int i = 0; i < entries.size();++i) {
        const QString& entry = entries.at(i);
        QString searchStr('.');
        searchStr.append(NATRON_PROJECT_FILE_EXT);
        searchStr.append('.');
        int suffixPos = entry.indexOf(searchStr);
        if (suffixPos != -1) {
            QFile::remove(savesDir.path()+QDir::separator()+entry);
        }
    }
}

QString Project::autoSavesDir() {
    return Natron::StandardPaths::writableLocation(Natron::StandardPaths::DataLocation) + QDir::separator() + "Autosaves";
}

void Project::reset() {
    {
        QMutexLocker l(&_imp->projectLock);
        _imp->autoSetProjectFormat = true;
        _imp->hasProjectBeenSavedByUser = false;
        _imp->projectCreationTime = QDateTime::currentDateTime();
        _imp->projectName = NATRON_PROJECT_UNTITLED;
        _imp->projectPath.clear();
        _imp->autoSaveTimer->stop();
        _imp->additionalFormats.clear();
    }
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    
    emit projectNameChanged(NATRON_PROJECT_UNTITLED);
    clearNodes();
}

void Project::setOrAddProjectFormat(const Format& frmt,bool skipAdd) {

    
    Format dispW;
    bool formatSet = false;
    {
        QMutexLocker l(&_imp->formatMutex);
        
        if(_imp->autoSetProjectFormat){
            _imp->autoSetProjectFormat = false;
            dispW = frmt;
            
            Format* df = appPTR->findExistingFormat(dispW.width(), dispW.height(),dispW.getPixelAspect());
            if (df) {
                dispW.setName(df->getName());
                setProjectDefaultFormat(dispW);
            } else {
                setProjectDefaultFormat(dispW);
            }
            formatSet = true;
        } else if(!skipAdd) {
            dispW = frmt;
            tryAddProjectFormat(dispW);
            
        }
    }
    if (formatSet) {
        emit formatChanged(dispW);
    }


}

///do not need to lock this function as all calls are thread-safe already
bool Project::connectNodes(int inputNumber,const std::string& parentName,boost::shared_ptr<Node> output){
    const std::vector<boost::shared_ptr<Node> > nodes = getCurrentNodes();
    for (U32 i = 0; i < nodes.size(); ++i) {
        assert(nodes[i]);
        if (nodes[i]->getName() == parentName) {
            return connectNodes(inputNumber,nodes[i], output);
        }
    }
    return false;
}

bool Project::connectNodes(int inputNumber,boost::shared_ptr<Node> input,boost::shared_ptr<Node> output,bool force) {

    ////Only called by the main-thread
    assert(QThread::currentThread() == qApp->thread());
    
    boost::shared_ptr<Node> existingInput = output->input(inputNumber);
    if (force && existingInput) {
        bool ok = disconnectNodes(existingInput, output);
        assert(ok);
        if (input->maximumInputs() > 0) {
            ok = connectNodes(input->getPreferredInputForConnection(), existingInput, input);
            assert(ok);
        }
    }

    if(!output->connectInput(input, inputNumber)){
        return false;
    }
    if(!input){
        return true;
    }
    input->connectOutput(output);
    return true;
}
bool Project::disconnectNodes(boost::shared_ptr<Node> input,boost::shared_ptr<Node> output,bool autoReconnect) {

    boost::shared_ptr<Node> inputToReconnectTo;
    int indexOfInput = output->inputIndex(input.get());
    if (indexOfInput == -1) {
        return false;
    }

    int inputsCount = input->maximumInputs();
    if (inputsCount == 1) {
        inputToReconnectTo = input->input(0);
    }

    if(input->disconnectOutput(output) < 0){
        return false;
    }
    if(output->disconnectInput(input) < 0){
        return false;
    }

    if (autoReconnect && inputToReconnectTo) {
        bool ok = connectNodes(indexOfInput, inputToReconnectTo, output);
        assert(ok);
    }

    return true;
}

bool Project::tryLock() const {
    return _imp->projectLock.tryLock();
}

void Project::unlock() const {
    assert(!_imp->projectLock.tryLock());
    _imp->projectLock.unlock();
}

bool Project::autoConnectNodes(boost::shared_ptr<Node> selected,boost::shared_ptr<Node> created) {
    ///We follow this rule:
    //        1) selected is output
    //          a) created is output --> fail
    //          b) created is input --> connect input
    //          c) created is regular --> connect input
    //        2) selected is input
    //          a) created is output --> connect output
    //          b) created is input --> fail
    //          c) created is regular --> connect output
    //        3) selected is regular
    //          a) created is output--> connect output
    //          b) created is input --> connect input
    //          c) created is regular --> connect output

    ///if true if will connect 'created' as input of 'selected',
    ///otherwise as output.
    bool connectAsInput = false;

    ///cannot connect 2 input nodes together: case 2-b)
    if (selected->maximumInputs() == 0 && created->maximumInputs() == 0) {
        return false;
    }
    ///cannot connect 2 output nodes together: case 1-a)
    if (selected->isOutputNode() && created->isOutputNode()) {
        return false;
    }

    ///1)
    if (selected->isOutputNode()) {

        ///assert we're not in 1-a)
        assert(!created->isOutputNode());

        ///for either cases 1-b) or 1-c) we just connect the created node as input of the selected node.
        connectAsInput = true;
    }
    ///2) and 3) are similar exceptfor case b)
    else {

        ///case 2 or 3- a): connect the created node as output of the selected node.
        if (created->isOutputNode()) {
            connectAsInput = false;
        }
        ///case b)
        else if (created->maximumInputs() == 0) {
            assert(selected->maximumInputs() != 0);
            ///case 3-b): connect the created node as input of the selected node
            connectAsInput = true;
            
        }
        ///case c) connect created as output of the selected node
        else {
            connectAsInput = false;
        }
    }

    bool ret = false;
    if (connectAsInput) {
        ///if the selected node is and inspector, we want to connect the created node on the active input
        boost::shared_ptr<InspectorNode> inspector = boost::dynamic_pointer_cast<InspectorNode>(selected);
        if (inspector) {
            int activeInputIndex = inspector->activeInput();
            bool ok = connectNodes(activeInputIndex, created, selected,true);
            assert(ok);
            ret = true;
        } else {
            ///connect it to the first input
            int selectedInput = selected->getPreferredInputForConnection();
            if (selectedInput != -1) {
                bool ok = connectNodes(selectedInput, created, selected,true);
                assert(ok);
                ret = true;
            } else {
                ret = false;
            }
        }
    } else {

        if (!created->isOutputNode()) {
            ///we find all the nodes that were previously connected to the selected node,
            ///and connect them to the created node instead.
            std::map<boost::shared_ptr<Node>,int> outputsConnectedToSelectedNode;
            selected->getOutputsConnectedToThisNode(&outputsConnectedToSelectedNode);
            for (std::map<boost::shared_ptr<Node>,int>::iterator it = outputsConnectedToSelectedNode.begin();
                 it!=outputsConnectedToSelectedNode.end(); ++it) {
                bool ok = disconnectNodes(selected, it->first);
                assert(ok);

                ok = connectNodes(it->second, created, it->first);
                assert(ok);
            }

        }
        ///finally we connect the created node to the selected node
        int createdInput = created->getPreferredInputForConnection();
        if (createdInput != -1) {
            bool ok = connectNodes(createdInput, selected, created);
            assert(ok);
            ret = true;
        } else {
            ret = false;
        }
    }

    ///update the render trees
    std::list<ViewerInstance* > viewers;
    created->hasViewersConnected(&viewers);
    for(std::list<ViewerInstance* >::iterator it = viewers.begin();it!=viewers.end();++it){
        (*it)->updateTreeAndRender();
    }
    return ret;

}
    
qint64 Project::getProjectCreationTime() const {
    QMutexLocker l(&_imp->projectLock);
    return _imp->projectCreationTime.toMSecsSinceEpoch();
}
    
boost::shared_ptr<Natron::Node> Project::getNodeByName(const std::string& name) const {
    QMutexLocker l(&_imp->nodesLock);
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (_imp->currentNodes[i]->isActivated() && _imp->currentNodes[i]->getName() == name) {
            return _imp->currentNodes[i];
        }
    }
    return boost::shared_ptr<Natron::Node>();
}
    
boost::shared_ptr<Natron::Node> Project::getNodePointer(Natron::Node* n) const
{
    QMutexLocker l(&_imp->nodesLock);
    for (U32 i = 0; i < _imp->currentNodes.size(); ++i) {
        if (_imp->currentNodes[i].get() == n) {
            return _imp->currentNodes[i];
        }
    }
    return boost::shared_ptr<Natron::Node>();
}
    
Natron::ViewerColorSpace Project::getDefaultColorSpaceForBitDepth(Natron::ImageBitDepth bitdepth) const
{
    switch (bitdepth) {
        case Natron::IMAGE_BYTE:
            return (Natron::ViewerColorSpace)_imp->colorSpace8bits->getValue();
        case Natron::IMAGE_SHORT:
            return (Natron::ViewerColorSpace)_imp->colorSpace16bits->getValue();
        case Natron::IMAGE_FLOAT:
            return (Natron::ViewerColorSpace)_imp->colorSpace32bits->getValue();
        default:
            assert(false);
            break;
    }
}
    
} //namespace Natron
