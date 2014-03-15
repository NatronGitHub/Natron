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

#include <QDir>
#include <QSettings>
#include <QThreadPool>

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
    _generalTab = Natron::createKnob<Tab_Knob>(this, "General");
    
    _linearPickers = Natron::createKnob<Bool_Knob>(this, "Linear color pickers");
    _linearPickers->setAnimationEnabled(false);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters will be converted"
                                   " to linear before being fetched. Otherwise they will be in the same color-space "
                                   " as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);
    
    _multiThreadedDisabled = Natron::createKnob<Bool_Knob>(this, "Disable multi-threading");
    _multiThreadedDisabled->setAnimationEnabled(false);
    _multiThreadedDisabled->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will not spawn any thread to render.");
    _generalTab->addKnob(_multiThreadedDisabled);
    
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
    
    boost::shared_ptr<Tab_Knob> ocioTab = Natron::createKnob<Tab_Knob>(this, "OpenColorIO");
    
    
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
    _ocioConfigKnob->populate(configs);
    _ocioConfigKnob->setValue<int>(defaultIndex);
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
    
    _viewersTab = Natron::createKnob<Tab_Knob>(this, "Viewers");
    
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
    _texturesMode->populate(textureModes,helpStringsTextureModes);
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
    
    _cachingTab = Natron::createKnob<Tab_Knob>(this, "Caching");
    
    _maxRAMPercent = Natron::createKnob<Int_Knob>(this, "Maximum system's RAM for caching");
    _maxRAMPercent->setAnimationEnabled(false);
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    std::string ramHint("This setting indicates the percentage of the system's total RAM "
                        NATRON_APPLICATION_NAME "'s caches are allowed to use."
                        " Your system has ");
    ramHint.append(printAsRAM(getSystemTotalRAM()).toStdString());
    ramHint.append(" of RAM.");
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
    
    _readersTab = Natron::createKnob<Tab_Knob>(this, "Readers");

    _writersTab = Natron::createKnob<Tab_Knob>(this, "Writers");
    
    
    setDefaultValues();
}


void Settings::setDefaultValues() {
    
    beginKnobsValuesChanged(Natron::PLUGIN_EDITED);
    _linearPickers->setDefaultValue<bool>(true);
    _multiThreadedDisabled->setDefaultValue<bool>(false);
    _renderInSeparateProcess->setDefaultValue<bool>(true);
    _autoPreviewEnabledForNewProjects->setDefaultValue<bool>(true);
    _extraPluginPaths->setDefaultValue<QString>("");
    _texturesMode->setDefaultValue<int>(0);
    _powerOf2Tiling->setDefaultValue<int>(8);
    _maxRAMPercent->setDefaultValue<int>(50);
    _maxPlayBackPercent->setDefaultValue(25);
    _maxDiskCacheGB->setDefaultValue<int>(10);

    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _readersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("ReadOIIOOFX")) {
                _readersMapping[i]->setDefaultValue<int>(j);
                break;
            }
        }
    }
    
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        for (U32 j = 0; j < entries.size(); ++j) {
            if (QString(entries[j].c_str()).contains("WriteOIIOOFX")) {
                _writersMapping[i]->setDefaultValue<int>(j);
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
    settings.setValue("LinearColorPickers",_linearPickers->getValue<bool>());
    settings.setValue("MultiThreadingDisabled", _multiThreadedDisabled->getValue<bool>());
    settings.setValue("RenderInSeparateProcess", _renderInSeparateProcess->getValue<bool>());
    settings.setValue("AutoPreviewDefault", _autoPreviewEnabledForNewProjects->getValue<bool>());
    settings.setValue("ExtraPluginsPaths", _extraPluginPaths->getValue<QString>());
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    QStringList configList = _customOcioConfigFile->getValue<QStringList>();
    if (!configList.isEmpty()) {
        settings.setValue("OCIOConfigFile", configList.at(0));
    }
    settings.setValue("OCIOConfig", _ocioConfigKnob->getValue<int>());
    settings.endGroup();
    
    settings.beginGroup("Caching");
    settings.setValue("MaximumRAMUsagePercentage", _maxRAMPercent->getValue<int>());
    settings.setValue("MaximumPlaybackRAMUsage", _maxPlayBackPercent->getValue<int>());
    settings.setValue("MaximumDiskSizeUsage", _maxDiskCacheGB->getValue<int>());
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    settings.setValue("ByteTextures", _texturesMode->getValue<int>());
    settings.setValue("TilesPowerOf2", _powerOf2Tiling->getValue<int>());
    settings.endGroup();
    
    settings.beginGroup("Readers");
    
    
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        settings.setValue(_readersMapping[i]->getDescription().c_str(),_readersMapping[i]->getValue<int>());
    }
    settings.endGroup();
    
    settings.beginGroup("Writers");
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        settings.setValue(_writersMapping[i]->getDescription().c_str(),_writersMapping[i]->getValue<int>());
    }
    settings.endGroup();
    
}

