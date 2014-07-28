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

#ifndef NATRON_ENGINE_SETTINGS_H_
#define NATRON_ENGINE_SETTINGS_H_

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"

/*The current settings in the preferences menu.
 @todo Move this class to QSettings instead*/


namespace Natron {
    class LibraryBinary;
}

class File_Knob;
class Page_Knob;
class Double_Knob;
class Int_Knob;
class Bool_Knob;
class Choice_Knob;
class Path_Knob;
class Color_Knob;
class String_Knob;

class Settings : public KnobHolder
{
public:

    Settings(AppInstance* appInstance);
    
    virtual ~Settings(){}
        
    virtual void evaluate(KnobI* /*knob*/,bool /*isSignificant*/,Natron::ValueChangedReason /*reason*/) OVERRIDE FINAL {}
    
    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReason reason,SequenceTime time) OVERRIDE FINAL;
    
    int getViewersBitDepth() const;
    
    int getViewerTilesPowerOf2() const;
    
    double getRamMaximumPercent() const;
    
    double getRamPlaybackMaximumPercent() const;
    
    U64 getMaximumDiskCacheSize() const;
    
    bool getColorPickerLinear() const;
    
    int getNumberOfThreads() const;
    
    void setNumberOfThreads(int threadsNb);
    
    const std::string& getReaderPluginIDForFileType(const std::string& extension);
    
    const std::string& getWriterPluginIDForFileType(const std::string& extension);
    
    void populateReaderPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows);
    
    void populateWriterPluginsAndFormats(const std::map<std::string,std::vector<std::string> >& rows);
    
    void getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats);
    
    void getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats);
   
    ///save the settings to the application's settings
    void saveSettings();
    
    ///restores the settings from disk
    void restoreSettings();
    
    bool wereChangesMadeSinceLastSave() const { return _wereChangesMadeSinceLastSave; }
    
    void resetWereChangesMadeSinceLastSave() { _wereChangesMadeSinceLastSave = false; }
    
    bool isAutoPreviewOnForNewProjects() const;
    
    QStringList getPluginsExtraSearchPaths() const;
    
    bool isRenderInSeparatedProcessEnabled() const;
    
    void restoreDefault();
    
    int getMaximumUndoRedoNodeGraph() const;
    
    int getAutoSaveDelayMS() const;
    
    bool isSnapToNodeEnabled() const;
    
    bool isCheckForUpdatesEnabled() const;
    
    void setCheckUpdatesEnabled(bool enabled);
    
    int getMaxPanelsOpened() const;
    
    void setMaxPanelsOpened(int maxPanels);
    
    void setConnectionHintsEnabled(bool enabled);
    
    bool isConnectionHintEnabled() const;
    
    bool loadBundledPlugins() const;
    
    bool preferBundledPlugins() const;
    
    void getDefaultNodeColor(float *r,float *g,float *b) const;
    
    void getDefaultSelectedNodeColor(float *r,float *g,float *b) const;
    
    void getDefaultBackDropColor(float *r,float *g,float *b) const;
    
    int getDisconnectedArrowLength() const;
    
    void getGeneratorColor(float *r,float *g,float *b) const;
    void getReaderColor(float *r,float *g,float *b) const;
    void getWriterColor(float *r,float *g,float *b) const;
    void getColorGroupColor(float *r,float *g,float *b) const;
    void getFilterGroupColor(float *r,float *g,float *b) const;
    void getTransformGroupColor(float *r,float *g,float *b) const;
    void getTimeGroupColor(float *r,float *g,float *b) const;
    void getDrawGroupColor(float *r,float *g,float *b) const;
    void getKeyerGroupColor(float *r,float *g,float *b) const;
    void getChannelGroupColor(float *r,float *g,float *b) const;
    void getMergeGroupColor(float *r,float *g,float *b) const;
    void getViewsGroupColor(float *r,float *g,float *b) const;
    void getDeepGroupColor(float *r,float *g,float *b) const;
    
    bool getRenderOnEditingFinishedOnly() const;
    void setRenderOnEditingFinishedOnly(bool render);
    
    bool getIconsBlackAndWhite() const;
    
    std::string getHostName() const;
