//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "Settings.h"

#include <QtCore/QDebug>
#include <QDir>
#include <QSettings>
#include <QThreadPool>
#include <QThread>
#include "Global/MemoryInfo.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/LibraryBinary.h"
#include "Engine/KnobTypes.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobFactory.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

using namespace Natron;


Settings::Settings(AppInstance* appInstance)
: KnobHolder(appInstance)
, _wereChangesMadeSinceLastSave(false)
{
    
}

static QString getDefaultOcioConfigPath() {
    QString binaryPath = appPTR->getApplicationBinaryPath();
    if (!binaryPath.isEmpty()) {
        binaryPath += QDir::separator();
    }
#ifdef __NATRON_LINUX__
    return binaryPath + "../share/OpenColorIO-Configs";
#elif defined(__NATRON_WIN32__)
    return binaryPath + "../Resources/OpenColorIO-Configs";
#elif defined(__NATRON_OSX__)
    return binaryPath + "../Resources/OpenColorIO-Configs";
#endif
}

void Settings::initializeKnobs(){
    
    _generalTab = Natron::createKnob<Page_Knob>(this, "General");
    
    _checkForUpdates = Natron::createKnob<Bool_Knob>(this, "Always check for updates on start-up");
    _checkForUpdates->setAnimationEnabled(false);
    _checkForUpdates->setHintToolTip("When checked, " NATRON_APPLICATION_NAME " will check for new updates on start-up of the application.");
    _generalTab->addKnob(_checkForUpdates);
    
    _autoSaveDelay = Natron::createKnob<Int_Knob>(this, "Auto-save trigger delay");
    _autoSaveDelay->setAnimationEnabled(false);
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip("The number of seconds after an event that " NATRON_APPLICATION_NAME " should wait before "
                                   " auto-saving. Note that if a render is in progress, " NATRON_APPLICATION_NAME " will "
                                   " wait until it is done to actually auto-save.");
    _generalTab->addKnob(_autoSaveDelay);
    
    _maxUndoRedoNodeGraph = Natron::createKnob<Int_Knob>(this, "Maximum undo/redo for the node graph");
    _maxUndoRedoNodeGraph->setAnimationEnabled(false);
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setMinimum(0);
    _maxUndoRedoNodeGraph->setMaximum(100);
    _maxUndoRedoNodeGraph->setHintToolTip("Set the maximum of events related to the node graph " NATRON_APPLICATION_NAME
                                 " will remember. Past this limit, older events will be deleted permanantly "
                                 " allowing to re-use the RAM for better purposes since Nodes can hold a significant amount of RAM. \n"
                                          "Changing this value will clear the undo/redo stack.");
    _generalTab->addKnob(_maxUndoRedoNodeGraph);
    
    _linearPickers = Natron::createKnob<Bool_Knob>(this, "Linear color pickers");
    _linearPickers->setAnimationEnabled(false);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters will be converted"
                                   " to linear before being fetched. Otherwise they will be in the same color-space "
                                   " as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);
    
    _numberOfThreads = Natron::createKnob<Int_Knob>(this, "Number of render threads");
    _numberOfThreads->setAnimationEnabled(false);
    QString numberOfThreadsToolTip = QString("Controls how many threads " NATRON_APPLICATION_NAME " should use to render. \n"
                                             "-1: Disable multi-threading totally (useful for debug) \n"
                                             "0: Guess from the number of cores. The ideal threads count for your hardware is %1.").arg(QThread::idealThreadCount());
    _numberOfThreads->setHintToolTip(numberOfThreadsToolTip.toStdString());
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _generalTab->addKnob(_numberOfThreads);
    
    _renderInSeparateProcess = Natron::createKnob<Bool_Knob>(this, "Render in a separate process");
    _renderInSeparateProcess->setAnimationEnabled(false);
    _renderInSeparateProcess->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will render (using the write nodes) in "
                                             "a separate process. Disabling it is most helpful for the dev team.");
    _generalTab->addKnob(_renderInSeparateProcess);
    
    _autoPreviewEnabledForNewProjects = Natron::createKnob<Bool_Knob>(this, "Auto-preview enabled by default for new projects");
    _autoPreviewEnabledForNewProjects->setAnimationEnabled(false);
    _autoPreviewEnabledForNewProjects->setHintToolTip("If checked then when creating a new project, the Auto-preview option"
                                                      " will be enabled.");
    _generalTab->addKnob(_autoPreviewEnabledForNewProjects);
    
    
    _snapNodesToConnections = Natron::createKnob<Bool_Knob>(this, "Snap to node");
    _snapNodesToConnections->setHintToolTip("When moving nodes on the node graph, snap them to positions where it lines them up "
                                            "with the inputs and output nodes.");
    _snapNodesToConnections->setDefaultValue(true);
    _snapNodesToConnections->setAnimationEnabled(false);
    _generalTab->addKnob(_snapNodesToConnections);
    
    _maxPanelsOpened = Natron::createKnob<Int_Knob>(this, "Maximum number of node settings panels opened");
    _maxPanelsOpened->setHintToolTip("This property holds the number of node settings pnaels that can be "
                                     "held by the properties dock at the same time."
                                     "The special value of 0 indicates there can be an unlimited number of panels opened.");
    _maxPanelsOpened->setAnimationEnabled(false);
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(0);
    _maxPanelsOpened->setMaximum(100);
    _generalTab->addKnob(_maxPanelsOpened);
    
    _generalTab->addKnob(Natron::createKnob<Separator_Knob>(this, "OpenFX Plugins"));
    
    _extraPluginPaths = Natron::createKnob<Path_Knob>(this, "Extra plugins search paths");
    _extraPluginPaths->setHintToolTip("All paths in this variable are separated by ';' and indicate"
                                      " extra search paths where " NATRON_APPLICATION_NAME " should scan for plug-ins. "
                                      NATRON_APPLICATION_NAME " already searchs for plug-ins at these locations:\n "
                                      " C:\\Program Files\\Common Files\\OFX\\Plugins on Windows, \n "
                                      " /usr/OFX/Plugins on Linux and \n "
                                      " /Library/OFX/Plugins on MacOSX. \n"
                                      " The changes made to the OpenFX plugins search paths will take effect"
                                      " upon the next launch of " NATRON_APPLICATION_NAME);
    _extraPluginPaths->setMultiPath(true);
    _generalTab->addKnob(_extraPluginPaths);
    
    boost::shared_ptr<Page_Knob> ocioTab = Natron::createKnob<Page_Knob>(this, "OpenColorIO");
    
    
    _ocioConfigKnob = Natron::createKnob<Choice_Knob>(this, "OpenColorIO config");
    _ocioConfigKnob->setAnimationEnabled(false);
    
    QString defaultOcioConfigsPath = getDefaultOcioConfigPath();
    QDir ocioConfigsDir(defaultOcioConfigsPath);
    std::vector<std::string> configs;
    int defaultIndex = 0;
    if (ocioConfigsDir.exists()) {
        QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        for (int i = 0; i < entries.size(); ++i) {
            if (entries[i].contains("nuke-default")) {
                defaultIndex = i;
            }
            configs.push_back(entries[i].toStdString());
        }
    }
    configs.push_back(NATRON_CUSTOM_OCIO_CONFIG_NAME);
    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setValue(defaultIndex,0);
    _ocioConfigKnob->setHintToolTip("Select the OpenColorIO config you would like to use globally for all "
                                    "operators that use OpenColorIO. Note that changing it will set the OCIO "
                                    "environment variable, hence any change to this parameter will be "
                                    "taken into account on the next application launch. "
                                    "When custom config is selected, you can use the custom OpenColorIO config file "
                                    "setting to point to the config you would like to use.");
    
    ocioTab->addKnob(_ocioConfigKnob);
    
    _customOcioConfigFile = Natron::createKnob<File_Knob>(this, "Custom OpenColorIO config file");
    _customOcioConfigFile->setAllDimensionsEnabled(false);
    _customOcioConfigFile->setHintToolTip("To use this, set the OpenColorIO config to custom config a point "
                                          "to a custom OpenColorIO config file (.ocio).");
    ocioTab->addKnob(_customOcioConfigFile);
    
    _viewersTab = Natron::createKnob<Page_Knob>(this, "Viewers");
    
    _texturesMode = Natron::createKnob<Choice_Knob>(this, "Viewer textures bit depth");
    _texturesMode->setAnimationEnabled(false);
    std::vector<std::string> textureModes;
    std::vector<std::string> helpStringsTextureModes;
    textureModes.push_back("Byte");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the software. Cached textures will be smaller in  the viewer cache.");
    textureModes.push_back("16bits half-float");
    helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back("32bits floating-point");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the hardware using GLSL. Cached textures will be larger in the viewer cache.");
    _texturesMode->populateChoices(textureModes,helpStringsTextureModes);
    _texturesMode->setHintToolTip("Bitdepth of the viewer textures used for rendering."
                                  " Hover each option with the mouse for a more detailed comprehension.");
    _viewersTab->addKnob(_texturesMode);
    
    _powerOf2Tiling = Natron::createKnob<Int_Knob>(this, "Viewer tile size is 2 to the power of...");
    _powerOf2Tiling->setHintToolTip("The power of 2 of the tiles size used by the Viewer to render."
                                    " A high value means that the viewer will usually render big tiles, which means"
                                    " you have good chances when panning/zooming to find an already rendered texture in the cache."
                                    " On the other hand a small value means that the tiles will be closer to the real size of"
                                    " images to be rendered and as a result of this there might be more cache misses." );
    _powerOf2Tiling->setMinimum(4);
    _powerOf2Tiling->setDisplayMinimum(4);
    _powerOf2Tiling->setMaximum(9);
    _powerOf2Tiling->setDisplayMaximum(9);
    
    _powerOf2Tiling->setAnimationEnabled(false);
    _viewersTab->addKnob(_powerOf2Tiling);
    
    _cachingTab = Natron::createKnob<Page_Knob>(this, "Caching");
    
    _maxRAMPercent = Natron::createKnob<Int_Knob>(this, "Maximum system's RAM for caching");
    _maxRAMPercent->setAnimationEnabled(false);
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    std::string ramHint("This setting indicates the percentage of the system's total RAM "
                        NATRON_APPLICATION_NAME "'s caches are allowed to use."
                        " Your system has ");
    ramHint.append(printAsRAM(getSystemTotalRAM()).toStdString());
    ramHint.append(" of RAM.");
    if (isApplication32Bits()) {
        ramHint.append("\n The version of " NATRON_APPLICATION_NAME " you are running is 32 bits, that means the system's total RAM "
                       "will be limited to 4Gib. In this case the percentage of RAM allowed will be min(4Gib,SystemtotalRAM) * MaxRamPercent.");
    }
    
    _maxRAMPercent->setHintToolTip(ramHint);
    _cachingTab->addKnob(_maxRAMPercent);
    
    _maxPlayBackPercent = Natron::createKnob<Int_Knob>(this, "Playback cache RAM percentage");
    _maxPlayBackPercent->setAnimationEnabled(false);
    _maxPlayBackPercent->setMinimum(0);
    _maxPlayBackPercent->setMaximum(100);
    _maxPlayBackPercent->setHintToolTip("This setting indicates the percentage of the Maximum system's RAM for caching"
                                        " dedicated for the playback cache. Normally you shouldn't change this value"
                                        " as it is tuned automatically by the Maximum system's RAM for caching, but"
                                        " this is made possible for convenience.");
    _cachingTab->addKnob(_maxPlayBackPercent);
    
    _maxDiskCacheGB = Natron::createKnob<Int_Knob>(this, "Maximum disk cache size");
    _maxDiskCacheGB->setAnimationEnabled(false);
    _maxDiskCacheGB->setMinimum(0);
    _maxDiskCacheGB->setMaximum(100);
    _maxDiskCacheGB->setHintToolTip("The maximum disk space the caches can use. (in GB)");
    _cachingTab->addKnob(_maxDiskCacheGB);
    
    
    ///readers & writers settings are created in a postponed manner because we don't know
    ///their dimension yet. See populateReaderPluginsAndFormats & populateWriterPluginsAndFormats
    
    _readersTab = Natron::createKnob<Page_Knob>(this, "Readers");

    _writersTab = Natron::createKnob<Page_Knob>(this, "Writers");
    
    
    setDefaultValues();
}


