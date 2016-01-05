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

#ifndef NATRON_ENGINE_SETTINGS_H
#define NATRON_ENGINE_SETTINGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <string>
#include <map>
#include <vector>

#include "Global/Macros.h"
#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/EngineFwd.h"


#define kQSettingsSoftwareMajorVersionSettingName "SoftwareVersionMajor"

/*The current settings in the preferences menu.
   @todo Move this class to QSettings instead*/


class Settings
    : public KnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON
    
public:
    enum KnownHostNameEnum {
        eKnownHostNameNatron = 0,
        eKnownHostNameNuke,
        eKnownHostNameFusion,
        eKnownHostNameVegas,
        eKnownHostNameToxik,
        eKnownHostNameScratch,
        eKnownHostNameDustBuster,
        eKnownHostNameResolve,
        eKnownHostNameResolveLite,
        eKnownHostNameMistika,
        eKnownHostNamePablo,
        eKnownHostNameMotionStudio,
        eKnownHostNameShake,
        eKnownHostNameBaselight,
        eKnownHostNameFrameCycler,
        eKnownHostNameNucoda,
        eKnownHostNameAvidDS,
        eKnownHostNameDX,
        eKnownHostNameTitlerPro,
        eKnownHostNameRamen,
        eKnownHostNameTuttleOfx,
        eKnownHostNameNone,
    };

    Settings(AppInstance* appInstance);

    virtual ~Settings()
    {
    }

    virtual void evaluate(KnobI* /*knob*/,
                          bool /*isSignificant*/,
                          Natron::ValueChangedReasonEnum /*reason*/) OVERRIDE FINAL
    {
    }

    virtual void onKnobValueChanged(KnobI* k,Natron::ValueChangedReasonEnum reason,double time,
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
    
    void populatePluginsTab();
    
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
    const std::string& getKnownHostName(KnownHostNameEnum e) const {
        return _knownHostNames[(int)e];
    }
    std::string getDefaultLayoutFile() const;
    
    bool getLoadProjectWorkspce() const;

    bool useCursorPositionIncrements() const;

    bool isAutoProjectFormatEnabled() const;
    
    bool isAutoFixRelativeFilePathEnabled() const;
    
    bool isAutoWipeEnabled() const;
    
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
    
    /**
     * @brief Return whether the render scale support is set to its default value (0)  or deactivated (1)
     * for the given plug-in.
     * If the plug-in ID is not valid, -1 is returned.
     **/
    int getRenderScaleSupportPreference(const Natron::Plugin* p) const;
    
    
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
    
    void setAutoDeclaredVariablePrintEnabled(bool enabled);
    
    bool isPluginIconActivatedOnNodeGraph() const;
    
    bool isNodeGraphAntiAliasingEnabled() const;
    
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

    
    void getSEKeywordColor(double* r,double* g, double* b) const;
    void getSEOperatorColor(double* r,double* g, double* b) const;
    void getSEBraceColor(double* r,double* g, double* b) const;
    void getSEDefClassColor(double* r,double* g, double* b) const;
    void getSEStringsColor(double* r,double* g, double* b) const;
    void getSECommentsColor(double* r,double* g, double* b) const;
    void getSESelfColor(double* r,double* g, double* b) const;
    void getSENumbersColor(double* r,double* g, double* b) const;
    void getSECurLineColor(double* r,double* g, double* b) const;
    
    
    void getPluginIconFrameColor(int *r, int *g, int *b) const;
    int getDopeSheetEditorNodeSeparationWith() const;
    
    bool isAutoProxyEnabled() const;
    unsigned int getAutoProxyMipMapLevel() const;
    
    bool isNaNHandlingEnabled() const;
    
    bool isInViewerProgressReportEnabled() const;
    
    bool isDefaultAppearanceOutdated() const;
    void restoreDefaultAppearance();
    
    std::string getUserStyleSheetFilePath() const;
    
    bool isPluginDeactivated(const Natron::Plugin* p) const;
    
Q_SIGNALS:
    
    void settingChanged(KnobI* knob);
    
private:

    virtual void initializeKnobs() OVERRIDE FINAL;
    
    void initializeKnobsGeneral();
    void initializeKnobsAppearance();
    void initializeKnobsViewers();
    void initializeKnobsNodeGraph();
    void initializeKnobsCaching();
    void initializeKnobsReaders();
    void initializeKnobsWriters();
    void initializeKnobsPlugins();
    void initializeKnobsPython();

    void warnChangedKnobs(const std::vector<KnobI*>& knobs);
    
    void setCachingLabels();
    void setDefaultValues();

    bool tryLoadOpenColorIOConfig();


    boost::shared_ptr<KnobPage> _generalTab;
    boost::shared_ptr<KnobBool> _natronSettingsExist;
  
    boost::shared_ptr<KnobBool> _checkForUpdates;
    boost::shared_ptr<KnobBool> _notifyOnFileChange;
    boost::shared_ptr<KnobInt> _autoSaveDelay;
    boost::shared_ptr<KnobBool> _linearPickers;
    boost::shared_ptr<KnobBool> _convertNaNValues;
    boost::shared_ptr<KnobInt> _numberOfThreads;
    boost::shared_ptr<KnobInt> _numberOfParallelRenders;
    boost::shared_ptr<KnobBool> _useThreadPool;
    boost::shared_ptr<KnobInt> _nThreadsPerEffect;
    boost::shared_ptr<KnobBool> _renderInSeparateProcess;
    boost::shared_ptr<KnobBool> _autoPreviewEnabledForNewProjects;
    boost::shared_ptr<KnobBool> _firstReadSetProjectFormat;
    boost::shared_ptr<KnobBool> _fixPathsOnProjectPathChanged;
    boost::shared_ptr<KnobInt> _maxPanelsOpened;
    boost::shared_ptr<KnobBool> _useCursorPositionIncrements;
    boost::shared_ptr<KnobFile> _defaultLayoutFile;
    boost::shared_ptr<KnobBool> _loadProjectsWorkspace;
    boost::shared_ptr<KnobBool> _renderOnEditingFinished;
    boost::shared_ptr<KnobBool> _activateRGBSupport;
    boost::shared_ptr<KnobBool> _activateTransformConcatenationSupport;
    boost::shared_ptr<KnobChoice> _hostName;
    boost::shared_ptr<KnobString> _customHostName;
    
    
    boost::shared_ptr<KnobChoice> _ocioConfigKnob;
    boost::shared_ptr<KnobBool> _warnOcioConfigKnobChanged;
    boost::shared_ptr<KnobBool> _ocioStartupCheck;
    boost::shared_ptr<KnobFile> _customOcioConfigFile;
    boost::shared_ptr<KnobPage> _cachingTab;

    boost::shared_ptr<KnobBool> _aggressiveCaching;
    ///The percentage of the value held by _maxRAMPercent to dedicate to playback cache (viewer cache's in-RAM portion) only
    boost::shared_ptr<KnobInt> _maxPlayBackPercent;
    boost::shared_ptr<KnobString> _maxPlaybackLabel;

    ///The percentage of the system total's RAM to dedicate to caching in theory. In practise this is limited
    ///by _unreachableRamPercent that determines how much RAM should be left free for other use on the computer
    boost::shared_ptr<KnobInt> _maxRAMPercent;
    boost::shared_ptr<KnobString> _maxRAMLabel;

    ///The percentage of the system total's RAM you want to keep free from cache usage
    ///When the cache grows and reaches a point where it is about to cross that threshold
    ///it starts freeing the LRU entries regardless of the _maxRAMPercent and _maxPlaybackPercent
    ///A reasonable value should be set for it, allowing Natron's caches to always stay in RAM and
    ///avoid being swapped-out on disk. Assuming the user isn't using many applications at the same time,
    ///10% seems a reasonable value.
    boost::shared_ptr<KnobInt> _unreachableRAMPercent;
    boost::shared_ptr<KnobString> _unreachableRAMLabel;
    
    ///The total disk space allowed for all Natron's caches
    boost::shared_ptr<KnobInt> _maxViewerDiskCacheGB;
    boost::shared_ptr<KnobInt> _maxDiskCacheNodeGB;
    boost::shared_ptr<KnobPath> _diskCachePath;
    boost::shared_ptr<KnobButton> _wipeDiskCache;
    
    boost::shared_ptr<KnobPage> _viewersTab;
    boost::shared_ptr<KnobChoice> _texturesMode;
    boost::shared_ptr<KnobInt> _powerOf2Tiling;
    boost::shared_ptr<KnobInt> _checkerboardTileSize;
    boost::shared_ptr<KnobColor> _checkerboardColor1;
    boost::shared_ptr<KnobColor> _checkerboardColor2;
    boost::shared_ptr<KnobBool> _autoWipe;
    boost::shared_ptr<KnobBool> _autoProxyWhenScrubbingTimeline;
    boost::shared_ptr<KnobChoice> _autoProxyLevel;
    boost::shared_ptr<KnobBool> _enableProgressReport;
    
    boost::shared_ptr<KnobPage> _nodegraphTab;
    boost::shared_ptr<KnobBool> _autoTurbo;
    boost::shared_ptr<KnobBool> _useNodeGraphHints;
    boost::shared_ptr<KnobBool> _snapNodesToConnections;
    boost::shared_ptr<KnobBool> _useBWIcons;
    boost::shared_ptr<KnobInt> _maxUndoRedoNodeGraph;
    boost::shared_ptr<KnobInt> _disconnectedArrowLength;
    boost::shared_ptr<KnobBool> _hideOptionalInputsAutomatically;
    boost::shared_ptr<KnobBool> _useInputAForMergeAutoConnect;
    boost::shared_ptr<KnobBool> _usePluginIconsInNodeGraph;
    boost::shared_ptr<KnobBool> _useAntiAliasing;
    boost::shared_ptr<KnobColor> _defaultNodeColor;
    boost::shared_ptr<KnobColor> _defaultBackdropColor;
    boost::shared_ptr<KnobColor> _defaultGeneratorColor;
    boost::shared_ptr<KnobColor> _defaultReaderColor;
    boost::shared_ptr<KnobColor> _defaultWriterColor;
    boost::shared_ptr<KnobColor> _defaultColorGroupColor;
    boost::shared_ptr<KnobColor> _defaultFilterGroupColor;
    boost::shared_ptr<KnobColor> _defaultTransformGroupColor;
    boost::shared_ptr<KnobColor> _defaultTimeGroupColor;
    boost::shared_ptr<KnobColor> _defaultDrawGroupColor;
    boost::shared_ptr<KnobColor> _defaultKeyerGroupColor;
    boost::shared_ptr<KnobColor> _defaultChannelGroupColor;
    boost::shared_ptr<KnobColor> _defaultMergeGroupColor;
    boost::shared_ptr<KnobColor> _defaultViewsGroupColor;
    boost::shared_ptr<KnobColor> _defaultDeepGroupColor;
    boost::shared_ptr<KnobInt> _defaultAppearanceVersion;
    
    boost::shared_ptr<KnobPage> _readersTab;
    std::vector< boost::shared_ptr<KnobChoice> > _readersMapping;
    boost::shared_ptr<KnobPage> _writersTab;
    std::vector< boost::shared_ptr<KnobChoice> >  _writersMapping;
    
    boost::shared_ptr<KnobPath> _extraPluginPaths;
    boost::shared_ptr<KnobPath> _templatesPluginPaths;
    boost::shared_ptr<KnobBool> _preferBundledPlugins;
    boost::shared_ptr<KnobBool> _loadBundledPlugins;
    boost::shared_ptr<KnobPage> _pluginsTab;
    
    boost::shared_ptr<KnobPage> _pythonPage;
    boost::shared_ptr<KnobString> _onProjectCreated;
    boost::shared_ptr<KnobString> _defaultOnProjectLoaded;
    boost::shared_ptr<KnobString> _defaultOnProjectSave;
    boost::shared_ptr<KnobString> _defaultOnProjectClose;
    boost::shared_ptr<KnobString> _defaultOnNodeCreated;
    boost::shared_ptr<KnobString> _defaultOnNodeDelete;
    boost::shared_ptr<KnobBool> _loadPyPlugsFromPythonScript;
    
    boost::shared_ptr<KnobBool> _echoVariableDeclarationToPython;
    boost::shared_ptr<KnobPage> _appearanceTab;
    
    boost::shared_ptr<KnobChoice> _systemFontChoice;
    boost::shared_ptr<KnobInt> _fontSize;
    boost::shared_ptr<KnobFile> _qssFile;
    
    boost::shared_ptr<KnobGroup> _guiColors;
    boost::shared_ptr<KnobColor> _sunkenColor;
    boost::shared_ptr<KnobColor> _baseColor;
    boost::shared_ptr<KnobColor> _raisedColor;
    boost::shared_ptr<KnobColor> _selectionColor;
    boost::shared_ptr<KnobColor> _textColor;
    boost::shared_ptr<KnobColor> _altTextColor;
    boost::shared_ptr<KnobColor> _timelinePlayheadColor;
    boost::shared_ptr<KnobColor> _timelineBGColor;
    boost::shared_ptr<KnobColor> _timelineBoundsColor;
    boost::shared_ptr<KnobColor> _interpolatedColor;
    boost::shared_ptr<KnobColor> _keyframeColor;
    boost::shared_ptr<KnobColor> _exprColor;
    boost::shared_ptr<KnobColor> _cachedFrameColor;
    boost::shared_ptr<KnobColor> _diskCachedFrameColor;
    
    boost::shared_ptr<KnobGroup> _curveEditorColors;
    boost::shared_ptr<KnobColor> _curveEditorBGColor;
    boost::shared_ptr<KnobColor> _gridColor;
    boost::shared_ptr<KnobColor> _curveEditorScaleColor;

    boost::shared_ptr<KnobGroup> _dopeSheetEditorColors;
    boost::shared_ptr<KnobColor> _dopeSheetEditorBackgroundColor;
    boost::shared_ptr<KnobColor> _dopeSheetEditorRootSectionBackgroundColor;
    boost::shared_ptr<KnobColor> _dopeSheetEditorKnobSectionBackgroundColor;
    boost::shared_ptr<KnobColor> _dopeSheetEditorScaleColor;
    boost::shared_ptr<KnobColor> _dopeSheetEditorGridColor;
    
    boost::shared_ptr<KnobGroup> _scriptEditorColors;
    boost::shared_ptr<KnobColor> _curLineColor;
    boost::shared_ptr<KnobColor> _keywordColor;
    boost::shared_ptr<KnobColor> _operatorColor;
    boost::shared_ptr<KnobColor> _braceColor;
    boost::shared_ptr<KnobColor> _defClassColor;
    boost::shared_ptr<KnobColor> _stringsColor;
    boost::shared_ptr<KnobColor> _commentsColor;
    boost::shared_ptr<KnobColor> _selfColor;
    boost::shared_ptr<KnobColor> _numbersColor;
    
    
    struct PerPluginKnobs
    {
        boost::shared_ptr<KnobBool> enabled;
        boost::shared_ptr<KnobChoice> renderScaleSupport;
        
        PerPluginKnobs(const boost::shared_ptr<KnobBool>& enabled,
                       const boost::shared_ptr<KnobChoice>& renderScaleSupport)
        : enabled(enabled)
        , renderScaleSupport(renderScaleSupport)
        {
            
        }
        
        PerPluginKnobs()
        : enabled() , renderScaleSupport()
        {
            
        }
    };

    std::map<const Natron::Plugin*,PerPluginKnobs> _pluginsMap;
    std::vector<std::string> _knownHostNames;
    bool _restoringSettings;
    bool _ocioRestored;
    bool _settingsExisted;
    bool _defaultAppearanceOutdated;
};

#endif // NATRON_ENGINE_SETTINGS_H
