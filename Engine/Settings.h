/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef NATRON_ENGINE_SETTINGS_H
#define NATRON_ENGINE_SETTINGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <string>
#include <map>
#include <vector>

#include "Global/GlobalDefines.h"

#include "Engine/Knob.h"
#include "Engine/ChoiceOption.h"
#include "Engine/ViewIdx.h"
#include "Engine/EngineFwd.h"


#define kQSettingsSoftwareMajorVersionSettingName "SoftwareVersionMajor"

NATRON_NAMESPACE_ENTER

/*The current settings in the preferences menu.*/
class Settings
    : public KnobHolder
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    enum KnownHostNameEnum
    {
        eKnownHostNameNatron = 0,
        eKnownHostNameNuke,
        eKnownHostNameFusion,
        eKnownHostNameCatalyst,
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
        eKnownHostNameNewBlueOFXBridge,
        eKnownHostNameRamen,
        eKnownHostNameTuttleOfx,
        eKnownHostNameNone,
    };

    enum EnableOpenGLEnum
    {
        eEnableOpenGLEnabled = 0,
        eEnableOpenGLDisabled,
        eEnableOpenGLDisabledIfBackground,
    };

    Settings();

    virtual ~Settings()
    {
    }

    virtual bool canKnobsAnimate() const OVERRIDE FINAL
    {
        return false;
    }

    virtual bool onKnobValueChanged(KnobI* k,
                                    ValueChangedReasonEnum reason,
                                    double time,
                                    ViewSpec view,
                                    bool originatedFromMainThread) OVERRIDE FINAL;

    double getRamMaximumPercent() const;

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

    void setUseGlobalThreadPool(bool use);

    void restorePluginSettings();

    void populateSystemFonts(const QSettings& settings, const std::vector<std::string>& fonts);

    // should settings be saved?
    void setSaveSettings(bool);

    bool getSaveSettings() const;

    ///save the settings to the application's settings
    void saveSettings(const std::vector<KnobI*>& settings, bool doWarnings, bool pluginSettings);

    void savePluginsSettings();

    void saveAllSettings();

    void saveSetting(KnobI* knob)
    {
        std::vector<KnobI*> knobs;

        knobs.push_back(knob);
        saveSettings(knobs, false, false);
    }

    ///restores the settings from disk
    void restoreSettings(bool useDefault);

    void restoreKnobsFromSettings(const KnobsVec& knobs);
    void restoreKnobsFromSettings(const std::vector<KnobI*>& knobs);

    bool isAutoPreviewOnForNewProjects() const;

    void getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const;

    bool getUseStdOFXPluginsLocation() const;

    bool isRenderInSeparatedProcessEnabled() const;

    bool isRenderQueuingEnabled() const;

    void setRenderQueuingEnabled(bool enabled);

    void restoreDefault();

    int getMaximumUndoRedoNodeGraph() const;

    int getAutoSaveDelayMS() const;

    bool isAutoSaveEnabledForUnsavedProjects() const;

    int saveVersions() const;

    bool isSnapToNodeEnabled() const;

    bool isCheckForUpdatesEnabled() const;

    void setCheckUpdatesEnabled(bool enabled);

#ifdef NATRON_USE_BREAKPAD
    bool isCrashReportingEnabled() const;
#endif

    int getMaxPanelsOpened() const;

    void setMaxPanelsOpened(int maxPanels);

    bool loadBundledPlugins() const;

    bool preferBundledPlugins() const;

    void getDefaultNodeColor(float *r, float *g, float *b) const;

    void getDefaultBackdropColor(float *r, float *g, float *b) const;

    int getDisconnectedArrowLength() const;

    void getGeneratorColor(float *r, float *g, float *b) const;
    void getReaderColor(float *r, float *g, float *b) const;
    void getWriterColor(float *r, float *g, float *b) const;
    void getColorGroupColor(float *r, float *g, float *b) const;
    void getFilterGroupColor(float *r, float *g, float *b) const;
    void getTransformGroupColor(float *r, float *g, float *b) const;
    void getTimeGroupColor(float *r, float *g, float *b) const;
    void getDrawGroupColor(float *r, float *g, float *b) const;
    void getKeyerGroupColor(float *r, float *g, float *b) const;
    void getChannelGroupColor(float *r, float *g, float *b) const;
    void getMergeGroupColor(float *r, float *g, float *b) const;
    void getViewsGroupColor(float *r, float *g, float *b) const;
    void getDeepGroupColor(float *r, float *g, float *b) const;

    bool getRenderOnEditingFinishedOnly() const;
    void setRenderOnEditingFinishedOnly(bool render);

    bool getIconsBlackAndWhite() const;

    std::string getHostName() const;
    const std::string& getKnownHostName(KnownHostNameEnum e) const
    {
        return _knownHostNames[(int)e].id;
    }

    std::string getDefaultLayoutFile() const;

    bool getLoadProjectWorkspce() const;