void Settings::restoreSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    if(settings.contains("LinearColorPickers")){
        _linearPickers->setValue<bool>(settings.value("LinearColorPickers").toBool());
    }
    if (settings.contains("MultiThreadingDisabled")) {
        _multiThreadedDisabled->setValue<bool>(settings.value("MultiThreadingDisabled").toBool());
    }
    if (settings.contains("RenderInSeparateProcess")) {
        _renderInSeparateProcess->setValue<bool>(settings.value("RenderInSeparateProcess").toBool());
    }
    if (settings.contains("AutoPreviewDefault")) {
        _autoPreviewEnabledForNewProjects->setValue<bool>(settings.value("AutoPreviewDefault").toBool());
    }
    if (settings.contains("ExtraPluginsPaths")) {
        _extraPluginPaths->setValue<QString>(settings.value("ExtraPluginsPaths").toString());
    }
    settings.endGroup();
    
    settings.beginGroup("OpenColorIO");
    if (settings.contains("OCIOConfigFile")) {
        _customOcioConfigFile->setValue<QStringList>(QStringList(settings.value("OCIOConfigFile").toString()));
    }
    if (settings.contains("OCIOConfig")) {
        int activeIndex = settings.value("OCIOConfig").toInt();
        if (activeIndex < (int)_ocioConfigKnob->getEntries().size()) {
            _ocioConfigKnob->setValue<int>(activeIndex);
        }
    }
    settings.endGroup();
    
    settings.beginGroup("Caching");
    if(settings.contains("MaximumRAMUsagePercentage")){
        _maxRAMPercent->setValue<int>(settings.value("MaximumRAMUsagePercentage").toInt());
    }
    if(settings.contains("MaximumPlaybackRAMUsage")){
        _maxPlayBackPercent->setValue<int>(settings.value("MaximumRAMUsagePercentage").toInt());
    }
    if(settings.contains("MaximumDiskSizeUsage")){
        _maxDiskCacheGB->setValue<int>(settings.value("MaximumDiskSizeUsage").toInt());
    }
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    if(settings.contains("ByteTextures")){
        _texturesMode->setValue<int>(settings.value("ByteTextures").toFloat());
    }
    if (settings.contains("TilesPowerOf2")) {
        _powerOf2Tiling->setValue<int>(settings.value("TilesPowerOf2").toInt());
    }
    settings.endGroup();
    
    settings.beginGroup("Readers");
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        QString format = _readersMapping[i]->getDescription().c_str();
        if(settings.contains(format)){
            int index = settings.value(format).toInt();
            if (index < (int)_readersMapping[i]->getEntries().size()) {
                _readersMapping[i]->setValue<int>(index);
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
                _writersMapping[i]->setValue<int>(index);   
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
        QStringList files = _customOcioConfigFile->getValue<QStringList>();
        if (files.isEmpty()) {
            return false;
        }
        if (!QFile::exists(files.at(0))) {
            Natron::errorDialog("OpenColorIO", files.at(0).toStdString() + ": No such file.");
            return false;
        }
        configFile = files.at(0);
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

void Settings::onKnobValueChanged(Knob* k,Natron::ValueChangedReason /*reason*/){

    _wereChangesMadeSinceLastSave = true;

    
    if (k == _texturesMode.get()) {
        std::map<int,AppInstance*> apps = appPTR->getAppInstances();
        bool isFirstViewer = true;
        for(std::map<int,AppInstance*>::iterator it = apps.begin();it!=apps.end();++it){
            const std::vector<Node*> nodes = it->second->getProject()->getCurrentNodes();
            for (U32 i = 0; i < nodes.size(); ++i) {
                assert(nodes[i]);
                if (nodes[i]->pluginID() == "Viewer") {
                    ViewerInstance* n = dynamic_cast<ViewerInstance*>(nodes[i]->getLiveInstance());
                    assert(n);
                    if(isFirstViewer){
                        if(!n->supportsGLSL() && _texturesMode->getValue<int>() != 0){
                            Natron::errorDialog("Viewer", "You need OpenGL GLSL in order to use 32 bit fp texutres.\n"
                                                "Reverting to 8bits textures.");
                            _texturesMode->setValue<int>(0);
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
    } else if(k == _multiThreadedDisabled.get()) {
        if (isMultiThreadingDisabled()) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
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
    }
}

int Settings::getViewersBitDepth() const {
    return _texturesMode->getValue<int>();
}

int Settings::getViewerTilesPowerOf2() const {
    return _powerOf2Tiling->getValue<int>();
}

double Settings::getRamMaximumPercent() const {
    return (double)_maxRAMPercent->getValue<int>() / 100.;
}

double Settings::getRamPlaybackMaximumPercent() const{
    return (double)_maxPlayBackPercent->getValue<int>() / 100.;
}

U64 Settings::getMaximumDiskCacheSize() const {
    return ((U64)(_maxDiskCacheGB->getValue<int>()) * std::pow(1024.,3.));
}
bool Settings::getColorPickerLinear() const {
    return _linearPickers->getValue<bool>();
}

bool Settings::isMultiThreadingDisabled() const {
    return _multiThreadedDisabled->getValue<bool>();
}

void Settings::setMultiThreadingDisabled(bool disabled) {
    _multiThreadedDisabled->setValue<bool>(disabled);
}

bool Settings::isAutoPreviewOnForNewProjects() const {
    return _autoPreviewEnabledForNewProjects->getValue<bool>();
}

const std::string& Settings::getReaderPluginIDForFileType(const std::string& extension){
    for (U32 i = 0; i < _readersMapping.size(); ++i) {
        if (_readersMapping[i]->getDescription() == extension) {
            const std::vector<std::string>& entries =  _readersMapping[i]->getEntries();
            int index = _readersMapping[i]->getActiveEntry();
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
            int index = _writersMapping[i]->getActiveEntry();
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
        k->populate(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote ReadOIIO !
            if (QString(it->second[i].c_str()).contains("ReadOIIOOFX")) {
                k->setValue<int>(i);
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
        k->populate(it->second);
        for (U32 i = 0; i < it->second.size(); ++i) {
            ///promote WriteOIIOOFX !
            if (QString(it->second[i].c_str()).contains("WriteOIIOOFX")) {
                k->setValue<int>(i);
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
        int index = _readersMapping[i]->getActiveEntry();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_readersMapping[i]->getDescription(),entries[index]));
    }
}

void Settings::getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats){
    for (U32 i = 0; i < _writersMapping.size(); ++i) {
        const std::vector<std::string>& entries = _writersMapping[i]->getEntries();
        int index = _writersMapping[i]->getActiveEntry();
        
        assert(index < (int)entries.size());
        
        formats->insert(std::make_pair(_writersMapping[i]->getDescription(),entries[index]));
    }
}

QStringList Settings::getPluginsExtraSearchPaths() const {
    QString paths = _extraPluginPaths->getValue<QString>();
    return paths.split(QChar(';'));
}

void Settings::restoreDefault() {
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    if (!QFile::remove(settings.fileName())) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }
    setDefaultValues();
}

bool Settings::isRenderInSeparatedProcessEnabled() const {
    return _renderInSeparateProcess->getValue<bool>();
}