void Settings::setDefaultValues() {
    
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    _checkForUpdates->setDefaultValue(false);
    _autoSaveDelay->setDefaultValue(5, 0);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _linearPickers->setDefaultValue(true,0);
    _numberOfThreads->setDefaultValue(0,0);
    _renderInSeparateProcess->setDefaultValue(true,0);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true,0);
    _maxPanelsOpened->setDefaultValue(10,0);
    _extraPluginPaths->setDefaultValue("",0);
    _texturesMode->setDefaultValue(0,0);
    _powerOf2Tiling->setDefaultValue(8,0);
    _maxRAMPercent->setDefaultValue(50,0);
    _maxPlayBackPercent->setDefaultValue(25,0);
    _maxDiskCacheGB->setDefaultValue(10,0);

    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _readersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("ReadOIIOOFX")) {
                _readersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }
    
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("WriteOIIOOFX")) {
                _writersMapping[i]->setDefaultValue(j,0);
                break;
            }
        }
    }
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
}

void Settings::saveSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    settings.setValue("CheckUpdates", _checkForUpdates->getValue());
    settings.setValue("AutoSaveDelay", _autoSaveDelay->getValue());
    settings.setValue("MaximumUndoRedoNodeGraph", _maxUndoRedoNodeGraph->getValue());
    settings.setValue("LinearColorPickers",_linearPickers->getValue());
    settings.setValue("Number of threads", _numberOfThreads->getValue());
    settings.setValue("RenderInSeparateProcess", _renderInSeparateProcess->getValue());
    settings.setValue("AutoPreviewDefault", _autoPreviewEnabledForNewProjects->getValue());
    settings.setValue("SnapToNode",_snapNodesToConnections->getValue());
    settings.setValue("MaxPanelsOpened", _maxPanelsOpened->getValue());
    settings.setValue("ExtraPluginsPaths", _extraPluginPaths->getValue().c_str());
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    std::string configList = _customOcioConfigFile->getValue();
    if (!configList.empty()) {
        settings.setValue("OCIOConfigFile", configList.c_str());
    }
    settings.setValue("OCIOConfig", _ocioConfigKnob->getValue());
    settings.endGroup();
    
    settings.beginGroup("Caching");
    settings.setValue("MaximumRAMUsagePercentage", _maxRAMPercent->getValue());
    settings.setValue("MaximumPlaybackRAMUsage", _maxPlayBackPercent->getValue());
    settings.setValue("MaximumDiskSizeUsage", _maxDiskCacheGB->getValue());
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    settings.setValue("ByteTextures", _texturesMode->getValue());
    settings.setValue("TilesPowerOf2", _powerOf2Tiling->getValue());
    settings.endGroup();
    
    settings.beginGroup("Readers");
    
    
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        settings.setValue(_readersMapping[i]->getDescription().c_str(),_readersMapping[i]->getValue());
    }
    settings.endGroup();
    
    settings.beginGroup("Writers");
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        settings.setValue(_writersMapping[i]->getDescription().c_str(),_writersMapping[i]->getValue());
    }
    settings.endGroup();
    
}