#ifdef Q_OS_WIN
    bool getEnableConsoleWindow() const;
#endif

    bool useCursorPositionIncrements() const;

    bool isAutoProjectFormatEnabled() const;

    bool isAutoFixRelativeFilePathEnabled() const;

    ///////////////////////////////////////////////////////
    // "Viewers" pane
    ImageBitDepthEnum getViewersBitDepth() const;
    int getViewerTilesPowerOf2() const;
    int getCheckerboardTileSize() const;
    void getCheckerboardColor1(double* r, double* g, double* b, double* a) const;
    void getCheckerboardColor2(double* r, double* g, double* b, double* a) const;
    bool isAutoWipeEnabled() const;
    bool isAutoProxyEnabled() const;
    unsigned int getAutoProxyMipMapLevel() const;
    int getMaxOpenedNodesViewerContext() const;
    bool viewerNumberKeys() const;
    bool viewerOverlaysPath() const;
    ///////////////////////////////////////////////////////

    bool areRGBPixelComponentsSupported() const;

    bool isTransformConcatenationEnabled() const;

    bool useInputAForMergeAutoConnect() const;

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

    bool notifyOnFileChange() const;

    bool isAggressiveCachingEnabled() const;

    bool isAutoTurboEnabled() const;

    void setAutoTurboModeEnabled(bool e);

    bool isFileDialogEnabledForNewWriters() const;

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

    void getSunkenColor(double* r, double* g, double* b) const;
    void getBaseColor(double* r, double* g, double* b) const;
    void getRaisedColor(double* r, double* g, double* b) const;
    void getSelectionColor(double* r, double* g, double* b) const;
    void getInterpolatedColor(double* r, double* g, double* b) const;
    void getKeyframeColor(double* r, double* g, double* b) const;
    void getTrackerKeyframeColor(double* r, double* g, double* b) const;
    void getExprColor(double* r, double* g, double* b) const;
    void getTextColor(double* r, double* g, double* b) const;
    void getAltTextColor(double* r, double* g, double* b) const;
    void getTimelinePlayheadColor(double* r, double* g, double* b) const;
    void getTimelineBoundsColor(double* r, double* g, double* b) const;
    void getTimelineBGColor(double* r, double* g, double* b) const;
    void getCachedFrameColor(double* r, double* g, double* b) const;
    void getDiskCachedColor(double* r, double* g, double* b) const;
    void getCurveEditorBGColor(double* r, double* g, double* b) const;
    void getCurveEditorGridColor(double* r, double* g, double* b) const;
    void getCurveEditorScaleColor(double* r, double* g, double* b) const;
    void getDopeSheetEditorBackgroundColor(double* r, double* g, double* b) const;
    void getDopeSheetEditorRootRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getDopeSheetEditorKnobRowBackgroundColor(double* r, double* g, double* b, double *a) const;
    void getDopeSheetEditorScaleColor(double* r, double* g, double* b) const;
    void getDopeSheetEditorGridColor(double* r, double* g, double* b) const;
    void getSliderColor(double* r, double* g, double* b) const;


    void getSEKeywordColor(double* r, double* g, double* b) const;
    void getSEOperatorColor(double* r, double* g, double* b) const;
    void getSEBraceColor(double* r, double* g, double* b) const;
    void getSEDefClassColor(double* r, double* g, double* b) const;
    void getSEStringsColor(double* r, double* g, double* b) const;
    void getSECommentsColor(double* r, double* g, double* b) const;
    void getSESelfColor(double* r, double* g, double* b) const;
    void getSENumbersColor(double* r, double* g, double* b) const;
    void getSECurLineColor(double* r, double* g, double* b) const;

    int getSEFontSize() const;
    std::string getSEFontFamily() const;

    void getPluginIconFrameColor(int *r, int *g, int *b) const;
    int getDopeSheetEditorNodeSeparationWith() const;

    bool isNaNHandlingEnabled() const;

    bool isCopyInputImageForPluginRenderEnabled() const;

    bool isDefaultAppearanceOutdated() const;
    void restoreDefaultAppearance();

    std::string getUserStyleSheetFilePath() const;