private:
    
    virtual void initializeKnobs() OVERRIDE FINAL;

    void setCachingLabels();
    void setDefaultValues();
        
    bool tryLoadOpenColorIOConfig();

    
    boost::shared_ptr<Page_Knob> _generalTab;
    boost::shared_ptr<Bool_Knob> _checkForUpdates;
    boost::shared_ptr<Int_Knob> _autoSaveDelay;
    boost::shared_ptr<Bool_Knob> _linearPickers;
    boost::shared_ptr<Int_Knob> _numberOfThreads;
    boost::shared_ptr<Bool_Knob> _renderInSeparateProcess;
    boost::shared_ptr<Bool_Knob> _autoPreviewEnabledForNewProjects;
    boost::shared_ptr<Int_Knob> _maxPanelsOpened;
    boost::shared_ptr<Bool_Knob> _renderOnEditingFinished;
    boost::shared_ptr<Path_Knob> _extraPluginPaths;
    boost::shared_ptr<Bool_Knob> _preferBundledPlugins;
    boost::shared_ptr<Bool_Knob> _loadBundledPlugins;
    boost::shared_ptr<String_Knob> _hostName;
    
    boost::shared_ptr<Choice_Knob> _ocioConfigKnob;
    boost::shared_ptr<File_Knob> _customOcioConfigFile;
    
    boost::shared_ptr<Page_Knob> _cachingTab;
    boost::shared_ptr<Int_Knob> _maxPlayBackPercent;
    boost::shared_ptr<String_Knob> _maxPlaybackLabel;
    
    boost::shared_ptr<Int_Knob> _maxRAMPercent;
    boost::shared_ptr<String_Knob> _maxRAMLabel;
    
    boost::shared_ptr<Int_Knob> _maxDiskCacheGB;
    
    boost::shared_ptr<Page_Knob> _viewersTab;
    boost::shared_ptr<Choice_Knob> _texturesMode;
    boost::shared_ptr<Int_Knob> _powerOf2Tiling;
    
    boost::shared_ptr<Page_Knob> _nodegraphTab;
    boost::shared_ptr<Bool_Knob> _useNodeGraphHints;
    boost::shared_ptr<Bool_Knob> _snapNodesToConnections;
    boost::shared_ptr<Bool_Knob> _useBWIcons;
    boost::shared_ptr<Int_Knob> _maxUndoRedoNodeGraph;
    boost::shared_ptr<Int_Knob> _disconnectedArrowLength;
    boost::shared_ptr<Color_Knob> _defaultNodeColor;
    boost::shared_ptr<Color_Knob> _defaultSelectedNodeColor;
    boost::shared_ptr<Color_Knob> _defaultBackdropColor;
    
    boost::shared_ptr<Color_Knob> _defaultGeneratorColor;
    boost::shared_ptr<Color_Knob> _defaultReaderColor;
    boost::shared_ptr<Color_Knob> _defaultWriterColor;
    boost::shared_ptr<Color_Knob> _defaultColorGroupColor;
    boost::shared_ptr<Color_Knob> _defaultFilterGroupColor;
    boost::shared_ptr<Color_Knob> _defaultTransformGroupColor;
    boost::shared_ptr<Color_Knob> _defaultTimeGroupColor;
    boost::shared_ptr<Color_Knob> _defaultDrawGroupColor;
    boost::shared_ptr<Color_Knob> _defaultKeyerGroupColor;
    boost::shared_ptr<Color_Knob> _defaultChannelGroupColor;
    boost::shared_ptr<Color_Knob> _defaultMergeGroupColor;
    boost::shared_ptr<Color_Knob> _defaultViewsGroupColor;
    boost::shared_ptr<Color_Knob> _defaultDeepGroupColor;

    
    boost::shared_ptr<Page_Knob> _readersTab;
    std::vector< boost::shared_ptr<Choice_Knob> > _readersMapping;
    
    boost::shared_ptr<Page_Knob> _writersTab;
    std::vector< boost::shared_ptr<Choice_Knob> >  _writersMapping;
    
    bool _wereChangesMadeSinceLastSave;
    bool _restoringSettings;
    
};

#endif // NATRON_ENGINE_SETTINGS_H_
