//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef NATRON_ENGINE_SETTINGS_H_
#define NATRON_ENGINE_SETTINGS_H_

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"

#define kQSettingsSoftwareMajorVersionSettingName "SoftwareVersionMajor"

/*The current settings in the preferences menu.
   @todo Move this class to QSettings instead*/


namespace Natron {
class LibraryBinary;
class Plugin;
}

class KnobI;
class File_Knob;
class Page_Knob;
class Double_Knob;
class Int_Knob;
class Bool_Knob;
class Group_Knob;
class Choice_Knob;
class Path_Knob;
class Color_Knob;
class String_Knob;
class QSettings;
class Separator_Knob;
class Settings
    : public KnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:

    Settings(AppInstance* appInstance);

    virtual ~Settings()
    {
    }

    virtual void evaluate(KnobI* /*knob*/,
                          bool /*isSignificant*/,
                          Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE FINAL
    {
    }

    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReasonEnum reason,SequenceTime time,
                                    bool originatedFromMainThread) OVERRIDE FINAL;

    Natron::ImageBitDepthEnum getViewersBitDepth() const;

    int getViewerTilesPowerOf2() const;

    double getRamMaximumPercent() const;

    double getRamPlaybackMaximumPercent() const;

    U64 getMaximumViewerDiskCacheSize() const;
    
    U64 getMaximumDiskCacheNodeSize() const;

    double getUnreachableRamPercent() const;

    bool getColorPickerLinear() const;

    int getNumberOfThreads() const;

    void setNumberOfThreads(int threadsNb);
    
    int getNumberOfParallelRenders() const;
    
    void setNumberOfParallelRenders(int nb);
    
    int getNumberOfThreadsPerEffect() const;
    
    bool useGlobalThreadPool() const;
    
    void setUseGlobalThreadPool(bool use) ;

    std::string getReaderPluginIDForFileType(const std::string & extension);
    std::string getWriterPluginIDForFileType(const std::string & extension);

    void populateReaderPluginsAndFormats(const std::map<std::string,std::vector< std::pair<std::string,double> > > & rows);

    void populateWriterPluginsAndFormats(const std::map<std::string,std::vector< std::pair<std::string,double> > > & rows);
    
    void populatePluginsTab(std::vector<Natron::Plugin*>& pluginsToIgnore);
    
    void populateSystemFonts(const QSettings& settings,const std::vector<std::string>& fonts);

    void getFileFormatsForReadingAndReader(std::map<std::string,std::string>* formats);

    void getFileFormatsForWritingAndWriter(std::map<std::string,std::string>* formats);

    ///save the settings to the application's settings
    void saveSettings(const std::vector<KnobI*>& settings,bool doWarnings);
    
    void saveAllSettings();
    
    void saveSetting(KnobI* knob) {
        std::vector<KnobI*> knobs;
        knobs.push_back(knob);
        saveSettings(knobs,false);
    }

    ///restores the settings from disk
    void restoreSettings();
    
    void restoreKnobsFromSettings(const std::vector<boost::shared_ptr<KnobI> >& knobs);
    void restoreKnobsFromSettings(const std::vector<KnobI*>& knobs);

    bool isAutoPreviewOnForNewProjects() const;

    void getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const;

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
    std::string getDefaultLayoutFile() const;

    bool useCursorPositionIncrements() const;

    bool isAutoProjectFormatEnabled() const;
    
    bool isAutoFixRelativeFilePathEnabled() const;
    
    int getCheckerboardTileSize() const;
    void getCheckerboardColor1(double* r,double* g,double* b,double* a) const;
    void getCheckerboardColor2(double* r,double* g,double* b,double* a) const;
    
    bool areRGBPixelComponentsSupported() const;
    
    bool isTransformConcatenationEnabled() const;
    
    bool isMergeAutoConnectingToAInput() const;
    
    /**
     * @brief If the OCIO startup check parameter is set to true, warn the user if the OCIO config is different
     * from the default value held by NATRON_CUSTOM_OCIO_CONFIG_NAME
     * This can only be called once the 1st AppInstance has been loaded otherwise dialogs could not be created.
     **/
    void doOCIOStartupCheckIfNeeded();
    
    /**
     * @brief Returns true if the QSettings existed prior to loading the settings
     **/
    bool didSettingsExistOnStartup() const;
    
    bool isAutoWipeEnabled() const;
    
    /**
     * @brief Return whether the render scale support is set to its default value (0)  or deactivated (1)
     * for the given plug-in.
     * If the plug-in ID is not valid, -1 is returned.
     **/
    int getRenderScaleSupportPreference(const std::string& pluginID) const;
    
    
    bool notifyOnFileChange() const;
    
    bool isAggressiveCachingEnabled() const;
    
    bool isAutoTurboEnabled() const;
    
    void setAutoTurboModeEnabled(bool e);
    
    void setOptionalInputsAutoHidden(bool hidden);
    bool areOptionalInputsAutoHidden() const;
    
    void getPythonGroupsSearchPaths(std::list<std::string>* templates) const;
    void appendPythonGroupsPath(const std::string& path);
    
    std::string getOnProjectCreatedCB();
    std::string getDefaultOnProjectLoadedCB();
    std::string getDefaultOnProjectSaveCB();
    std::string getDefaultOnProjectCloseCB();
    std::string getDefaultOnNodeCreatedCB();
    std::string getDefaultOnNodeDeleteCB();
    
    void setOnProjectCreatedCB(const std::string& func);
    void setOnProjectLoadedCB(const std::string& func);
    
    bool isLoadFromPyPlugsEnabled() const;
    
    bool isAutoDeclaredVariablePrintActivated() const;
    
    bool isPluginIconActivatedOnNodeGraph() const;
    
    void getSunkenColor(double* r,double* g,double* b) const;
    void getBaseColor(double* r,double* g,double* b) const;
    void getRaisedColor(double* r,double* g,double* b) const;
    void getSelectionColor(double* r,double* g,double* b) const;
    void getInterpolatedColor(double* r,double* g,double* b) const;
    void getKeyframeColor(double* r,double* g,double* b) const;
    void getExprColor(double* r,double* g,double* b) const;
    void getTextColor(double* r,double* g,double* b) const;
    void getAltTextColor(double* r,double* g,double* b) const;
    void getTimelinePlayheadColor(double* r,double* g,double* b) const;
    void getTimelineBoundsColor(double* r,double* g,double* b) const;
    void getTimelineBGColor(double* r,double* g,double* b) const;
    void getCachedFrameColor(double* r,double* g,double* b) const;
    void getDiskCachedColor(double* r,double* g,double* b) const;
    void getCurveEditorBGColor(double* r,double* g,double* b) const;
    void getCurveEditorGridColor(double* r,double* g,double* b) const;
    void getCurveEditorScaleColor(double* r,double* g,double* b) const;
    void getDopeSheetEditorBackgroundColor(double* r,double* g,double* b) const;
    void getDopeSheetEditorRootRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getDopeSheetEditorKnobRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getDopeSheetEditorScaleColor(double* r,double* g, double* b) const;
    void getDopeSheetEditorGridColor(double* r,double* g, double* b) const;

    void getPluginIconFrameColor(int *r, int *g, int *b) const;
    int getDopeSheetEditorNodeSeparationWith() const;
    
    bool isAutoProxyEnabled() const;
    unsigned int getAutoProxyMipMapLevel() const;
    
    bool isNaNHandlingEnabled() const;
    
    bool isInViewerProgressReportEnabled() const;
    