#ifdef NATRON_DOCUMENTATION_ONLINE
    int getDocumentationSource() const;
#endif
    int getServerPort() const;
    void setServerPort(int port) const;

    QString makeHTMLDocumentation(bool genHTML) const;

    bool isAutoScrollEnabled() const;

    GLRendererID getActiveOpenGLRendererID() const;

    void populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers);

    bool isOpenGLRenderingEnabled() const;

    int getMaxOpenGLContexts() const;

    bool isDriveLetterToUNCPathConversionEnabled() const;

Q_SIGNALS:

    void settingChanged(KnobI* knob);

private:

    virtual void initializeKnobs() OVERRIDE FINAL;

    void initializeKnobsGeneral();
    void initializeKnobsThreading();
    void initializeKnobsRendering();
    void initializeKnobsGPU();
    void initializeKnobsProjectSetup();
    void initializeKnobsDocumentation();
    void initializeKnobsUserInterface();
    void initializeKnobsColorManagement();
    void initializeKnobsCaching();
    void initializeKnobsViewers();
    void initializeKnobsNodeGraph();
    void initializeKnobsPlugins();
    void initializeKnobsPython();
    void initializeKnobsAppearance();
    void initializeKnobsGuiColors();
    void initializeKnobsCurveEditorColors();
    void initializeKnobsDopeSheetColors();
    void initializeKnobsNodeGraphColors();
    void initializeKnobsScriptEditorColors();


    void warnChangedKnobs(const std::vector<KnobI*>& knobs);

    void setCachingLabels();
    void setDefaultValues();

    bool tryLoadOpenColorIOConfig();


    KnobBoolPtr _natronSettingsExist;
    KnobBoolPtr _saveSettings;


    // General
    KnobPagePtr _generalTab;
    KnobBoolPtr _checkForUpdates;
#ifdef NATRON_USE_BREAKPAD
    KnobBoolPtr _enableCrashReports;
    KnobButtonPtr _testCrashReportButton;
#endif
    KnobBoolPtr _autoSaveUnSavedProjects;
    KnobIntPtr _autoSaveDelay;
    KnobIntPtr _saveVersions;
    KnobChoicePtr _hostName;
    KnobStringPtr _customHostName;

    // General/Threading
    KnobPagePtr _threadingPage;
    KnobIntPtr _numberOfThreads;
    KnobIntPtr _numberOfParallelRenders;
    KnobBoolPtr _useThreadPool;
    KnobIntPtr _nThreadsPerEffect;
    KnobBoolPtr _renderInSeparateProcess;
    KnobBoolPtr _queueRenders;

    // General/Rendering
    KnobPagePtr _renderingPage;
    KnobBoolPtr _convertNaNValues;
    KnobBoolPtr _pluginUseImageCopyForSource;
    KnobBoolPtr _activateRGBSupport;
    KnobBoolPtr _activateTransformConcatenationSupport;

    // General/GPU rendering
    KnobPagePtr _gpuPage;
    KnobStringPtr _openglRendererString;
    KnobChoicePtr _availableOpenGLRenderers;
    KnobIntPtr _nOpenGLContexts;
    KnobChoicePtr _enableOpenGL;



    // General/Projects setup
    KnobPagePtr _projectsPage;
    KnobBoolPtr _firstReadSetProjectFormat;
    KnobBoolPtr _autoPreviewEnabledForNewProjects;
    KnobBoolPtr _fixPathsOnProjectPathChanged;
    KnobBoolPtr _enableMappingFromDriveLettersToUNCShareNames;

    // General/Documentation
    KnobPagePtr _documentationPage;
    KnobIntPtr _wwwServerPort;
