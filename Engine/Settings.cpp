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
    
    _viewersTab = Natron::createKnob<Tab_Knob>(this, "Viewers");
    
    _texturesMode = Natron::createKnob<Choice_Knob>(this, "Textures bit depth");
    _texturesMode->turnOffAnimation();
    std::vector<std::string> textureModes;
    std::vector<std::string> helpStringsTextureModes;
    textureModes.push_back("Byte");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the software. Cached textures will be smaller in  the viewer cache.");
    textureModes.push_back("16bits half-float");
    helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back("32bits fp");
    helpStringsTextureModes.push_back("Viewer's post-process like color-space conversion will be done\n"
                                      "by the hardware using GLSL. Cached textures will be larger in the viewer cache.");
    _texturesMode->populate(textureModes,helpStringsTextureModes);
    _texturesMode->setValue<int>(0);
    _texturesMode->setHintToolTip("Bitdepth of the viewer textures used for rendering."
                                  " Hover each option with the mouse for a more detailed comprehension.");
    _viewersTab->addKnob(_texturesMode);
    
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
    
    
    
}

Settings::ReadersSettings::ReadersSettings(){
    
}

/*changes the decoder for files identified by the filetype*/
void Settings::ReadersSettings::changeMapping(const std::string& filetype, LibraryBinary* decoder){
    _fileTypesMap.insert(make_pair(filetype, decoder));
}

/*use to initialise default mapping*/
void Settings::ReadersSettings::fillMap(const std::map<std::string,LibraryBinary*>& defaultMap){
    for(std::map<std::string,LibraryBinary*>::const_iterator it = defaultMap.begin();it!=defaultMap.end();++it) {
        _fileTypesMap.insert(*it);
    }
}

LibraryBinary* Settings::ReadersSettings::decoderForFiletype(const std::string& type) const {
    for(std::map<std::string,LibraryBinary*>::const_iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it){
        QString sType(type.c_str());
        QString curType(it->first.c_str());
        sType = sType.toUpper();
        curType = curType.toUpper();
        if(sType == curType){
            return it->second;
        }
    }
    return NULL;
}

Settings::WritersSettings::WritersSettings():_maximumBufferSize(2){}

/*Returns a library if it could find an encoder for the filetype,
 otherwise returns NULL.*/
LibraryBinary* Settings::WritersSettings::encoderForFiletype(const std::string& type) const{
    for(std::map<std::string,LibraryBinary*>::const_iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it){
        QString sType(type.c_str());
        QString curType(it->first.c_str());
        sType = sType.toUpper();
        curType = curType.toUpper();
        if(sType == curType){
            return it->second;
        }
    }
    return NULL;
}

/*changes the encoder for files identified by the filetype*/
void Settings::WritersSettings::changeMapping(const std::string& filetype,LibraryBinary* encoder){
    _fileTypesMap.insert(make_pair(filetype, encoder));
}

/*use to initialise default mapping*/
void Settings::WritersSettings::fillMap(const std::map<std::string,LibraryBinary*>& defaultMap) {
    for(std::map<std::string,LibraryBinary*>::const_iterator it = defaultMap.begin();it!=defaultMap.end();++it) {
        _fileTypesMap.insert(*it);
    }
}

std::vector<std::string> Settings::ReadersSettings::supportedFileTypes() const {
    std::vector<std::string> out;
    for(std::map<std::string,LibraryBinary*>::const_iterator it = _fileTypesMap.begin();it!=_fileTypesMap.end();++it) {
        out.push_back(it->first);
    }
    return out;
}

void Settings::saveSettings(){
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    settings.setValue("LinearColorPickers",_linearPickers->getValue<bool>());
    settings.endGroup();
    
    settings.beginGroup("Caching");
    settings.setValue("MaximumRAMUsagePercentage", _maxRAMPercent->getValue<int>());
    settings.setValue("MaximumPlaybackRAMUsage", _maxPlayBackPercent->getValue<int>());
    settings.setValue("MaximumDiskSizeUsage", _maxDiskCacheGB->getValue<int>());
    settings.endGroup();
    
    settings.beginGroup("Viewers");
    settings.setValue("ByteTextures", _texturesMode->getValue<int>());
    settings.endGroup();
    
    //not serialiazing readers/writers settings as they are meaningless for now
}

void Settings::restoreSettings(){
    notifyProjectBeginKnobsValuesChanged(Natron::OTHER_REASON);
    QSettings settings(NATRON_ORGANIZATION_NAME,NATRON_APPLICATION_NAME);
    settings.beginGroup("General");
    if(settings.contains("LinearColorPickers")){
        _linearPickers->setValue<bool>(settings.value("LinearColorPickers").toBool());
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
    settings.endGroup();
    notifyProjectEndKnobsValuesChanged(Natron::OTHER_REASON);

}

void Settings::onKnobValueChanged(Knob* k,Natron::ValueChangedReason /*reason*/){
    if(!appPTR->isInitialized()){
        return;
    }
    if(k == _texturesMode.get()){
        std::map<int,AppInstance*> apps = appPTR->getAppInstances();
        bool isFirstViewer = true;
        for(std::map<int,AppInstance*>::iterator it = apps.begin();it!=apps.end();++it){
            const std::vector<Node*>& nodes = it->second->getProject()->getCurrentNodes();
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
    }else if(k == _maxDiskCacheGB.get()){
        appPTR->setApplicationsCachesMaximumDiskSpace(getMaximumDiskCacheSize());
    }else if(k == _maxRAMPercent.get()){
        appPTR->setApplicationsCachesMaximumMemoryPercent(getRamMaximumPercent());
    }else if(k == _maxPlayBackPercent.get()){
        appPTR->setPlaybackCacheMaximumSize(getRamPlaybackMaximumPercent());
    }
}

int Settings::getViewersBitDepth() const{
    return _texturesMode->getValue<int>();
}

double Settings::getRamMaximumPercent() const{
    return (double)_maxRAMPercent->getValue<int>() / 100.;
}
double Settings::getRamPlaybackMaximumPercent() const{
    return (double)_maxPlayBackPercent->getValue<int>() / 100.;
}

U64 Settings::getMaximumDiskCacheSize() const{
    return ((U64)(_maxDiskCacheGB->getValue<int>()) * std::pow(1024.,3.));
}
bool Settings::getColorPickerLinear() const{
    return _linearPickers->getValue<bool>();
}