void Settings::restoreSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    notifyProjectBeginKnobsValuesChanged(Natron::PROJECT_LOADING);
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    if (settings.contains("CheckUpdates")) {
        _checkForUpdates->setValue(settings.value("CheckUpdates").toBool(), 0);
    }
    if (settings.contains("AutoSaveDelay")) {
        _autoSaveDelay->setValue(settings.value("AutoSaveDelay").toInt(),0);
    }
    if (settings.contains("MaximumUndoRedoNodeGraph")) {
        _maxUndoRedoNodeGraph->setValue(settings.value("MaximumUndoRedoNodeGraph").toInt(), 0);
    }
    if(settings.contains("LinearColorPickers")){
        _linearPickers->setValue(settings.value("LinearColorPickers").toBool(),0);
    }
    if (settings.contains("Number of threads")) {
        _numberOfThreads->setValue(settings.value("Number of threads").toInt(),0);
    }
    if (settings.contains("RenderInSeparateProcess")) {
        _renderInSeparateProcess->setValue(settings.value("RenderInSeparateProcess").toBool(),0);
    }
    if (settings.contains("AutoPreviewDefault")) {
        _autoPreviewEnabledForNewProjects->setValue(settings.value("AutoPreviewDefault").toBool(),0);
    }
    if (settings.contains("SnapToNode")) {
        _snapNodesToConnections->setValue(settings.value("SnapToNode").toBool(), 0);
    }
    if (settings.contains("MaxPanelsOpened")) {
        _maxPanelsOpened->setValue(settings.value("MaxPanelsOpened").toInt(), 0);
    }
    if (settings.contains("ExtraPluginsPaths")) {
        _extraPluginPaths->setValue(settings.value("ExtraPluginsPaths").toString().toStdString(),0);
    }
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    if (settings.contains("OCIOConfigFile")) {
        _customOcioConfigFile->setValue(settings.value("OCIOConfigFile").toString().toStdString(),0);
    }
    if (settings.contains("OCIOConfig")) {
        int activeIndex = settings.value("OCIOConfig").toInt();
        if (activeIndex < (int)_ocioConfigKnob->getEntries().size()) {
            _ocioConfigKnob->setValue(activeIndex,0);
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Caching");
    if(settings.contains("MaximumRAMUsagePercentage")){
        _maxRAMPercent->setValue(settings.value("MaximumRAMUsagePercentage").toInt(),0);
    }
    if(settings.contains("MaximumPlaybackRAMUsage")){
        _maxPlayBackPercent->setValue(settings.value("MaximumPlaybackRAMUsage").toInt(),0);
    }
    if(settings.contains("MaximumDiskSizeUsage")){
        _maxDiskCacheGB->setValue(settings.value("MaximumDiskSizeUsage").toInt(),0);
    }
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    if(settings.contains("ByteTextures")){
        _texturesMode->setValue(settings.value("ByteTextures").toInt(),0);
    }
    if (settings.contains("TilesPowerOf2")) {
        _powerOf2Tiling->setValue(settings.value("TilesPowerOf2").toInt(),0);
    }
    settings.endGroup();
    
    settings.beginGroup("Readers");
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        QString format = _readersMapping[i]->getDescription().c_str();
        if(settings.contains(format)){
            int index = settings.value(format).toInt();
            if (index < (int)_readersMapping[i]->getEntries().size()) {
                _readersMapping[i]->setValue(index,0);
            }
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Writers");
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        QString format = _writersMapping[i]->getDescription().c_str();
        if(settings.contains(format)){
            int index = settings.value(format).toInt();
            if (index < (int)_writersMapping[i]->getEntries().size()) {
                _writersMapping[i]->setValue(index,0);
            }
        }
    }
    settings.endGroup();
    notifyProjectEndKnobsValuesChanged();

}

bool Settings::tryLoadOpenColorIOConfig()
{
    QString configFile;
    if (_customOcioConfigFile->isEnabled(0)) {
        ///try to load from the file
        std::string file = _customOcioConfigFile->getValue();
        if (file.empty()) {
            return false;
        }
        if (!QFile::exists(file.c_str())) {
            Natron::errorDialog("OpenColorIO", file + ": No such file.");
            return false;
        }
        configFile = file.c_str();
    } else {
        ///try to load from the combobox
        QString activeEntryText(_ocioConfigKnob->getActiveEntryText().c_str());
        QString configFileName = QString(activeEntryText + ".ocio");
        QString defaultConfigsPath = getDefaultOcioConfigPath();
        QDir defaultConfigsDir(defaultConfigsPath);
        if (!defaultConfigsDir.exists()) {
            qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory does not exist.";
            return false;
        }
        ///try to open the .ocio config file first in the defaultConfigsDir
        ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
        if (!defaultConfigsDir.exists(configFileName)) {
            QDir subDir(defaultConfigsPath + QDir::separator() + activeEntryText);
            if (!subDir.exists()) {
                Natron::errorDialog("OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + ": No such file or directory.");
                return false;
            }
            if (!subDir.exists("config.ocio")) {
                Natron::errorDialog("OpenColorIO",subDir.absoluteFilePath("config.ocio").toStdString() + ": No such file or directory.");
                return false;
            }
            configFile = subDir.absoluteFilePath("config.ocio");
        } else {
            configFile = defaultConfigsDir.absoluteFilePath(configFileName);
        }
    }
    qDebug() << "setting OCIO=" << configFile;
    qputenv("OCIO", configFile.toUtf8());
    return true;
}

void Settings::onKnobValueChanged(KnobI* k,Natron::ValueChangedReason /*reason*/){

    _wereChangesMadeSinceLastSave = true;

    
    if (k == _texturesMode.get()) {
        std::map<int,AppInstanceRef> apps = appPTR->getAppInstances();
        bool isFirstViewer = true;
        for(std::map<int,AppInstanceRef>::iterator it = apps.begin();it!=apps.end();++it){
            const std::vector<boost::shared_ptr<Node> > nodes = it->second.app->getProject()->getCurrentNodes();
            for (U32 i = 0; i < nodes.size(); ++i) {
                assert(nodes[i]);
                if (nodes[i]->pluginID() == "Viewer") {
                    ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
                    assert(n);
                    if(isFirstViewer){
                        if(!n->supportsGLSL() && _texturesMode->getValue() != 0){
                            Natron::errorDialog("Viewer", "You need OpenGL GLSL in order to use 32 bit fp texutres.\n"
                                                "Reverting to 8bits textures.");
                            _texturesMode->setValue(0,0);
                            return;
                        }
                    }
                    n->updateTreeAndRender();
                }
            }
        }
    } else if(k == _maxDiskCacheGB.get()) {
        appPTR->setApplicationsCachesMaximumDiskSpace(getMaximumDiskCacheSize());
    } else if(k == _maxRAMPercent.get()) {
        appPTR->setApplicationsCachesMaximumMemoryPercent(getRamMaximumPercent());
    } else if(k == _maxPlayBackPercent.get()) {
        appPTR->setPlaybackCacheMaximumSize(getRamPlaybackMaximumPercent());
    } else if(k == _numberOfThreads.get()) {
        int nbThreads = getNumberOfThreads();
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if(k == _ocioConfigKnob.get()) {
        if (_ocioConfigKnob->getActiveEntryText() == std::string(NATRON_CUSTOM_OCIO_CONFIG_NAME)) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
         
    } else if (k == _customOcioConfigFile.get()) {
        tryLoadOpenColorIOConfig();
    } else if (k == _maxUndoRedoNodeGraph.get()) {
        appPTR->setUndoRedoStackLimit(_maxUndoRedoNodeGraph->getValue());
    } else if (k == _maxPanelsOpened.get()) {
        appPTR->onMaxPanelsOpenedChanged(_maxPanelsOpened->getValue());
    }
}

int Settings::getViewersBitDepth() const {
    return _texturesMode->getValue();
}

int Settings::getViewerTilesPowerOf2() const {
    return _powerOf2Tiling->getValue();
}

double Settings::getRamMaximumPercent() const {
    return (double)_maxRAMPercent->getValue() / 100.;
}

double Settings::getRamPlaybackMaximumPercent() const{
    return (double)_maxPlayBackPercent->getValue() / 100.;
}

U64 Settings::getMaximumDiskCacheSize() const {
    return ((U64)(_maxDiskCacheGB->getValue()) * std::pow(1024.,3.));
}
bool Settings::getColorPickerLinear() const {
    return _linearPickers->getValue();
}

int Settings::getNumberOfThreads() const {
    return _numberOfThreads->getValue();
}

void Settings::setNumberOfThreads(int threadsNb) {
    _numberOfThreads->setValue(threadsNb,0);
}

bool Settings::isAutoPreviewOnForNewProjects() const {
    return _autoPreviewEnabledForNewProjects->getValue();
}

const std::string& Settings::getReaderPluginIDForFileType(const std::string& extension){
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        if (_readersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>& entries =  _readersMapping[i]->getEntries();
            int index = _readersMapping[i]->getValue();
            assert(index < (int)entries.size());
            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
 }

const std::string& Settings::getWriterPluginIDForFileType(const std::string& extension){
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        if (_writersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>& entries =  _writersMapping[i]->getEntries();
            int index = _writersMapping[i]->getValue();
            assert(index < (int)entries.size());
            return entries[index];
        }
    }
    throw std::invalid_argument("Unsupported file extension");
}

void Settings::populateReaderPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows){
    
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote ReadOIIO !
            if (QString(it->second[i].c_str()).contains("ReadOIIOOFX")) {
                k->setValue(i,0);
                break;
            }
        }
        _readersMapping.push_back(k);
        _readersTab->addKnob(k);
    }
}