#ifdef NATRON_DOCUMENTATION_ONLINE
    KnobChoicePtr _documentationSource;
#endif

    // General/User Interface
    KnobPagePtr _uiPage;
    KnobBoolPtr _notifyOnFileChange;

#ifdef NATRON_ENABLE_IO_META_NODES
    KnobBoolPtr _filedialogForWriters;
#endif
    KnobBoolPtr _renderOnEditingFinished;
    KnobBoolPtr _linearPickers;
    KnobIntPtr _maxPanelsOpened;
    KnobBoolPtr _useCursorPositionIncrements;
    KnobFilePtr _defaultLayoutFile;
    KnobBoolPtr _loadProjectsWorkspace;
#ifdef Q_OS_WIN
    KnobBoolPtr _enableConsoleWindow;
#endif

    // Color-Management
    KnobPagePtr _ocioTab;
    KnobChoicePtr _ocioConfigKnob;
    KnobBoolPtr _warnOcioConfigKnobChanged;
    KnobBoolPtr _ocioStartupCheck;
    KnobFilePtr _customOcioConfigFile;

    // Caching
    KnobPagePtr _cachingTab;
    KnobBoolPtr _aggressiveCaching;
    ///The percentage of the value held by _maxRAMPercent to dedicate to playback cache (viewer cache's in-RAM portion) only
    KnobStringPtr _maxPlaybackLabel;

    ///The percentage of the system total's RAM to dedicate to caching in theory. In practise this is limited
    ///by _unreachableRamPercent that determines how much RAM should be left free for other use on the computer
    KnobIntPtr _maxRAMPercent;
    KnobStringPtr _maxRAMLabel;

    ///The percentage of the system total's RAM you want to keep free from cache usage
    ///When the cache grows and reaches a point where it is about to cross that threshold
    ///it starts freeing the LRU entries regardless of the _maxRAMPercent and _maxPlaybackPercent
    ///A reasonable value should be set for it, allowing Natron's caches to always stay in RAM and
    ///avoid being swapped-out on disk. Assuming the user isn't using many applications at the same time,
    ///10% seems a reasonable value.
    KnobIntPtr _unreachableRAMPercent;
    KnobStringPtr _unreachableRAMLabel;

    ///The total disk space allowed for all Natron's caches
    KnobIntPtr _maxViewerDiskCacheGB;
    KnobIntPtr _maxDiskCacheNodeGB;
    KnobPathPtr _diskCachePath;
    KnobButtonPtr _wipeDiskCache;

    // Viewer
    KnobPagePtr _viewersTab;
    KnobChoicePtr _texturesMode;
    KnobIntPtr _powerOf2Tiling;
    KnobIntPtr _checkerboardTileSize;
    KnobColorPtr _checkerboardColor1;
    KnobColorPtr _checkerboardColor2;
    KnobBoolPtr _autoWipe;
    KnobBoolPtr _autoProxyWhenScrubbingTimeline;
    KnobChoicePtr _autoProxyLevel;
    KnobIntPtr _maximumNodeViewerUIOpened;
    KnobBoolPtr _viewerNumberKeys;
    KnobBoolPtr _viewerOverlaysPath;

    // Nodegraph
    KnobPagePtr _nodegraphTab;
    KnobBoolPtr _autoScroll;
    KnobBoolPtr _autoTurbo;
    KnobBoolPtr _snapNodesToConnections;
    KnobBoolPtr _useBWIcons;
    KnobIntPtr _maxUndoRedoNodeGraph;
    KnobIntPtr _disconnectedArrowLength;
    KnobBoolPtr _hideOptionalInputsAutomatically;
    KnobBoolPtr _useInputAForMergeAutoConnect;
    KnobBoolPtr _usePluginIconsInNodeGraph;
    KnobBoolPtr _useAntiAliasing;

    // Plugins
    KnobPagePtr _pluginsTab;
    KnobPathPtr _extraPluginPaths;
    KnobBoolPtr _useStdOFXPluginsLocation;
    KnobPathPtr _templatesPluginPaths;
    KnobBoolPtr _preferBundledPlugins;
    KnobBoolPtr _loadBundledPlugins;

    // Python
    KnobPagePtr _pythonPage;
    KnobStringPtr _onProjectCreated;
    KnobStringPtr _defaultOnProjectLoaded;
    KnobStringPtr _defaultOnProjectSave;
    KnobStringPtr _defaultOnProjectClose;
    KnobStringPtr _defaultOnNodeCreated;
    KnobStringPtr _defaultOnNodeDelete;
    KnobBoolPtr _loadPyPlugsFromPythonScript;
    KnobBoolPtr _echoVariableDeclarationToPython;

    // Appearance
    KnobPagePtr _appearanceTab;
    KnobChoicePtr _systemFontChoice;
    KnobIntPtr _fontSize;
    KnobFilePtr _qssFile;
    KnobIntPtr _defaultAppearanceVersion;

    // Appearance/Main Window
    KnobPagePtr _guiColorsTab;
    KnobColorPtr _sunkenColor;
    KnobColorPtr _baseColor;
    KnobColorPtr _raisedColor;
    KnobColorPtr _selectionColor;
    KnobColorPtr _textColor;
    KnobColorPtr _altTextColor;
    KnobColorPtr _timelinePlayheadColor;
    KnobColorPtr _timelineBGColor;
    KnobColorPtr _timelineBoundsColor;
    KnobColorPtr _interpolatedColor;
    KnobColorPtr _keyframeColor;
    KnobColorPtr _trackerKeyframeColor;
    KnobColorPtr _exprColor;
    KnobColorPtr _cachedFrameColor;
    KnobColorPtr _diskCachedFrameColor;
    KnobColorPtr _sliderColor;

    // Apprance/Curve Editor
    KnobPagePtr _curveEditorColorsTab;
    KnobColorPtr _curveEditorBGColor;
    KnobColorPtr _gridColor;
    KnobColorPtr _curveEditorScaleColor;

    // Appearance/Dope Sheet
    KnobPagePtr _dopeSheetEditorColorsTab;
    KnobColorPtr _dopeSheetEditorBackgroundColor;
    KnobColorPtr _dopeSheetEditorRootSectionBackgroundColor;
    KnobColorPtr _dopeSheetEditorKnobSectionBackgroundColor;
    KnobColorPtr _dopeSheetEditorScaleColor;
    KnobColorPtr _dopeSheetEditorGridColor;

    // Appearance/Script Editor
    KnobPagePtr _scriptEditorColorsTab;
    KnobColorPtr _curLineColor;
    KnobColorPtr _keywordColor;
    KnobColorPtr _operatorColor;
    KnobColorPtr _braceColor;
    KnobColorPtr _defClassColor;
    KnobColorPtr _stringsColor;
    KnobColorPtr _commentsColor;
    KnobColorPtr _selfColor;
    KnobColorPtr _numbersColor;
    KnobChoicePtr _scriptEditorFontChoice;
    KnobIntPtr _scriptEditorFontSize;

    // Appearance/Node Graph
    KnobPagePtr _nodegraphColorsTab;
    KnobColorPtr _defaultNodeColor;
    KnobColorPtr _defaultBackdropColor;
    KnobColorPtr _defaultGeneratorColor;
    KnobColorPtr _defaultReaderColor;
    KnobColorPtr _defaultWriterColor;
    KnobColorPtr _defaultColorGroupColor;
    KnobColorPtr _defaultFilterGroupColor;
    KnobColorPtr _defaultTransformGroupColor;
    KnobColorPtr _defaultTimeGroupColor;
    KnobColorPtr _defaultDrawGroupColor;
    KnobColorPtr _defaultKeyerGroupColor;
    KnobColorPtr _defaultChannelGroupColor;
    KnobColorPtr _defaultMergeGroupColor;
    KnobColorPtr _defaultViewsGroupColor;
    KnobColorPtr _defaultDeepGroupColor;
    std::vector<ChoiceOption> _knownHostNames;
    bool _restoringSettings;
    bool _ocioRestored;
    bool _settingsExisted;
    bool _defaultAppearanceOutdated;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_SETTINGS_H