Q_SIGNALS:
    
    void settingChanged(KnobI* knob);
    
private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    void warnChangedKnobs(const std::vector<KnobI*>& knobs);
    
    void setCachingLabels();
    void setDefaultValues();

    bool tryLoadOpenColorIOConfig();


    boost::shared_ptr<Page_Knob> _generalTab;
    boost::shared_ptr<Bool_Knob> _natronSettingsExist;
  
    boost::shared_ptr<Bool_Knob> _checkForUpdates;
    boost::shared_ptr<Bool_Knob> _notifyOnFileChange;
    boost::shared_ptr<Int_Knob> _autoSaveDelay;
    boost::shared_ptr<Bool_Knob> _linearPickers;
    boost::shared_ptr<Bool_Knob> _convertNaNValues;
    boost::shared_ptr<Int_Knob> _numberOfThreads;
    boost::shared_ptr<Int_Knob> _numberOfParallelRenders;
    boost::shared_ptr<Bool_Knob> _useThreadPool;
    boost::shared_ptr<Int_Knob> _nThreadsPerEffect;
    boost::shared_ptr<Bool_Knob> _renderInSeparateProcess;
    boost::shared_ptr<Bool_Knob> _autoPreviewEnabledForNewProjects;
    boost::shared_ptr<Bool_Knob> _firstReadSetProjectFormat;
    boost::shared_ptr<Bool_Knob> _fixPathsOnProjectPathChanged;
    boost::shared_ptr<Int_Knob> _maxPanelsOpened;
    boost::shared_ptr<Bool_Knob> _useCursorPositionIncrements;
    boost::shared_ptr<File_Knob> _defaultLayoutFile;
    boost::shared_ptr<Bool_Knob> _renderOnEditingFinished;
    boost::shared_ptr<Bool_Knob> _activateRGBSupport;
    boost::shared_ptr<Bool_Knob> _activateTransformConcatenationSupport;
    boost::shared_ptr<String_Knob> _hostName;
    boost::shared_ptr<Choice_Knob> _ocioConfigKnob;
    boost::shared_ptr<Bool_Knob> _warnOcioConfigKnobChanged;
    boost::shared_ptr<Bool_Knob> _ocioStartupCheck;
    boost::shared_ptr<File_Knob> _customOcioConfigFile;
    boost::shared_ptr<Page_Knob> _cachingTab;

    boost::shared_ptr<Bool_Knob> _aggressiveCaching;
    ///The percentage of the value held by _maxRAMPercent to dedicate to playback cache (viewer cache's in-RAM portion) only
    boost::shared_ptr<Int_Knob> _maxPlayBackPercent;
    boost::shared_ptr<String_Knob> _maxPlaybackLabel;

    ///The percentage of the system total's RAM to dedicate to caching in theory. In practise this is limited
    ///by _unreachableRamPercent that determines how much RAM should be left free for other use on the computer
    boost::shared_ptr<Int_Knob> _maxRAMPercent;
    boost::shared_ptr<String_Knob> _maxRAMLabel;

    ///The percentage of the system total's RAM you want to keep free from cache usage
    ///When the cache grows and reaches a point where it is about to cross that threshold
    ///it starts freeing the LRU entries regardless of the _maxRAMPercent and _maxPlaybackPercent
    ///A reasonable value should be set for it, allowing Natron's caches to always stay in RAM and
    ///avoid being swapped-out on disk. Assuming the user isn't using many applications at the same time,
    ///10% seems a reasonable value.
    boost::shared_ptr<Int_Knob> _unreachableRAMPercent;
    boost::shared_ptr<String_Knob> _unreachableRAMLabel;
    
    ///The total disk space allowed for all Natron's caches
    boost::shared_ptr<Int_Knob> _maxViewerDiskCacheGB;
    boost::shared_ptr<Int_Knob> _maxDiskCacheNodeGB;
    boost::shared_ptr<Path_Knob> _diskCachePath;
    
    boost::shared_ptr<Page_Knob> _viewersTab;
    boost::shared_ptr<Choice_Knob> _texturesMode;
    boost::shared_ptr<Int_Knob> _powerOf2Tiling;
    boost::shared_ptr<Int_Knob> _checkerboardTileSize;
    boost::shared_ptr<Color_Knob> _checkerboardColor1;
    boost::shared_ptr<Color_Knob> _checkerboardColor2;
    boost::shared_ptr<Bool_Knob> _autoWipe;
    boost::shared_ptr<Bool_Knob> _autoProxyWhenScrubbingTimeline;
    boost::shared_ptr<Choice_Knob> _autoProxyLevel;
    boost::shared_ptr<Bool_Knob> _enableProgressReport;
    
    boost::shared_ptr<Page_Knob> _nodegraphTab;
    boost::shared_ptr<Bool_Knob> _autoTurbo;
    boost::shared_ptr<Bool_Knob> _useNodeGraphHints;
    boost::shared_ptr<Bool_Knob> _snapNodesToConnections;
    boost::shared_ptr<Bool_Knob> _useBWIcons;
    boost::shared_ptr<Int_Knob> _maxUndoRedoNodeGraph;
    boost::shared_ptr<Int_Knob> _disconnectedArrowLength;
    boost::shared_ptr<Bool_Knob> _hideOptionalInputsAutomatically;
    boost::shared_ptr<Bool_Knob> _useInputAForMergeAutoConnect;
    boost::shared_ptr<Bool_Knob> _usePluginIconsInNodeGraph;
    boost::shared_ptr<Color_Knob> _defaultNodeColor;
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
    
    boost::shared_ptr<Path_Knob> _extraPluginPaths;
    boost::shared_ptr<Path_Knob> _templatesPluginPaths;
    boost::shared_ptr<Bool_Knob> _preferBundledPlugins;
    boost::shared_ptr<Bool_Knob> _loadBundledPlugins;
    boost::shared_ptr<Page_Knob> _pluginsTab;
    
    boost::shared_ptr<Page_Knob> _pythonPage;
    boost::shared_ptr<String_Knob> _onProjectCreated;
    boost::shared_ptr<String_Knob> _defaultOnProjectLoaded;
    boost::shared_ptr<String_Knob> _defaultOnProjectSave;
    boost::shared_ptr<String_Knob> _defaultOnProjectClose;
    boost::shared_ptr<String_Knob> _defaultOnNodeCreated;
    boost::shared_ptr<String_Knob> _defaultOnNodeDelete;
    boost::shared_ptr<Bool_Knob> _loadPyPlugsFromPythonScript;
    
    boost::shared_ptr<Bool_Knob> _echoVariableDeclarationToPython;
    boost::shared_ptr<Page_Knob> _appearanceTab;
    
    boost::shared_ptr<Choice_Knob> _systemFontChoice;
    boost::shared_ptr<Int_Knob> _fontSize;
    
    boost::shared_ptr<Group_Knob> _guiColors;
    boost::shared_ptr<Color_Knob> _sunkenColor;
    boost::shared_ptr<Color_Knob> _baseColor;
    boost::shared_ptr<Color_Knob> _raisedColor;
    boost::shared_ptr<Color_Knob> _selectionColor;
    boost::shared_ptr<Color_Knob> _textColor;
    boost::shared_ptr<Color_Knob> _altTextColor;
    boost::shared_ptr<Color_Knob> _timelinePlayheadColor;
    boost::shared_ptr<Color_Knob> _timelineBGColor;
    boost::shared_ptr<Color_Knob> _timelineBoundsColor;
    boost::shared_ptr<Color_Knob> _interpolatedColor;
    boost::shared_ptr<Color_Knob> _keyframeColor;
    boost::shared_ptr<Color_Knob> _exprColor;
    boost::shared_ptr<Color_Knob> _cachedFrameColor;
    boost::shared_ptr<Color_Knob> _diskCachedFrameColor;
    
    boost::shared_ptr<Group_Knob> _curveEditorColors;
    boost::shared_ptr<Color_Knob> _curveEditorBGColor;
    boost::shared_ptr<Color_Knob> _gridColor;
    boost::shared_ptr<Color_Knob> _curveEditorScaleColor;

    boost::shared_ptr<Group_Knob> _dopeSheetEditorColors;
    boost::shared_ptr<Color_Knob> _dopeSheetEditorBackgroundColor;
    boost::shared_ptr<Color_Knob> _dopeSheetEditorRootSectionBackgroundColor;
    boost::shared_ptr<Color_Knob> _dopeSheetEditorKnobSectionBackgroundColor;
    boost::shared_ptr<Color_Knob> _dopeSheetEditorScaleColor;
    boost::shared_ptr<Color_Knob> _dopeSheetEditorGridColor;
    
    std::map<std::string,boost::shared_ptr<Choice_Knob> > _perPluginRenderScaleSupport;
    bool _restoringSettings;
    bool _ocioRestored;
    bool _settingsExisted;
};

#endif // NATRON_ENGINE_SETTINGS_H_