void Settings::populateWriterPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows){
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->setAnimationEnabled(false);
        k->populateChoices(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote WriteOIIOOFX !
            if (QString(it->second[i].c_str()).contains("WriteOIIOOFX")) {
                k->setValue(i,0);
                break;
            }
        }
        _writersMapping.push_back(k);
        _writersTab->addKnob(k);
    }
}

void Settings::getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats){
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _readersMapping[i]->getEntries();
        int index = _readersMapping[i]->getValue();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_readersMapping[i]->getDescription(),entries[index]));
    }
}

void Settings::getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats){
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        int index = _writersMapping[i]->getValue();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_writersMapping[i]->getDescription(),entries[index]));
    }
}

QStringList Settings::getPluginsExtraSearchPaths() const {
    QString paths = _extraPluginPaths->getValue().c_str();
    return paths.split(QChar(';'));
}

void Settings::restoreDefault() {
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    if (!QFile::remove(settings.fileName())) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }
    
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension();++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    endKnobsValuesChanged(Natron::PLUGIN_EDITED);
}

bool Settings::isRenderInSeparatedProcessEnabled() const {
    return _renderInSeparateProcess->getValue();
}

int Settings::getMaximumUndoRedoNodeGraph() const
{
    return _maxUndoRedoNodeGraph->getValue();
}

int Settings::getAutoSaveDelayMS() const
{
    return _autoSaveDelay->getValue() * 1000;
}

bool Settings::isSnapToNodeEnabled() const
{
    return _snapNodesToConnections->getValue();
}


bool Settings::isCheckForUpdatesEnabled() const
{
    return _checkForUpdates->getValue();
}

void Settings::setCheckUpdatesEnabled(bool enabled)
{
    _checkForUpdates->setValue(enabled, 0);
    saveSettings();
}

int Settings::getMaxPanelsOpened() const
{
    return _maxPanelsOpened->getValue();
}

void Settings::setMaxPanelsOpened(int maxPanels)
{
    _maxPanelsOpened->setValue(maxPanels, 0);
}