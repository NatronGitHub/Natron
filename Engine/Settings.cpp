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

#include "Global/AppManager.h"
#include "Global/MemoryInfo.h"
#include "Global/LibraryBinary.h"

#include "Engine/KnobTypes.h"
#include "Engine/KnobFactory.h"
#include "Engine/Project.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"

using namespace Natron;


Settings::Settings(AppInstance* appInstance)
: KnobHolder(appInstance)
, _wereChangesMadeSinceLastSave(false)
{
    
}

void Settings::initializeKnobs(){
    _generalTab = Natron::createKnob<Tab_Knob>(this, "General");
    
    _linearPickers = Natron::createKnob<Bool_Knob>(this, "Linear color pickers");
    _linearPickers->turnOffAnimation();
    _linearPickers->setValue<bool>(true);
    _linearPickers->setHintToolTip("When activated, all colors picked from the color parameters will be converted"
                                   " to linear before being fetched. Otherwise they will be in the same color-space "
                                   " as the viewer they were picked from.");
    _generalTab->addKnob(_linearPickers);
    
    _multiThreadedDisabled = Natron::createKnob<Bool_Knob>(this, "Disable multi-threading");
    _multiThreadedDisabled->turnOffAnimation();
    _multiThreadedDisabled->setValue<bool>(false);
    _multiThreadedDisabled->setHintToolTip("If true, " NATRON_APPLICATION_NAME " will not spawn any thread to render.");
    _generalTab->addKnob(_multiThreadedDisabled);
    
    _viewersTab = Natron::createKnob<Tab_Knob>(this, "Viewers");
    
    _texturesMode = Natron::createKnob<Choice_Knob>(this, "Viewer textures bit depth");
    _texturesMode->turnOffAnimation();
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
    _texturesMode->setValue<int>(0);
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
    
    _powerOf2Tiling->setValue<int>(8);
    _powerOf2Tiling->turnOffAnimation();
    _viewersTab->addKnob(_powerOf2Tiling);
    
    _cachingTab = Natron::createKnob<Tab_Knob>(this, "Caching");
    
    _maxRAMPercent = Natron::createKnob<Int_Knob>(this, "Maximum system's RAM for caching");
    _maxRAMPercent->turnOffAnimation();
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    _maxRAMPercent->setValue<int>(50);
    std::string ramHint("This setting indicates the percentage of the system's total RAM "
                        NATRON_APPLICATION_NAME "'s caches are allowed to use."
                        " Your system has ");
    ramHint.append(printAsRAM(getSystemTotalRAM()).toStdString());
    ramHint.append(" of RAM.");
    _maxRAMPercent->setHintToolTip(ramHint);
    _cachingTab->addKnob(_maxRAMPercent);
    
    _maxPlayBackPercent = Natron::createKnob<Int_Knob>(this, "Playback cache RAM percentage");
    _maxPlayBackPercent->turnOffAnimation();
    _maxPlayBackPercent->setMinimum(0);
    _maxPlayBackPercent->setMaximum(100);
    _maxPlayBackPercent->setValue(25);
    _maxPlayBackPercent->setHintToolTip("This setting indicates the percentage of the Maximum system's RAM for caching"
                                        " dedicated for the playback cache. Normally you shouldn't change this value"
                                        " as it is tuned automatically by the Maximum system's RAM for caching, but"
                                        " this is made possible for convenience.");
    _cachingTab->addKnob(_maxPlayBackPercent);
    
    _maxDiskCacheGB = Natron::createKnob<Int_Knob>(this, "Maximum disk cache size");
    _maxDiskCacheGB->turnOffAnimation();
    _maxDiskCacheGB->setMinimum(0);
    _maxDiskCacheGB->setValue<int>(10);
    _maxDiskCacheGB->setMaximum(100);
    _maxDiskCacheGB->setHintToolTip("The maximum disk space the caches can use. (in GB)");
    _cachingTab->addKnob(_maxDiskCacheGB);
    
    
    ///readers & writers settings are created in a postponed manner because we don't know
    ///their dimension yet. See populateReaderPluginsAndFormats & populateWriterPluginsAndFormats
    
    _readersTab = Natron::createKnob<Tab_Knob>(this, "Readers");

    _writersTab = Natron::createKnob<Tab_Knob>(this, "Writers");
    
    
    
}



void Settings::saveSettings(){
    
    _wereChangesMadeSinceLastSave = false;
    
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    settings.setValue("LinearColorPickers",_linearPickers->getValue<bool>());
    settings.setValue("MultiThreadingDisabled", _multiThreadedDisabled->getValue<bool>());
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

void Settings::onKnobValueChanged(Knob* k,Natron::ValueChangedReason /*reason*/){
    
    if (!appPTR->isInitialized()) {
        return;
    }
    
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
        k->turnOffAnimation();
        k->populate(it->second);
        _readersMapping.push_back(k);
        _readersTab->addKnob(k);
    }
}

void Settings::populateWriterPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows){
    for (std::map<std::string,std::vector<std::string> >::const_iterator it = rows.begin(); it!=rows.end(); ++it) {
        boost::shared_ptr<Choice_Knob> k = Natron::createKnob<Choice_Knob>(this, it->first);
        k->turnOffAnimation();
        k->populate(it->second);
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

