/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Settings.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>
#include <QtCore/QThread>
#include <QtCore/QTextStream>

#ifdef WINDOWS
#include <tchar.h>
#endif

#include "Global/MemoryInfo.h"
#include "Global/StrUtils.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/OSGLContext.h"
#include "Engine/StandardPaths.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiDefines.h"

#include <SequenceParsing.h> // for SequenceParsing::removePath

#ifdef WINDOWS
#include <ofxhPluginCache.h>
#endif

#ifdef HAVE_OSMESA
#include "Engine/OSGLFunctions_mesa.h"
#endif

#define NATRON_DEFAULT_OCIO_CONFIG_NAME "blender"


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

#define NATRON_DEFAULT_APPEARANCE_VERSION 1

#define NATRON_CUSTOM_HOST_NAME_ENTRY "Custom..."

NATRON_NAMESPACE_ENTER;

enum CPUDriverEnum {
    eCPUDriverSoftPipe = 0,
    eCPUDriverLLVMPipe
};

static const CPUDriverEnum defaultMesaDriver = eCPUDriverLLVMPipe;

struct SettingsPrivate
{

    Q_DECLARE_TR_FUNCTIONS(Settings)

public:

    KnobBoolPtr _natronSettingsExist;


    // General
    KnobPagePtr _generalTab;
    KnobBoolPtr _checkForUpdates;
    KnobBoolPtr _enableCrashReports;
    KnobButtonPtr _testCrashReportButton;
    KnobBoolPtr _autoSaveUnSavedProjects;
    KnobIntPtr _autoSaveDelay;
    KnobBoolPtr _saveSafetyMode;
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
    KnobChoicePtr _osmesaRenderers;
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

    KnobBoolPtr _filedialogForWriters;

    KnobBoolPtr _renderOnEditingFinished;
    KnobBoolPtr _linearPickers;
    KnobIntPtr _maxPanelsOpened;
    KnobBoolPtr _useCursorPositionIncrements;
    KnobFilePtr _defaultLayoutFile;
    KnobBoolPtr _loadProjectsWorkspace;

    // Color-Management
    KnobPagePtr _ocioTab;
    KnobChoicePtr _ocioConfigKnob;
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
    KnobBoolPtr _viewerKeys;

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

    // Appearance/Dope Sheet
    KnobPagePtr _animationColorsTab;
    KnobColorPtr _animationViewBackgroundColor;
    KnobColorPtr _dopesheetRootSectionBackgroundColor;
    KnobColorPtr _dopesheetKnobSectionBackgroundColor;
    KnobColorPtr _animationViewScaleColor;
    KnobColorPtr _animationViewGridColor;

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
    KnobColorPtr _cloneColor;
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
    std::vector<std::string> _knownHostNames;

    Settings* _publicInterface;

    std::set<KnobIWPtr> knobsRequiringRestart;

    // Count the number of stylesheet changes during restore defaults
    // to avoid reloading it many times
    int _nRedrawStyleSheetRequests;
    bool _restoringDefaults;
    bool _restoringSettings;
    bool _ocioRestored;
    bool _settingsExisted;
    bool _defaultAppearanceOutdated;



    SettingsPrivate(Settings* publicInterface)
    : _publicInterface(publicInterface)
    , knobsRequiringRestart()
    , _nRedrawStyleSheetRequests(0)
    , _restoringDefaults(false)
    , _restoringSettings(false)
    , _ocioRestored(false)
    , _settingsExisted(false)
    , _defaultAppearanceOutdated(false)
    {
        
    }

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
    void initializeKnobsDopeSheetColors();
    void initializeKnobsNodeGraphColors();
    void initializeKnobsScriptEditorColors();

    void setCachingLabels();
    void setDefaultValues();

    bool tryLoadOpenColorIOConfig();

};


Settings::Settings()
: KnobHolder( AppInstancePtr() ) // < Settings are process wide and do not belong to a single AppInstance
, _imp(new SettingsPrivate(this))
{
}

Settings::~Settings()
{
}


static QStringList
getDefaultOcioConfigPaths()
{
    QString binaryPath = appPTR->getApplicationBinaryPath();
    StrUtils::ensureLastPathSeparator(binaryPath);

#ifdef __NATRON_LINUX__
    QStringList ret;
    ret.push_back( QString::fromUtf8("/usr/share/OpenColorIO-Configs") );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../share/OpenColorIO-Configs") ) );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );

    return ret;
#elif defined(__NATRON_WIN32__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#elif defined(__NATRON_OSX__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#endif
}

bool
Settings::doesKnobChangeRequiresRestart(const KnobIPtr& knob)
{
    std::set<KnobIWPtr>::iterator found = _imp->knobsRequiringRestart.find(knob);
    if (found == _imp->knobsRequiringRestart.end()) {
        return false;
    }
    return true;
}

void
Settings::initializeKnobs()
{
    _imp->initializeKnobsGeneral();
    _imp->initializeKnobsThreading();
    _imp->initializeKnobsRendering();
    _imp->initializeKnobsGPU();
    _imp->initializeKnobsProjectSetup();
    _imp->initializeKnobsDocumentation();
    _imp->initializeKnobsUserInterface();
    _imp->initializeKnobsColorManagement();
    _imp->initializeKnobsCaching();
    _imp->initializeKnobsViewers();
    _imp->initializeKnobsNodeGraph();
    _imp->initializeKnobsPlugins();
    _imp->initializeKnobsPython();
    _imp->initializeKnobsAppearance();
    _imp->initializeKnobsGuiColors();
    _imp->initializeKnobsDopeSheetColors();
    _imp->initializeKnobsNodeGraphColors();
    _imp->initializeKnobsScriptEditorColors();

    _imp->setDefaultValues();
}

void
SettingsPrivate::initializeKnobsGeneral()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _generalTab = AppManager::createKnob<KnobPage>( thisShared, tr("General") );

    _natronSettingsExist = AppManager::createKnob<KnobBool>( thisShared, tr("Existing settings") );
    _natronSettingsExist->setName("existingSettings");
    _natronSettingsExist->setSecret(true);
    _generalTab->addKnob(_natronSettingsExist);

    _checkForUpdates = AppManager::createKnob<KnobBool>( thisShared, tr("Always check for updates on start-up") );
    _checkForUpdates->setName("checkForUpdates");
    _checkForUpdates->setHintToolTip( tr("When checked, %1 will check for new updates on start-up of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    knobsRequiringRestart.insert(_checkForUpdates);
    _generalTab->addKnob(_checkForUpdates);

    _enableCrashReports = AppManager::createKnob<KnobBool>( thisShared, tr("Enable crash reporting") );
    _enableCrashReports->setName("enableCrashReports");
    _enableCrashReports->setHintToolTip( tr("When checked, if %1 crashes a window will pop-up asking you "
                                            "whether you want to upload the crash dump to the developers or not. "
                                            "This can help them track down the bug.\n"
                                            "If you need to turn the crash reporting system off, uncheck this.\n"
                                            "Note that when using the application in command-line mode, if crash reports are "
                                            "enabled, they will be automatically uploaded.\n"
                                            "Changing this requires a restart of the application to take effect.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableCrashReports->setAddNewLine(false);
    knobsRequiringRestart.insert(_enableCrashReports);
    _generalTab->addKnob(_enableCrashReports);

    _testCrashReportButton = AppManager::createKnob<KnobButton>( thisShared, tr("Test Crash Reporting") );
    _testCrashReportButton->setName("testCrashReporting");
    _testCrashReportButton->setHintToolTip( tr("This button is for developers only to test whether the crash reporting system "
                                               "works correctly. Do not use this.") );
    _generalTab->addKnob(_testCrashReportButton);


    _autoSaveDelay = AppManager::createKnob<KnobInt>( thisShared, tr("Auto-save trigger delay") );
    _autoSaveDelay->setName("autoSaveDelay");
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setRange(0, 60);
    _autoSaveDelay->setHintToolTip( tr("The number of seconds after an event that %1 should wait before "
                                       " auto-saving. Note that if a render is in progress, %1 will "
                                       " wait until it is done to actually auto-save.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveDelay);


    _autoSaveUnSavedProjects = AppManager::createKnob<KnobBool>( thisShared, tr("Enable Auto-save for unsaved projects") );
    _autoSaveUnSavedProjects->setName("autoSaveUnSavedProjects");
    _autoSaveUnSavedProjects->setHintToolTip( tr("When activated %1 will auto-save projects that have never been "
                                                 "saved and will prompt you on startup if an auto-save of that unsaved project was found. "
                                                 "Disabling this will no longer save un-saved project.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveUnSavedProjects);


    _saveSafetyMode = AppManager::createKnob<KnobBool>( thisShared, tr("Full Recovery Save") );
    _saveSafetyMode->setName("fullRecoverySave");
    _saveSafetyMode->setHintToolTip(tr("The save safety is used when saving projects. When checked all default values of parameters will be saved in the project, even if they did not change. This is useful "
                                       "in the case a default value in a plug-in was changed by its developers, to ensure that the value is still the same when loading a project."
                                       "By default this should not be needed as default values change are very rare. In a scenario where a project cannot be recovered in a newer version because "
                                       "the default values for a node have changed, just save your project in an older version of %1 with this parameter checked so that it reloads correctly in the newer version.\n"
                                       "Note that checking this parameter can make project files significantly larger.").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)));
    _generalTab->addKnob(_saveSafetyMode);


    _hostName = AppManager::createKnob<KnobChoice>( thisShared, tr("Appear to plug-ins as") );
    _hostName->setName("pluginHostName");
    knobsRequiringRestart.insert(_hostName);
    _hostName->setHintToolTip( tr("WARNING: Changing this requires clearing the OpenFX plug-ins load cache from the Cache menu.\n"
                                  "%1 will appear with the name of the selected application to the OpenFX plug-ins. "
                                  "Changing it to the name of another application can help loading plugins which "
                                  "restrict their usage to specific OpenFX host(s). "
                                  "If a Host is not listed here, use the \"Custom\" entry to enter a custom host name.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _knownHostNames.clear();
    std::vector<std::string> visibleHostEntries, hostEntriesHelp;
    assert(_knownHostNames.size() == (int)eKnownHostNameNatron);
    _knownHostNames.push_back(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);
    visibleHostEntries.push_back(NATRON_APPLICATION_NAME);
    assert(_knownHostNames.size() == (int)eKnownHostNameNuke);
    _knownHostNames.push_back("uk.co.thefoundry.nuke");
    visibleHostEntries.push_back("Nuke");
    assert(_knownHostNames.size() == (int)eKnownHostNameFusion);
    _knownHostNames.push_back("com.eyeonline.Fusion");
    visibleHostEntries.push_back("Fusion");
    assert(_knownHostNames.size() == (int)eKnownHostNameCatalyst);
    _knownHostNames.push_back("com.sony.Catalyst.Edit");
    visibleHostEntries.push_back("Sony Catalyst Edit");
    assert(_knownHostNames.size() == (int)eKnownHostNameVegas);
    _knownHostNames.push_back("com.sonycreativesoftware.vegas");
    visibleHostEntries.push_back("Sony Vegas");
    assert(_knownHostNames.size() == (int)eKnownHostNameToxik);
    _knownHostNames.push_back("Autodesk Toxik");
    visibleHostEntries.push_back("Toxik");
    assert(_knownHostNames.size() == (int)eKnownHostNameScratch);
    _knownHostNames.push_back("Assimilator");
    visibleHostEntries.push_back("Scratch");
    assert(_knownHostNames.size() == (int)eKnownHostNameDustBuster);
    _knownHostNames.push_back("Dustbuster");
    visibleHostEntries.push_back("DustBuster");
    assert(_knownHostNames.size() == (int)eKnownHostNameResolve);
    _knownHostNames.push_back("DaVinciResolve");
    visibleHostEntries.push_back("Da Vinci Resolve");
    assert(_knownHostNames.size() == (int)eKnownHostNameResolveLite);
    _knownHostNames.push_back("DaVinciResolveLite");
    visibleHostEntries.push_back("Da Vinci Resolve Lite");
    assert(_knownHostNames.size() == (int)eKnownHostNameMistika);
    _knownHostNames.push_back("Mistika");
    visibleHostEntries.push_back("SGO Mistika");
    assert(_knownHostNames.size() == (int)eKnownHostNamePablo);
    _knownHostNames.push_back("com.quantel.genq");
    visibleHostEntries.push_back("Quantel Pablo Rio");
    assert(_knownHostNames.size() == (int)eKnownHostNameMotionStudio);
    _knownHostNames.push_back("com.idtvision.MotionStudio");
    visibleHostEntries.push_back("IDT Motion Studio");
    assert(_knownHostNames.size() == (int)eKnownHostNameShake);
    _knownHostNames.push_back("com.apple.shake");
    visibleHostEntries.push_back("Shake");
    assert(_knownHostNames.size() == (int)eKnownHostNameBaselight);
    _knownHostNames.push_back("Baselight");
    visibleHostEntries.push_back("Baselight");
    assert(_knownHostNames.size() == (int)eKnownHostNameFrameCycler);
    _knownHostNames.push_back("IRIDAS Framecycler");
    visibleHostEntries.push_back("FrameCycler");
    assert(_knownHostNames.size() == (int)eKnownHostNameNucoda);
    _knownHostNames.push_back("Nucoda");
    visibleHostEntries.push_back("Nucoda Film Master");
    assert(_knownHostNames.size() == (int)eKnownHostNameAvidDS);
    _knownHostNames.push_back("DS OFX HOST");
    visibleHostEntries.push_back("Avid DS");
    assert(_knownHostNames.size() == (int)eKnownHostNameDX);
    _knownHostNames.push_back("com.chinadigitalvideo.dx");
    visibleHostEntries.push_back("China Digital Video DX");
    assert(_knownHostNames.size() == (int)eKnownHostNameTitlerPro);
    _knownHostNames.push_back("com.newblue.titlerpro");
    visibleHostEntries.push_back("NewBlueFX Titler Pro");
    assert(_knownHostNames.size() == (int)eKnownHostNameNewBlueOFXBridge);
    _knownHostNames.push_back("com.newblue.ofxbridge");
    visibleHostEntries.push_back("NewBlueFX OFX Bridge");
    assert(_knownHostNames.size() == (int)eKnownHostNameRamen);
    _knownHostNames.push_back("Ramen");
    visibleHostEntries.push_back("Ramen");
    assert(_knownHostNames.size() == (int)eKnownHostNameTuttleOfx);
    _knownHostNames.push_back("TuttleOfx");
    visibleHostEntries.push_back("TuttleOFX");


    assert( visibleHostEntries.size() == _knownHostNames.size() );
    hostEntriesHelp = _knownHostNames;

    visibleHostEntries.push_back(NATRON_CUSTOM_HOST_NAME_ENTRY);
    hostEntriesHelp.push_back("Custom host name");

    _hostName->populateChoices(visibleHostEntries, hostEntriesHelp);
    _hostName->setAddNewLine(false);
    _generalTab->addKnob(_hostName);

    _customHostName = AppManager::createKnob<KnobString>( thisShared, tr("Custom Host name") );
    _customHostName->setName("customHostName");
    knobsRequiringRestart.insert(_customHostName);
    _customHostName->setHintToolTip( tr("This is the name of the OpenFX host (application) as it appears to the OpenFX plugins. "
                                        "Changing it to the name of another application can help loading some plugins which "
                                        "restrict their usage to specific OpenFX hosts. You shoud leave "
                                        "this to its default value, unless a specific plugin refuses to load or run. "
                                        "Changing this takes effect upon the next application launch, and requires clearing "
                                        "the OpenFX plugins cache from the Cache menu. "
                                        "The default host name is: \n%1").arg( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME) ) );
    _customHostName->setSecret(true);
    _generalTab->addKnob(_customHostName);
} // Settings::initializeKnobsGeneral

void
SettingsPrivate::initializeKnobsThreading()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _threadingPage = AppManager::createKnob<KnobPage>( thisShared, tr("Threading") );

    _numberOfThreads = AppManager::createKnob<KnobInt>( thisShared, tr("Number of render threads (0=\"guess\")") );
    _numberOfThreads->setName("noRenderThreads");

    QString numberOfThreadsToolTip = tr("Controls how many threads %1 should use to render. \n"
                                        "-1: Disable multithreading totally (useful for debugging) \n"
                                        "0: Guess the thread count from the number of cores. The ideal threads count for this hardware is %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QThread::idealThreadCount() );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
    _numberOfThreads->setRange(-1, INT_MAX);
    _numberOfThreads->setDisplayRange(-1, 30);
    _threadingPage->addKnob(_numberOfThreads);

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders = AppManager::createKnob<KnobInt>( thisShared, tr("Number of parallel renders (0=\"guess\")") );
    _numberOfParallelRenders->setHintToolTip( tr("Controls the number of parallel frame that will be rendered at the same time by the renderer."
                                                 "A value of 0 indicate that %1 should automatically determine "
                                                 "the best number of parallel renders to launch given your CPU activity. "
                                                 "Setting a value different than 0 should be done only if you know what you're doing and can lead "
                                                 "in some situations to worse performances. Overall to get the best performances you should have your "
                                                 "CPU at 100% activity without idle times.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _numberOfParallelRenders->setName("nParallelRenders");
    _numberOfParallelRenders->setRange(0, INT_MAX);
    _numberOfParallelRenders->disableSlider();
    _threadingPage->addKnob(_numberOfParallelRenders);
#endif

    _useThreadPool = AppManager::createKnob<KnobBool>( thisShared, tr("Effects use the thread-pool") );

    _useThreadPool->setName("useThreadPool");
    _useThreadPool->setHintToolTip( tr("When checked, all effects will use a global thread-pool to do their processing instead of launching "
                                       "their own threads. "
                                       "This suppresses the overhead created by the operating system creating new threads on demand for "
                                       "each rendering of a special effect. As a result of this, the rendering might be faster on systems "
                                       "with a lot of cores (>= 8). \n"
                                       "WARNING: This is known not to work when using The Foundry's Furnace plug-ins (and potentially "
                                       "some other plug-ins that the dev team hasn't not tested against it). When using these plug-ins, "
                                       "make sure to uncheck this option first otherwise it will crash %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_useThreadPool);

    _nThreadsPerEffect = AppManager::createKnob<KnobInt>( thisShared, tr("Max threads usable per effect (0=\"guess\")") );
    _nThreadsPerEffect->setName("nThreadsPerEffect");
    _nThreadsPerEffect->setHintToolTip( tr("Controls how many threads a specific effect can use at most to do its processing. "
                                           "A high value will allow 1 effect to spawn lots of thread and might not be efficient because "
                                           "the time spent to launch all the threads might exceed the time spent actually processing."
                                           "By default (0) the renderer applies an heuristic to determine what's the best number of threads "
                                           "for an effect.") );

    _nThreadsPerEffect->setRange(0, INT_MAX);
    _nThreadsPerEffect->disableSlider();
    _threadingPage->addKnob(_nThreadsPerEffect);

    _renderInSeparateProcess = AppManager::createKnob<KnobBool>( thisShared, tr("Render in a separate process") );
    _renderInSeparateProcess->setName("renderNewProcess");
    _renderInSeparateProcess->setHintToolTip( tr("If true, %1 will render frames to disk in "
                                                 "a separate process so that if the main application crashes, the render goes on.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_renderInSeparateProcess);

    _queueRenders = AppManager::createKnob<KnobBool>( thisShared, tr("Append new renders to queue") );
    _queueRenders->setHintToolTip( tr("When checked, renders will be queued in the Progress Panel and will start only when all "
                                      "other prior tasks are done.") );
    _queueRenders->setName("queueRenders");
    _threadingPage->addKnob(_queueRenders);
} // Settings::initializeKnobsThreading

void
SettingsPrivate::initializeKnobsRendering()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _renderingPage = AppManager::createKnob<KnobPage>( thisShared, tr("Rendering") );

    _convertNaNValues = AppManager::createKnob<KnobBool>( thisShared, tr("Convert NaN values") );
    _convertNaNValues->setName("convertNaNs");
    _convertNaNValues->setHintToolTip( tr("When activated, any pixel that is a Not-a-Number will be converted to 1 to avoid potential crashes from "
                                          "downstream nodes. These values can be produced by faulty plug-ins when they use wrong arithmetic such as "
                                          "division by zero. Disabling this option will keep the NaN(s) in the buffers: this may lead to an "
                                          "undefined behavior.") );
    _renderingPage->addKnob(_convertNaNValues);

    _pluginUseImageCopyForSource = AppManager::createKnob<KnobBool>( thisShared, tr("Copy input image before rendering any plug-in") );
    _pluginUseImageCopyForSource->setName("copyInputImage");
    _pluginUseImageCopyForSource->setHintToolTip( tr("If checked, when before rendering any node, %1 will copy "
                                                     "the input image to a local temporary image. This is to work-around some plug-ins "
                                                     "that write to the source image, thus modifying the output of the node upstream in "
                                                     "the cache. This is a known bug of an old version of RevisionFX REMap for instance. "
                                                     "By default, this parameter should be leaved unchecked, as this will require an extra "
                                                     "image allocation and copy before rendering any plug-in.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _renderingPage->addKnob(_pluginUseImageCopyForSource);

    _activateRGBSupport = AppManager::createKnob<KnobBool>( thisShared, tr("RGB components support") );
    _activateRGBSupport->setHintToolTip( tr("When checked %1 is able to process images with only RGB components "
                                            "(support for images with RGBA and Alpha components is always enabled). "
                                            "Un-checking this option may prevent plugins that do not well support RGB components from crashing %1. "
                                            "Changing this option requires a restart of the application.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateRGBSupport->setName("rgbSupport");
    _renderingPage->addKnob(_activateRGBSupport);


    _activateTransformConcatenationSupport = AppManager::createKnob<KnobBool>( thisShared, tr("Transforms concatenation support") );
    _activateTransformConcatenationSupport->setHintToolTip( tr("When checked %1 is able to concatenate transform effects "
                                                               "when they are chained in the compositing tree. This yields better results and faster "
                                                               "render times because the image is only filtered once instead of as many times as there are "
                                                               "transformations.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateTransformConcatenationSupport->setName("transformCatSupport");
    _renderingPage->addKnob(_activateTransformConcatenationSupport);
}

void
Settings::populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers)
{
    if ( renderers.empty() ) {
        _imp->_availableOpenGLRenderers->setSecret(true);
        _imp->_nOpenGLContexts->setSecret(true);
        _imp->_enableOpenGL->setSecret(true);
        return;
    }

    _imp->_nOpenGLContexts->setSecret(false);
    _imp->_enableOpenGL->setSecret(false);

    std::vector<std::string> entries( renderers.size() );
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        std::string option = it->vendorName + ' ' + it->rendererName + ' ' + it->glVersionString;
        entries[i] = option;
    }
    _imp->_availableOpenGLRenderers->populateChoices(entries);
    _imp->_availableOpenGLRenderers->setSecret(renderers.size() == 1);


#ifdef HAVE_OSMESA
#ifdef OSMESA_GALLIUM_DRIVER
    std::vector<std::string> mesaDrivers;
    mesaDrivers.push_back("softpipe");
    mesaDrivers.push_back("llvmpipe");
    _imp->_osmesaRenderers->populateChoices(mesaDrivers);
    _imp->_osmesaRenderers->setSecret(false);
#else
    _imp->_osmesaRenderers->setSecret(false);
#endif
#else
    _imp->_osmesaRenderers->setSecret(true);
#endif
}

GLRendererID
Settings::getOpenGLCPUDriver() const
{
    GLRendererID ret;
    if (!_imp->_osmesaRenderers->getIsSecret()) {
        ret.renderID =  _imp->_osmesaRenderers->getValue();
    } else {
        ret.renderID = 0;
    }
    return ret;
}

bool
Settings::isOpenGLRenderingEnabled() const
{
    if (_imp->_enableOpenGL->getIsSecret()) {
        return false;
    }
    EnableOpenGLEnum enableOpenGL = (EnableOpenGLEnum)_imp->_enableOpenGL->getValue();
    return enableOpenGL == eEnableOpenGLEnabled || (enableOpenGL == eEnableOpenGLDisabledIfBackground && !appPTR->isBackground());
}

int
Settings::getMaxOpenGLContexts() const
{
    return _imp->_nOpenGLContexts->getValue();
}

GLRendererID
Settings::getActiveOpenGLRendererID() const
{
    if ( _imp->_availableOpenGLRenderers->getIsSecret() ) {
        // We were not able to detect multiple renderers, use default
        return GLRendererID();
    }
    int activeIndex = _imp->_availableOpenGLRenderers->getValue();
    const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
    if ( (activeIndex < 0) || ( activeIndex >= (int)renderers.size() ) ) {
        // Invalid index
        return GLRendererID();
    }
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        if (i == activeIndex) {
            return it->rendererID;
        }
    }

    return GLRendererID();
}

void
SettingsPrivate::initializeKnobsGPU()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _gpuPage = AppManager::createKnob<KnobPage>( thisShared, tr("GPU Rendering") );
    _openglRendererString = AppManager::createKnob<KnobString>( thisShared, tr("Active OpenGL renderer") );

    _openglRendererString->setName("activeOpenGLRenderer");
    _openglRendererString->setHintToolTip( tr("The currently active OpenGL renderer.") );
    _openglRendererString->setAsLabel();
    _gpuPage->addKnob(_openglRendererString);

    _availableOpenGLRenderers = AppManager::createKnob<KnobChoice>( thisShared, tr("OpenGL renderer") );
    _availableOpenGLRenderers->setName("chooseOpenGLRenderer");
    _availableOpenGLRenderers->setHintToolTip( tr("The renderer used to perform OpenGL rendering. Changing the OpenGL renderer requires a restart of the application.") );
    knobsRequiringRestart.insert(_availableOpenGLRenderers);
    _gpuPage->addKnob(_availableOpenGLRenderers);

    _osmesaRenderers = AppManager::createKnob<KnobChoice>(thisShared, tr("CPU OpenGL renderer"));
    _osmesaRenderers->setName("cpuOpenGLRenderer");
    knobsRequiringRestart.insert(_osmesaRenderers);
    _osmesaRenderers->setHintToolTip(tr("Internally, %1 can render OpenGL plug-ins on the CPU by using the OSMesa open-source library. You may select which driver OSMesa uses to perform it's CPU rendering. llvm-pipe is more efficient but may contain some bugs.").arg(QString::fromUtf8(NATRON_APPLICATION_NAME)));
    _gpuPage->addKnob(_osmesaRenderers);


    _nOpenGLContexts = AppManager::createKnob<KnobInt>( thisShared, tr("No. of OpenGL Contexts") );
    _nOpenGLContexts->setName("maxOpenGLContexts");
    _nOpenGLContexts->setRange(1, 8);
    _nOpenGLContexts->setDisplayRange(1, 8);;
    _nOpenGLContexts->setHintToolTip( tr("The number of OpenGL contexts created to perform OpenGL rendering. Each OpenGL context can be attached to a CPU thread, allowing for more frames to be rendered simultaneously. Increasing this value may increase performances for graphs with mixed CPU/GPU nodes but can drastically reduce performances if too many OpenGL contexts are active at once.") );
    _gpuPage->addKnob(_nOpenGLContexts);


    _enableOpenGL = AppManager::createKnob<KnobChoice>( thisShared, tr("OpenGL Rendering") );
    _enableOpenGL->setName("enableOpenGLRendering");
    {
        std::vector<std::string> entries;
        std::vector<std::string> helps;
        assert(entries.size() == (int)Settings::eEnableOpenGLEnabled);
        entries.push_back("Enabled");
        helps.push_back( tr("If a plug-in support GPU rendering, prefer rendering using the GPU if possible.").toStdString() );
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabled);
        entries.push_back("Disabled");
        helps.push_back( tr("Disable GPU rendering for all plug-ins.").toStdString() );
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabledIfBackground);
        entries.push_back("Disabled If Background");
        helps.push_back( tr("Disable GPU rendering when rendering with NatronRenderer but not in GUI mode.").toStdString() );
        _enableOpenGL->populateChoices(entries, helps);
    }
    _enableOpenGL->setHintToolTip( tr("Select whether to activate OpenGL rendering or not. If disabled, even though a Project enable GPU rendering, it will not be activated.") );
    _gpuPage->addKnob(_enableOpenGL);
}

void
SettingsPrivate::initializeKnobsProjectSetup()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _projectsPage = AppManager::createKnob<KnobPage>( thisShared, tr("Project Setup") );

    _firstReadSetProjectFormat = AppManager::createKnob<KnobBool>( thisShared, tr("First image read set project format") );
    _firstReadSetProjectFormat->setName("autoProjectFormat");
    _firstReadSetProjectFormat->setHintToolTip( tr("If checked, the project size is set to this of the first image or video read within the project.") );
    _projectsPage->addKnob(_firstReadSetProjectFormat);


    _autoPreviewEnabledForNewProjects = AppManager::createKnob<KnobBool>( thisShared, tr("Auto-preview enabled by default for new projects") );
    _autoPreviewEnabledForNewProjects->setName("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setHintToolTip( tr("If checked, then when creating a new project, the Auto-preview option"
                                                          " is enabled.") );
    _projectsPage->addKnob(_autoPreviewEnabledForNewProjects);


    _fixPathsOnProjectPathChanged = AppManager::createKnob<KnobBool>( thisShared, tr("Auto fix relative file-paths") );
    _fixPathsOnProjectPathChanged->setHintToolTip( tr("If checked, when a project-path changes (either the name or the value pointed to), %1 checks all file-path parameters in the project and tries to fix them.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _fixPathsOnProjectPathChanged->setName("autoFixRelativePaths");

    _projectsPage->addKnob(_fixPathsOnProjectPathChanged);

    _enableMappingFromDriveLettersToUNCShareNames = AppManager::createKnob<KnobBool>( thisShared, tr("Use drive letters instead of server names (Windows only)") );
    _enableMappingFromDriveLettersToUNCShareNames->setHintToolTip( tr("This is only relevant for Windows: If checked, %1 will not convert a path starting with a drive letter from the file dialog to a network share name. You may use this if for example you want to share a same project with several users across facilities with different servers but where users have all the same drive attached to a server.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableMappingFromDriveLettersToUNCShareNames->setName("useDriveLetters");
#ifndef __NATRON_WIN32__
    _enableMappingFromDriveLettersToUNCShareNames->setEnabled(false);
#endif
    _projectsPage->addKnob(_enableMappingFromDriveLettersToUNCShareNames);
}

void
SettingsPrivate::initializeKnobsDocumentation()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _documentationPage = AppManager::createKnob<KnobPage>( thisShared, tr("Documentation") );

#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource = AppManager::createKnob<KnobChoice>( thisShared, tr("Documentation Source") );
    _documentationSource->setName("documentationSource");
    _documentationSource->setHintToolTip( tr("Documentation source.") );
    _documentationSource->appendChoice("Local");
    _documentationSource->appendChoice("Online");
    _documentationSource->appendChoice("None");
    _documentationPage->addKnob(_documentationSource);
#endif

    /// used to store temp port for local webserver
    _wwwServerPort = AppManager::createKnob<KnobInt>( thisShared, tr("Documentation local port (0=auto)") );
    _wwwServerPort->setName("webserverPort");
    _wwwServerPort->setHintToolTip( tr("The port onto which the documentation server will listen to. A value of 0 indicate that the documentation should automatically find a port by itself.") );
    _documentationPage->addKnob(_wwwServerPort);
}

void
SettingsPrivate::initializeKnobsUserInterface()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _uiPage = AppManager::createKnob<KnobPage>( thisShared, tr("User Interface") );
    _uiPage->setName("userInterfacePage");

    _notifyOnFileChange = AppManager::createKnob<KnobBool>( thisShared, tr("Warn when a file changes externally") );
    _notifyOnFileChange->setName("warnOnExternalChange");
    _notifyOnFileChange->setHintToolTip( tr("When checked, if a file read from a file parameter changes externally, a warning will be displayed "
                                            "on the viewer. Turning this off will suspend the notification system.") );
    _uiPage->addKnob(_notifyOnFileChange);

    _filedialogForWriters = AppManager::createKnob<KnobBool>( thisShared, tr("Prompt with file dialog when creating Write node") );
    _filedialogForWriters->setName("writeUseDialog");
    _filedialogForWriters->setDefaultValue(true);
    _filedialogForWriters->setHintToolTip( tr("When checked, opens-up a file dialog when creating a Write node") );
    _uiPage->addKnob(_filedialogForWriters);


    _renderOnEditingFinished = AppManager::createKnob<KnobBool>( thisShared, tr("Refresh viewer only when editing is finished") );
    _renderOnEditingFinished->setName("renderOnEditingFinished");
    _renderOnEditingFinished->setHintToolTip( tr("When checked, the viewer triggers a new render only when mouse is released when editing parameters, curves "
                                                 " or the timeline. This setting doesn't apply to roto splines editing.") );
    _uiPage->addKnob(_renderOnEditingFinished);


    _linearPickers = AppManager::createKnob<KnobBool>( thisShared, tr("Linear color pickers") );
    _linearPickers->setName("linearPickers");
    _linearPickers->setHintToolTip( tr("When activated, all colors picked from the color parameters are linearized "
                                       "before being fetched. Otherwise they are in the same colorspace "
                                       "as the viewer they were picked from.") );
    _uiPage->addKnob(_linearPickers);

    _maxPanelsOpened = AppManager::createKnob<KnobInt>( thisShared, tr("Maximum number of open settings panels (0=\"unlimited\")") );
    _maxPanelsOpened->setName("maxPanels");
    _maxPanelsOpened->setHintToolTip( tr("This property holds the maximum number of settings panels that can be "
                                         "held by the properties dock at the same time."
                                         "The special value of 0 indicates there can be an unlimited number of panels opened.") );
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setRange(1, 100);
    _uiPage->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = AppManager::createKnob<KnobBool>( thisShared, tr("Value increments based on cursor position") );
    _useCursorPositionIncrements->setName("cursorPositionAwareFields");
    _useCursorPositionIncrements->setHintToolTip( tr("When enabled, incrementing the value fields of parameters with the "
                                                     "mouse wheel or with arrow keys will increment the digits on the right "
                                                     "of the cursor. \n"
                                                     "When disabled, the value fields are incremented given what the plug-in "
                                                     "decided it should be. You can alter this increment by holding "
                                                     "Shift (x10) or Control (/10) while incrementing.") );
    _uiPage->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = AppManager::createKnob<KnobFile>( thisShared, tr("Default layout file") );
    _defaultLayoutFile->setName("defaultLayout");
    _defaultLayoutFile->setHintToolTip( tr("When set, %1 uses the given layout file "
                                           "as default layout for new projects. You can export/import a layout to/from a file "
                                           "from the Layout menu. If empty, the default application layout is used.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _uiPage->addKnob(_defaultLayoutFile);

    _loadProjectsWorkspace = AppManager::createKnob<KnobBool>( thisShared, tr("Load workspace embedded within projects") );
    _loadProjectsWorkspace->setName("loadProjectWorkspace");
    _loadProjectsWorkspace->setHintToolTip( tr("When checked, when loading a project, the workspace (windows layout) will also be loaded, otherwise it "
                                               "will use your current layout.") );
    _uiPage->addKnob(_loadProjectsWorkspace);
} // Settings::initializeKnobsUserInterface

void
SettingsPrivate::initializeKnobsColorManagement()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _ocioTab = AppManager::createKnob<KnobPage>( thisShared, tr("Color Management") );
    _ocioConfigKnob = AppManager::createKnob<KnobChoice>( thisShared, tr("OpenColorIO configuration") );
    knobsRequiringRestart.insert(_ocioConfigKnob);

    _ocioConfigKnob->setName("ocioConfig");

    std::vector<std::string> configs;
    int defaultIndex = 0;
    QStringList defaultOcioConfigsPaths = getDefaultOcioConfigPaths();
    Q_FOREACH(const QString &defaultOcioConfigsDir, defaultOcioConfigsPaths) {
        QDir ocioConfigsDir(defaultOcioConfigsDir);

        if ( ocioConfigsDir.exists() ) {
            QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < entries.size(); ++j) {
                if ( entries[j] == QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ) {
                    defaultIndex = j;
                }
                configs.push_back( entries[j].toStdString() );
            }

            break; //if we found 1 OpenColorIO-Configs directory, skip the next
        }
    }
    configs.push_back(NATRON_CUSTOM_OCIO_CONFIG_NAME);
    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setDefaultValue(defaultIndex);
    _ocioConfigKnob->setHintToolTip( tr("Select the OpenColorIO configuration you would like to use globally for all "
                                        "operators and plugins that use OpenColorIO, by setting the \"OCIO\" "
                                        "environment variable. Only nodes created after changing this parameter will take "
                                        "it into account, and it is better to restart the application after changing it. "
                                        "When \"%1\" is selected, the "
                                        "\"Custom OpenColorIO config file\" parameter is used.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );

    _ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = AppManager::createKnob<KnobFile>( thisShared, tr("Custom OpenColorIO configuration file") );

    _customOcioConfigFile->setName("ocioCustomConfigFile");
    knobsRequiringRestart.insert(_customOcioConfigFile);

    if (_ocioConfigKnob->getNumEntries() == 1) {
        _customOcioConfigFile->setEnabled(true);
    } else {
        _customOcioConfigFile->setEnabled(false);
    }

    _customOcioConfigFile->setHintToolTip( tr("OpenColorIO configuration file (*.ocio) to use when \"%1\" "
                                              "is selected as the OpenColorIO config.").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );
    _ocioTab->addKnob(_customOcioConfigFile);


    _ocioStartupCheck = AppManager::createKnob<KnobBool>( thisShared, tr("Warn on startup if OpenColorIO config is not the default") );
    _ocioStartupCheck->setName("startupCheckOCIO");
    _ocioTab->addKnob(_ocioStartupCheck);
} // Settings::initializeKnobsColorManagement

void
SettingsPrivate::initializeKnobsAppearance()
{

    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _appearanceTab = AppManager::createKnob<KnobPage>( thisShared, tr("Appearance") );

    _defaultAppearanceVersion = AppManager::createKnob<KnobInt>( thisShared, tr("Appearance version") );
    _defaultAppearanceVersion->setName("appearanceVersion");
    _defaultAppearanceVersion->setSecret(true);
    _appearanceTab->addKnob(_defaultAppearanceVersion);

    _systemFontChoice = AppManager::createKnob<KnobChoice>( thisShared, tr("Font") );
    _systemFontChoice->setHintToolTip( tr("List of all fonts available on your system") );
    _systemFontChoice->setName("systemFont");
    _systemFontChoice->setAddNewLine(false);
    knobsRequiringRestart.insert(_systemFontChoice);
    _appearanceTab->addKnob(_systemFontChoice);

    _fontSize = AppManager::createKnob<KnobInt>( thisShared, tr("Font size") );
    _fontSize->setName("fontSize");
    knobsRequiringRestart.insert(_fontSize);
    _appearanceTab->addKnob(_fontSize);

    _qssFile = AppManager::createKnob<KnobFile>( thisShared, tr("Stylesheet file (.qss)") );
    _qssFile->setName("stylesheetFile");
    knobsRequiringRestart.insert(_qssFile);
    _qssFile->setHintToolTip( tr("When pointing to a valid .qss file, the stylesheet of the application will be set according to this file instead of the default "
                                 "stylesheet. You can adapt the default stylesheet that can be found in your distribution of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _appearanceTab->addKnob(_qssFile);
} // Settings::initializeKnobsAppearance

void
SettingsPrivate::initializeKnobsGuiColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _guiColorsTab = AppManager::createKnob<KnobPage>( thisShared, tr("Main Window") );
    _appearanceTab->addKnob(_guiColorsTab);

    _useBWIcons = AppManager::createKnob<KnobBool>( thisShared, tr("Use black & white toolbutton icons") );
    _useBWIcons->setName("useBwIcons");
    _useBWIcons->setHintToolTip( tr("When checked, the tools icons in the left toolbar are greyscale. Changing this takes "
                                    "effect upon the next launch of the application.") );
    _guiColorsTab->addKnob(_useBWIcons);


    _sunkenColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Sunken"), 3);
    _sunkenColor->setName("sunken");
    _sunkenColor->setSimplified(true);
    _guiColorsTab->addKnob(_sunkenColor);

    _baseColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Base"), 3);
    _baseColor->setName("base");
    _baseColor->setSimplified(true);
    _guiColorsTab->addKnob(_baseColor);

    _raisedColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Raised"), 3);
    _raisedColor->setName("raised");
    _raisedColor->setSimplified(true);
    _guiColorsTab->addKnob(_raisedColor);

    _selectionColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Selection"), 3);
    _selectionColor->setName("selection");
    _selectionColor->setSimplified(true);
    _guiColorsTab->addKnob(_selectionColor);

    _textColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Text"), 3);
    _textColor->setName("text");
    _textColor->setSimplified(true);
    _guiColorsTab->addKnob(_textColor);

    _altTextColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Unmodified text"), 3);
    _altTextColor->setName("unmodifiedText");
    _altTextColor->setSimplified(true);
    _guiColorsTab->addKnob(_altTextColor);

    _timelinePlayheadColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Timeline playhead"), 3);
    _timelinePlayheadColor->setName("timelinePlayhead");
    _timelinePlayheadColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelinePlayheadColor);


    _timelineBGColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Timeline background"), 3);
    _timelineBGColor->setName("timelineBG");
    _timelineBGColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBGColor);

    _timelineBoundsColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Timeline bounds"), 3);
    _timelineBoundsColor->setName("timelineBound");
    _timelineBoundsColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBoundsColor);

    _cachedFrameColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Cached frame"), 3);
    _cachedFrameColor->setName("cachedFrame");
    _cachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_cachedFrameColor);

    _diskCachedFrameColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Disk cached frame"), 3);
    _diskCachedFrameColor->setName("diskCachedFrame");
    _diskCachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_diskCachedFrameColor);

    _interpolatedColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Interpolated value"), 3);
    _interpolatedColor->setName("interpValue");
    _interpolatedColor->setSimplified(true);
    _guiColorsTab->addKnob(_interpolatedColor);

    _keyframeColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Keyframe"), 3);
    _keyframeColor->setName("keyframe");
    _keyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_keyframeColor);

    _trackerKeyframeColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Track User Keyframes"), 3);
    _trackerKeyframeColor->setName("trackUserKeyframe");
    _trackerKeyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_trackerKeyframeColor);

    _exprColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Expression"), 3);
    _exprColor->setName("exprColor");
    _exprColor->setSimplified(true);
    _guiColorsTab->addKnob(_exprColor);

    _sliderColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Slider"), 3);
    _sliderColor->setName("slider");
    _sliderColor->setSimplified(true);
    _guiColorsTab->addKnob(_sliderColor);
} // Settings::initializeKnobsGuiColors


void
SettingsPrivate::initializeKnobsDopeSheetColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _animationColorsTab = AppManager::createKnob<KnobPage>( thisShared, tr("Animation Module") );
    _appearanceTab->addKnob(_animationColorsTab);

    _animationViewBackgroundColor = AppManager::createKnob<KnobColor>(thisShared, tr("Background Color"), 3);
    _animationViewBackgroundColor->setName("animationViewBGColor");
    _animationViewBackgroundColor->setSimplified(true);
    _animationColorsTab->addKnob(_animationViewBackgroundColor);

    _dopesheetRootSectionBackgroundColor = AppManager::createKnob<KnobColor>(thisShared, tr("Root section background color"), 4);
    _dopesheetRootSectionBackgroundColor->setName("dopesheetRootSectionBackground");
    _dopesheetRootSectionBackgroundColor->setSimplified(true);
    _animationColorsTab->addKnob(_dopesheetRootSectionBackgroundColor);

    _dopesheetKnobSectionBackgroundColor = AppManager::createKnob<KnobColor>(thisShared, tr("Knob section background color"), 4);
    _dopesheetKnobSectionBackgroundColor->setName("dopesheetKnobSectionBackground");
    _dopesheetKnobSectionBackgroundColor->setSimplified(true);
    _animationColorsTab->addKnob(_dopesheetKnobSectionBackgroundColor);

    _animationViewScaleColor = AppManager::createKnob<KnobColor>(thisShared, tr("Scale Text Color"), 3);
    _animationViewScaleColor->setName("animationViewScaleColor");
    _animationViewScaleColor->setSimplified(true);
    _animationColorsTab->addKnob(_animationViewScaleColor);

    _animationViewGridColor = AppManager::createKnob<KnobColor>(thisShared, tr("Grid Color"), 3);
    _animationViewGridColor->setName("animationViewGridColor");
    _animationViewGridColor->setSimplified(true);
    _animationColorsTab->addKnob(_animationViewGridColor);
}

void
SettingsPrivate::initializeKnobsNodeGraphColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _nodegraphColorsTab = AppManager::createKnob<KnobPage>( thisShared, tr("Node Graph") );
    _appearanceTab->addKnob(_nodegraphColorsTab);

    _usePluginIconsInNodeGraph = AppManager::createKnob<KnobBool>( thisShared, tr("Display plug-in icon on node-graph") );
    _usePluginIconsInNodeGraph->setName("usePluginIcons");
    _usePluginIconsInNodeGraph->setHintToolTip( tr("When checked, each node that has a plug-in icon will display it in the node-graph."
                                                   "Changing this option will not affect already existing nodes, unless a restart of Natron is made.") );
    _usePluginIconsInNodeGraph->setAddNewLine(false);
    _nodegraphColorsTab->addKnob(_usePluginIconsInNodeGraph);

    _useAntiAliasing = AppManager::createKnob<KnobBool>( thisShared, tr("Anti-Aliasing") );
    _useAntiAliasing->setName("antiAliasing");
    _useAntiAliasing->setHintToolTip( tr("When checked, the node graph will be painted using anti-aliasing. Unchecking it may increase performances."
                                         " Changing this requires a restart of Natron") );
    _nodegraphColorsTab->addKnob(_useAntiAliasing);


    _defaultNodeColor = AppManager::createKnob<KnobColor>(thisShared, tr("Default node color"), 3);
    _defaultNodeColor->setName("defaultNodeColor");
    _defaultNodeColor->setSimplified(true);
    _defaultNodeColor->setHintToolTip( tr("The default color used for newly created nodes.") );

    _nodegraphColorsTab->addKnob(_defaultNodeColor);


    _cloneColor = AppManager::createKnob<KnobColor>(thisShared, tr("Clone Color"), 3);
    _cloneColor->setName("cloneColor");
    _cloneColor->setSimplified(true);
    _cloneColor->setHintToolTip( tr("The default color used for clone nodes.") );

    _nodegraphColorsTab->addKnob(_cloneColor);



    _defaultBackdropColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Default backdrop color"), 3);
    _defaultBackdropColor->setName("backdropColor");
    _defaultBackdropColor->setSimplified(true);
    _defaultBackdropColor->setHintToolTip( tr("The default color used for newly created backdrop nodes.") );
    _nodegraphColorsTab->addKnob(_defaultBackdropColor);

    _defaultReaderColor =  AppManager::createKnob<KnobColor>(thisShared, tr(PLUGIN_GROUP_IMAGE_READERS), 3);
    _defaultReaderColor->setName("readerColor");
    _defaultReaderColor->setSimplified(true);
    _defaultReaderColor->setHintToolTip( tr("The color used for newly created Reader nodes.") );
    _nodegraphColorsTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  AppManager::createKnob<KnobColor>(thisShared, tr(PLUGIN_GROUP_IMAGE_WRITERS), 3);
    _defaultWriterColor->setName("writerColor");
    _defaultWriterColor->setSimplified(true);
    _defaultWriterColor->setHintToolTip( tr("The color used for newly created Writer nodes.") );
    _nodegraphColorsTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Generators"), 3);
    _defaultGeneratorColor->setName("generatorColor");
    _defaultGeneratorColor->setSimplified(true);
    _defaultGeneratorColor->setHintToolTip( tr("The color used for newly created Generator nodes.") );
    _nodegraphColorsTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Color group"), 3);
    _defaultColorGroupColor->setName("colorNodesColor");
    _defaultColorGroupColor->setSimplified(true);
    _defaultColorGroupColor->setHintToolTip( tr("The color used for newly created Color nodes.") );
    _nodegraphColorsTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Filter group"), 3);
    _defaultFilterGroupColor->setName("filterNodesColor");
    _defaultFilterGroupColor->setSimplified(true);
    _defaultFilterGroupColor->setHintToolTip( tr("The color used for newly created Filter nodes.") );
    _nodegraphColorsTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Transform group"), 3);
    _defaultTransformGroupColor->setName("transformNodesColor");
    _defaultTransformGroupColor->setSimplified(true);
    _defaultTransformGroupColor->setHintToolTip( tr("The color used for newly created Transform nodes.") );
    _nodegraphColorsTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Time group"), 3);
    _defaultTimeGroupColor->setName("timeNodesColor");
    _defaultTimeGroupColor->setSimplified(true);
    _defaultTimeGroupColor->setHintToolTip( tr("The color used for newly created Time nodes.") );
    _nodegraphColorsTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Draw group"), 3);
    _defaultDrawGroupColor->setName("drawNodesColor");
    _defaultDrawGroupColor->setSimplified(true);
    _defaultDrawGroupColor->setHintToolTip( tr("The color used for newly created Draw nodes.") );
    _nodegraphColorsTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Keyer group"), 3);
    _defaultKeyerGroupColor->setName("keyerNodesColor");
    _defaultKeyerGroupColor->setSimplified(true);
    _defaultKeyerGroupColor->setHintToolTip( tr("The color used for newly created Keyer nodes.") );
    _nodegraphColorsTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Channel group"), 3);
    _defaultChannelGroupColor->setName("channelNodesColor");
    _defaultChannelGroupColor->setSimplified(true);
    _defaultChannelGroupColor->setHintToolTip( tr("The color used for newly created Channel nodes.") );
    _nodegraphColorsTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Merge group"), 3);
    _defaultMergeGroupColor->setName("defaultMergeColor");
    _defaultMergeGroupColor->setSimplified(true);
    _defaultMergeGroupColor->setHintToolTip( tr("The color used for newly created Merge nodes.") );
    _nodegraphColorsTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Views group"), 3);
    _defaultViewsGroupColor->setName("defaultViewsColor");
    _defaultViewsGroupColor->setSimplified(true);
    _defaultViewsGroupColor->setHintToolTip( tr("The color used for newly created Views nodes.") );
    _nodegraphColorsTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  AppManager::createKnob<KnobColor>(thisShared, tr("Deep group"), 3);
    _defaultDeepGroupColor->setName("defaultDeepColor");
    _defaultDeepGroupColor->setSimplified(true);
    _defaultDeepGroupColor->setHintToolTip( tr("The color used for newly created Deep nodes.") );
    _nodegraphColorsTab->addKnob(_defaultDeepGroupColor);
} // Settings::initializeKnobsNodeGraphColors

void
SettingsPrivate::initializeKnobsScriptEditorColors()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _scriptEditorColorsTab = AppManager::createKnob<KnobPage>( thisShared, tr("Script Editor") );
    _scriptEditorColorsTab->setParentKnob(_appearanceTab);

    _scriptEditorFontChoice = AppManager::createKnob<KnobChoice>( thisShared, tr("Font") );
    _scriptEditorFontChoice->setHintToolTip( tr("List of all fonts available on your system") );
    _scriptEditorFontChoice->setName("scriptEditorFont");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontChoice);

    _scriptEditorFontSize = AppManager::createKnob<KnobInt>( thisShared, tr("Font Size") );
    _scriptEditorFontSize->setHintToolTip( tr("The font size") );
    _scriptEditorFontSize->setName("scriptEditorFontSize");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontSize);

    _curLineColor = AppManager::createKnob<KnobColor>(thisShared, tr("Current Line Color"), 3);
    _curLineColor->setName("currentLineColor");
    _curLineColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_curLineColor);

    _keywordColor = AppManager::createKnob<KnobColor>(thisShared, tr("Keyword Color"), 3);
    _keywordColor->setName("keywordColor");
    _keywordColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_keywordColor);

    _operatorColor = AppManager::createKnob<KnobColor>(thisShared, tr("Operator Color"), 3);
    _operatorColor->setName("operatorColor");
    _operatorColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_operatorColor);

    _braceColor = AppManager::createKnob<KnobColor>(thisShared, tr("Brace Color"), 3);
    _braceColor->setName("braceColor");
    _braceColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_braceColor);

    _defClassColor = AppManager::createKnob<KnobColor>(thisShared, tr("Class Def Color"), 3);
    _defClassColor->setName("classDefColor");
    _defClassColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_defClassColor);

    _stringsColor = AppManager::createKnob<KnobColor>(thisShared, tr("Strings Color"), 3);
    _stringsColor->setName("stringsColor");
    _stringsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_stringsColor);

    _commentsColor = AppManager::createKnob<KnobColor>(thisShared, tr("Comments Color"), 3);
    _commentsColor->setName("commentsColor");
    _commentsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_commentsColor);

    _selfColor = AppManager::createKnob<KnobColor>(thisShared, tr("Self Color"), 3);
    _selfColor->setName("selfColor");
    _selfColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_selfColor);

    _numbersColor = AppManager::createKnob<KnobColor>(thisShared, tr("Numbers Color"), 3);
    _numbersColor->setName("numbersColor");
    _numbersColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_numbersColor);
} // Settings::initializeKnobsScriptEditorColors

void
SettingsPrivate::initializeKnobsViewers()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _viewersTab = AppManager::createKnob<KnobPage>( thisShared, tr("Viewer") );

    _texturesMode = AppManager::createKnob<KnobChoice>( thisShared, tr("Viewer textures bit depth") );
    _texturesMode->setName("texturesBitDepth");
    std::vector<std::string> textureModes;
    std::vector<std::string> helpStringsTextureModes;
    textureModes.push_back("Byte");
    helpStringsTextureModes.push_back( tr("Post-processing done by the viewer (such as colorspace conversion) is done "
                                          "by the CPU. As a results, the size of cached textures is smaller.").toStdString() );
    //textureModes.push_back("16bits half-float");
    //helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back("32bits floating-point");
    helpStringsTextureModes.push_back( tr("Post-processing done by the viewer (such as colorspace conversion) is done "
                                          "by the GPU, using GLSL. As a results, the size of cached textures is larger.").toStdString() );
    _texturesMode->populateChoices(textureModes, helpStringsTextureModes);
    _texturesMode->setHintToolTip( tr("Bit depth of the viewer textures used for rendering."
                                      " Hover each option with the mouse for a detailed description.") );
    _viewersTab->addKnob(_texturesMode);

    _powerOf2Tiling = AppManager::createKnob<KnobInt>( thisShared, tr("Viewer tile size is 2 to the power of...") );
    _powerOf2Tiling->setName("viewerTiling");
    _powerOf2Tiling->setHintToolTip( tr("The dimension of the viewer tiles is 2^n by 2^n (i.e. 256 by 256 pixels for n=8). "
                                        "A high value means that the viewer renders large tiles, so that "
                                        "rendering is done less often, but on larger areas.") );
    _powerOf2Tiling->disableSlider();
    _powerOf2Tiling->setRange(4, 9);
    _powerOf2Tiling->setDisplayRange(4, 9);

    _viewersTab->addKnob(_powerOf2Tiling);

    _checkerboardTileSize = AppManager::createKnob<KnobInt>( thisShared, tr("Checkerboard tile size (pixels)") );
    _checkerboardTileSize->setName("checkerboardTileSize");
    _checkerboardTileSize->setRange(1, INT_MAX);
    _checkerboardTileSize->setHintToolTip( tr("The size (in screen pixels) of one tile of the checkerboard.") );
    _viewersTab->addKnob(_checkerboardTileSize);

    _checkerboardColor1 = AppManager::createKnob<KnobColor>(thisShared, tr("Checkerboard color 1"), 4);
    _checkerboardColor1->setName("checkerboardColor1");
    _checkerboardColor1->setHintToolTip( tr("The first color used by the checkerboard.") );
    _viewersTab->addKnob(_checkerboardColor1);

    _checkerboardColor2 = AppManager::createKnob<KnobColor>(thisShared, tr("Checkerboard color 2"), 4);
    _checkerboardColor2->setName("checkerboardColor2");
    _checkerboardColor2->setHintToolTip( tr("The second color used by the checkerboard.") );
    _viewersTab->addKnob(_checkerboardColor2);

    _autoWipe = AppManager::createKnob<KnobBool>( thisShared, tr("Automatically enable wipe") );
    _autoWipe->setName("autoWipeForViewer");
    _autoWipe->setHintToolTip( tr("When checked, the wipe tool of the viewer will be automatically enabled "
                                  "when the mouse is hovering the viewer and changing an input of a viewer." ) );
    _viewersTab->addKnob(_autoWipe);


    _autoProxyWhenScrubbingTimeline = AppManager::createKnob<KnobBool>( thisShared, tr("Automatically enable proxy when scrubbing the timeline") );
    _autoProxyWhenScrubbingTimeline->setName("autoProxyScrubbing");
    _autoProxyWhenScrubbingTimeline->setHintToolTip( tr("When checked, the proxy mode will be at least at the level "
                                                        "indicated by the auto-proxy parameter.") );
    _autoProxyWhenScrubbingTimeline->setAddNewLine(false);
    _viewersTab->addKnob(_autoProxyWhenScrubbingTimeline);


    _autoProxyLevel = AppManager::createKnob<KnobChoice>( thisShared, tr("Auto-proxy level") );
    _autoProxyLevel->setName("autoProxyLevel");
    std::vector<std::string> autoProxyChoices;
    autoProxyChoices.push_back("2");
    autoProxyChoices.push_back("4");
    autoProxyChoices.push_back("8");
    autoProxyChoices.push_back("16");
    autoProxyChoices.push_back("32");
    _autoProxyLevel->populateChoices(autoProxyChoices);
    _viewersTab->addKnob(_autoProxyLevel);

    _maximumNodeViewerUIOpened = AppManager::createKnob<KnobInt>( thisShared, tr("Max. opened node viewer interface") );
    _maximumNodeViewerUIOpened->setName("maxNodeUiOpened");
    _maximumNodeViewerUIOpened->setRange(1, INT_MAX);
    _maximumNodeViewerUIOpened->disableSlider();
    _maximumNodeViewerUIOpened->setHintToolTip( tr("Controls the maximum amount of nodes that can have their interface showing up at the same time in the viewer") );
    _viewersTab->addKnob(_maximumNodeViewerUIOpened);

    _viewerKeys = AppManager::createKnob<KnobBool>( thisShared, tr("Use number keys for the viewer") );
    _viewerKeys->setName("viewerNumberKeys");
    _viewerKeys->setHintToolTip( tr("When enabled, the row of number keys on the keyboard "
                                    "is used for switching input (<key> connects input to A side, "
                                    "<shift-key> connects input to B side), even if the corresponding "
                                    "character in the current keyboard layout is not a number.\n"
                                    "This may have to be disabled when using a remote display connection "
                                    "to Linux from a different OS.") );
    _viewersTab->addKnob(_viewerKeys);
} // Settings::initializeKnobsViewers

void
SettingsPrivate::initializeKnobsNodeGraph()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    /////////// Nodegraph tab
    _nodegraphTab = AppManager::createKnob<KnobPage>( thisShared, tr("Nodegraph") );

    _autoScroll = AppManager::createKnob<KnobBool>( thisShared, tr("Auto Scroll") );
    _autoScroll->setName("autoScroll");
    _autoScroll->setHintToolTip( tr("When checked the node graph will auto scroll if you move a node outside the current graph view.") );
    _nodegraphTab->addKnob(_autoScroll);

    _autoTurbo = AppManager::createKnob<KnobBool>( thisShared, tr("Auto-turbo") );
    _autoTurbo->setName("autoTurbo");
    _autoTurbo->setHintToolTip( tr("When checked the Turbo-mode will be enabled automatically when playback is started and disabled "
                                   "when finished.") );
    _nodegraphTab->addKnob(_autoTurbo);

    _snapNodesToConnections = AppManager::createKnob<KnobBool>( thisShared, tr("Snap to node") );
    _snapNodesToConnections->setName("enableSnapToNode");
    _snapNodesToConnections->setHintToolTip( tr("When moving nodes on the node graph, snap to positions where they are lined up "
                                                "with the inputs and output nodes.") );
    _nodegraphTab->addKnob(_snapNodesToConnections);


    _maxUndoRedoNodeGraph = AppManager::createKnob<KnobInt>( thisShared, tr("Maximum undo/redo for the node graph") );

    _maxUndoRedoNodeGraph->setName("maxUndoRedo");
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setRange(0, INT_MAX);
    _maxUndoRedoNodeGraph->setHintToolTip( tr("Set the maximum of events related to the node graph %1 "
                                              "remembers. Past this limit, older events will be deleted forever, "
                                              "allowing to re-use the RAM for other purposes. \n"
                                              "Changing this value will clear the undo/redo stack.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = AppManager::createKnob<KnobInt>( thisShared, tr("Disconnected arrow length") );
    _disconnectedArrowLength->setName("disconnectedArrowLength");
    _disconnectedArrowLength->setHintToolTip( tr("The size of a disconnected node input arrow in pixels.") );
    _disconnectedArrowLength->disableSlider();

    _nodegraphTab->addKnob(_disconnectedArrowLength);

    _hideOptionalInputsAutomatically = AppManager::createKnob<KnobBool>( thisShared, tr("Auto hide masks inputs") );
    _hideOptionalInputsAutomatically->setName("autoHideInputs");
    _hideOptionalInputsAutomatically->setHintToolTip( tr("When checked, any diconnected mask input of a node in the nodegraph "
                                                         "will be visible only when the mouse is hovering the node or when it is "
                                                         "selected.") );
    _nodegraphTab->addKnob(_hideOptionalInputsAutomatically);

    _useInputAForMergeAutoConnect = AppManager::createKnob<KnobBool>( thisShared, tr("Merge node connect to A input") );
    _useInputAForMergeAutoConnect->setName("mergeConnectToA");
    _useInputAForMergeAutoConnect->setHintToolTip( tr("If checked, upon creation of a new Merge node, the input A will be preferred "
                                                      "for auto-connection and when disabling the node instead of the input B. "
                                                      "This also applies to any other node with inputs named A and B.") );
    _nodegraphTab->addKnob(_useInputAForMergeAutoConnect);
} // Settings::initializeKnobsNodeGraph

void
SettingsPrivate::initializeKnobsCaching()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    /////////// Caching tab
    _cachingTab = AppManager::createKnob<KnobPage>( thisShared, tr("Caching") );

    _aggressiveCaching = AppManager::createKnob<KnobBool>( thisShared, tr("Aggressive caching") );
    _aggressiveCaching->setName("aggressiveCaching");
    _aggressiveCaching->setHintToolTip( tr("When checked, %1 will cache the output of all images "
                                           "rendered by all nodes, regardless of their \"Force caching\" parameter. When enabling this option "
                                           "you need to have at least 8GiB of RAM, and 16GiB is recommended.\n"
                                           "If not checked, %1 will only cache the  nodes "
                                           "which have multiple outputs, or their parameter \"Force caching\" checked or if one of its "
                                           "output has its settings panel opened.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_aggressiveCaching);

    _maxRAMPercent = AppManager::createKnob<KnobInt>( thisShared, tr("Maximum amount of RAM memory used for caching (% of total RAM)") );
    _maxRAMPercent->setName("maxRAMPercent");
    _maxRAMPercent->disableSlider();
    _maxRAMPercent->setRange(0, 100);
    QString ramHint( tr("This setting indicates the percentage of the total RAM which can be used by the memory caches. "
                        "This system has %1 of RAM.").arg( printAsRAM( getSystemTotalRAM() ) ) );
    if ( isApplication32Bits() && (getSystemTotalRAM() > 4ULL * 1024ULL * 1024ULL * 1024ULL) ) {
        ramHint.append( QString::fromUtf8("\n") );
        ramHint.append( tr("The version of %1 you are running is 32 bits, which means the available RAM "
                           "is limited to 4GiB. The amount of RAM used for caching is 4GiB * MaxRamPercent.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    }

    _maxRAMPercent->setHintToolTip(ramHint);
    _maxRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_maxRAMPercent);

    _maxRAMLabel = AppManager::createKnob<KnobString>( thisShared, std::string() );
    _maxRAMLabel->setName("maxRamLabel");
    _maxRAMLabel->setIsPersistent(false);
    _maxRAMLabel->setAsLabel();
    _cachingTab->addKnob(_maxRAMLabel);


    _unreachableRAMPercent = AppManager::createKnob<KnobInt>( thisShared, tr("System RAM to keep free (% of total RAM)") );
    _unreachableRAMPercent->setName("unreachableRAMPercent");
    _unreachableRAMPercent->disableSlider();
    _unreachableRAMPercent->setRange(0, 90);
    _unreachableRAMPercent->setHintToolTip(tr("This determines how much RAM should be kept free for other applications "
                                              "running on the same system. "
                                              "When this limit is reached, the caches start recycling memory instead of growing. "
                                              //"A reasonable value should be set for it allowing the caches to stay in physical RAM " // users don't understand what swap is
                                              //"and avoid being swapped-out on disk. "
                                              "This value should reflect the amount of memory "
                                              "you want to keep available on your computer for other usage. "
                                              "A low value may result in a massive slowdown and high disk usage.")
                                           );
    _unreachableRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_unreachableRAMPercent);
    _unreachableRAMLabel = AppManager::createKnob<KnobString>( thisShared, std::string() );
    _unreachableRAMLabel->setName("unreachableRAMLabel");
    _unreachableRAMLabel->setIsPersistent(false);
    _unreachableRAMLabel->setAsLabel();
    _cachingTab->addKnob(_unreachableRAMLabel);

    _maxViewerDiskCacheGB = AppManager::createKnob<KnobInt>( thisShared, tr("Maximum playback disk cache size (GiB)") );
    _maxViewerDiskCacheGB->setName("maxViewerDiskCache");
    _maxViewerDiskCacheGB->disableSlider();
    _maxViewerDiskCacheGB->setRange(0, 100);
    _maxViewerDiskCacheGB->setHintToolTip( tr("The maximum size that may be used by the playback cache on disk (in GiB)") );
    _cachingTab->addKnob(_maxViewerDiskCacheGB);

    _maxDiskCacheNodeGB = AppManager::createKnob<KnobInt>( thisShared, tr("Maximum DiskCache node disk usage (GiB)") );
    _maxDiskCacheNodeGB->setName("maxDiskCacheNode");
    _maxDiskCacheNodeGB->disableSlider();
    _maxDiskCacheNodeGB->setRange(0, 100);
    _maxDiskCacheNodeGB->setHintToolTip( tr("The maximum size that may be used by the DiskCache node on disk (in GiB)") );
    _cachingTab->addKnob(_maxDiskCacheNodeGB);


    _diskCachePath = AppManager::createKnob<KnobPath>( thisShared, tr("Disk cache path (empty = default)") );
    _diskCachePath->setName("diskCachePath");
    _diskCachePath->setMultiPath(false);

    QString defaultLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    QString diskCacheTt( tr("WARNING: Changing this parameter requires a restart of the application. \n"
                            "This is points to the location where %1 on-disk caches will be. "
                            "This variable should point to your fastest disk. If the parameter is left empty or the location set is invalid, "
                            "the default location will be used. The default location is: \n").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );

    _diskCachePath->setHintToolTip( diskCacheTt + defaultLocation );
    _cachingTab->addKnob(_diskCachePath);

    _wipeDiskCache = AppManager::createKnob<KnobButton>( thisShared, tr("Wipe Disk Cache") );
    _wipeDiskCache->setHintToolTip( tr("Cleans-up all caches, deleting all folders that may contain cached data. "
                                       "This is provided in case %1 lost track of cached images "
                                       "for some reason.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_wipeDiskCache);
} // Settings::initializeKnobsCaching

void
SettingsPrivate::initializeKnobsPlugins()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _pluginsTab = AppManager::createKnob<KnobPage>( thisShared, tr("Plug-ins") );
    _pluginsTab->setName("plugins");

#if defined(__linux__) || defined(__FreeBSD__)
    std::string searchPath("/usr/OFX/Plugins");
#elif defined(__APPLE__)
    std::string searchPath("/Library/OFX/Plugins");
#elif defined(WINDOWS)

    std::wstring basePath = std::wstring( OFX::Host::PluginCache::getStdOFXPluginPath() );
    basePath.append( std::wstring(L" and C:\\Program Files\\Common Files\\OFX\\Plugins") );
    std::string searchPath = StrUtils::utf16_to_utf8(basePath);

#endif

    _loadBundledPlugins = AppManager::createKnob<KnobBool>( thisShared, tr("Use bundled plug-ins") );
    knobsRequiringRestart.insert(_loadBundledPlugins);
    _loadBundledPlugins->setName("useBundledPlugins");
    _loadBundledPlugins->setHintToolTip( tr("When checked, %1 also uses the plug-ins bundled "
                                            "with the binary distribution.\n"
                                            "When unchecked, only system-wide plug-ins found in are loaded (more information can be "
                                            "found in the help for the \"Extra plug-ins search paths\" setting).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = AppManager::createKnob<KnobBool>( thisShared, tr("Prefer bundled plug-ins over system-wide plug-ins") );
    knobsRequiringRestart.insert(_preferBundledPlugins);
    _preferBundledPlugins->setName("preferBundledPlugins");
    _preferBundledPlugins->setHintToolTip( tr("When checked, and if \"Use bundled plug-ins\" is also checked, plug-ins bundled with the %1 binary distribution will take precedence over system-wide plug-ins "
                                              "if they have the same internal ID.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_preferBundledPlugins);

    _useStdOFXPluginsLocation = AppManager::createKnob<KnobBool>( thisShared, tr("Enable default OpenFX plugins location") );
    knobsRequiringRestart.insert(_useStdOFXPluginsLocation);
    _useStdOFXPluginsLocation->setName("useStdOFXPluginsLocation");
    _useStdOFXPluginsLocation->setHintToolTip( tr("When checked, %1 also uses the OpenFX plug-ins found in the default location (%2).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _pluginsTab->addKnob(_useStdOFXPluginsLocation);

    _extraPluginPaths = AppManager::createKnob<KnobPath>( thisShared, tr("OpenFX plug-ins search path") );
    knobsRequiringRestart.insert(_extraPluginPaths);
    _extraPluginPaths->setName("extraPluginsSearchPaths");
    _extraPluginPaths->setHintToolTip( tr("Extra search paths where %1 should scan for OpenFX plug-ins. "
                                          "Extra plug-ins search paths can also be specified using the OFX_PLUGIN_PATH environment variable.\n"
                                          "The priority order for system-wide plug-ins, from high to low, is:\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is checked)\n"
                                          "- plug-ins found in OFX_PLUGIN_PATH\n"
                                          "- plug-ins found in %2 (if \"Enable default OpenFX plug-ins location\" is checked)\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is not checked)\n"
                                          "Any change will take effect on the next launch of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _extraPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_extraPluginPaths);

    _templatesPluginPaths = AppManager::createKnob<KnobPath>( thisShared, tr("PyPlugs search path") );
    _templatesPluginPaths->setName("groupPluginsSearchPath");
    knobsRequiringRestart.insert(_templatesPluginPaths);
    _templatesPluginPaths->setHintToolTip( tr("Search path where %1 should scan for Python group scripts (PyPlugs). "
                                              "The search paths for groups can also be specified using the "
                                              "NATRON_PLUGIN_PATH environment variable.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _templatesPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_templatesPluginPaths);

} // Settings::initializeKnobsPlugins

void
SettingsPrivate::initializeKnobsPython()
{
    KnobHolderPtr thisShared = _publicInterface->shared_from_this();
    _pythonPage = AppManager::createKnob<KnobPage>( thisShared, tr("Python") );


    _onProjectCreated = AppManager::createKnob<KnobString>( thisShared, tr("After project created") );
    _onProjectCreated->setName("afterProjectCreated");
    _onProjectCreated->setHintToolTip( tr("Callback called once a new project is created (this is never called "
                                          "when \"After project loaded\" is called.)\n"
                                          "The signature of the callback is : callback(app) where:\n"
                                          "- app: points to the current application instance\n") );
    _pythonPage->addKnob(_onProjectCreated);


    _defaultOnProjectLoaded = AppManager::createKnob<KnobString>( thisShared, tr("Default after project loaded") );
    _defaultOnProjectLoaded->setName("defOnProjectLoaded");
    _defaultOnProjectLoaded->setHintToolTip( tr("The default afterProjectLoad callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectLoaded);

    _defaultOnProjectSave = AppManager::createKnob<KnobString>( thisShared, tr("Default before project save") );
    _defaultOnProjectSave->setName("defOnProjectSave");
    _defaultOnProjectSave->setHintToolTip( tr("The default beforeProjectSave callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectSave);


    _defaultOnProjectClose = AppManager::createKnob<KnobString>( thisShared, tr("Default before project close") );
    _defaultOnProjectClose->setName("defOnProjectClose");
    _defaultOnProjectClose->setHintToolTip( tr("The default beforeProjectClose callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnProjectClose);


    _defaultOnNodeCreated = AppManager::createKnob<KnobString>( thisShared, tr("Default after node created") );
    _defaultOnNodeCreated->setName("defOnNodeCreated");
    _defaultOnNodeCreated->setHintToolTip( tr("The default afterNodeCreated callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeCreated);


    _defaultOnNodeDelete = AppManager::createKnob<KnobString>( thisShared, tr("Default before node removal") );
    _defaultOnNodeDelete->setName("defOnNodeDelete");
    _defaultOnNodeDelete->setHintToolTip( tr("The default beforeNodeRemoval callback that will be set for new projects.") );
    _pythonPage->addKnob(_defaultOnNodeDelete);


    _echoVariableDeclarationToPython = AppManager::createKnob<KnobBool>( thisShared, tr("Print auto-declared variables in the Script Editor") );
    _echoVariableDeclarationToPython->setName("printAutoDeclaredVars");
    _echoVariableDeclarationToPython->setHintToolTip( tr("When checked, %1 will print in the Script Editor all variables that are "
                                                         "automatically declared, such as the app variable or node attributes.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pythonPage->addKnob(_echoVariableDeclarationToPython);
} // initializeKnobs

void
SettingsPrivate::setCachingLabels()
{
    int maxTotalRam = _maxRAMPercent->getValue();
    U64 systemTotalRam = getSystemTotalRAM();
    U64 maxRAM = (U64)( ( (double)maxTotalRam / 100. ) * systemTotalRam );

    _maxRAMLabel->setValue( printAsRAM(maxRAM).toStdString() );
    _unreachableRAMLabel->setValue( printAsRAM( (double)systemTotalRam * ( (double)_unreachableRAMPercent->getValue() / 100. ) ).toStdString() );
}

void
SettingsPrivate::setDefaultValues()
{
    _publicInterface->beginChanges();

    _natronSettingsExist->setDefaultValue(false);

    // General
    _checkForUpdates->setDefaultValue(false);
    _enableCrashReports->setDefaultValue(true);
    _autoSaveUnSavedProjects->setDefaultValue(true);
    _autoSaveDelay->setDefaultValue(5);
    _hostName->setDefaultValue(0);
    _customHostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);

    // General/Threading
    _numberOfThreads->setDefaultValue(0);
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders->setDefaultValue(0);
#endif
    _useThreadPool->setDefaultValue(true);
    _nThreadsPerEffect->setDefaultValue(0);
    _renderInSeparateProcess->setDefaultValue(false);
    _queueRenders->setDefaultValue(false);

    // General/Rendering
    _convertNaNValues->setDefaultValue(true);
    _pluginUseImageCopyForSource->setDefaultValue(false);
    _activateRGBSupport->setDefaultValue(true);
    _activateTransformConcatenationSupport->setDefaultValue(true);

    // General/GPU rendering
    //_openglRendererString
    _osmesaRenderers->setDefaultValue(defaultMesaDriver);
    _nOpenGLContexts->setDefaultValue(2);
#if NATRON_VERSION_MAJOR < 2 || (NATRON_VERSION_MAJOR == 2 && NATRON_VERSION_MINOR < 2)
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabled);
#else
#  if NATRON_VERSION_MAJOR >= 3
    _enableOpenGL->setDefaultValue((int)Settings::eEnableOpenGLEnabled);
#  else
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabledIfBackground);
#  endif
#endif

    // General/Projects setup
    _firstReadSetProjectFormat->setDefaultValue(true);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true);
    _fixPathsOnProjectPathChanged->setDefaultValue(true);
    //_enableMappingFromDriveLettersToUNCShareNames

    // General/Documentation
    _wwwServerPort->setDefaultValue(0);
#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource->setDefaultValue(0);
#endif

    // General/User Interface
    _notifyOnFileChange->setDefaultValue(true);
#ifdef NATRON_ENABLE_IO_META_NODES
    //_filedialogForWriters
#endif
    _renderOnEditingFinished->setDefaultValue(false);
    _linearPickers->setDefaultValue(true);
    _maxPanelsOpened->setDefaultValue(10);
    _useCursorPositionIncrements->setDefaultValue(true);
    //_defaultLayoutFile
    _loadProjectsWorkspace->setDefaultValue(false);

    // Color-Management
    //_ocioConfigKnob
    _ocioStartupCheck->setDefaultValue(true);
    //_customOcioConfigFile

    // Caching
    _aggressiveCaching->setDefaultValue(false);
    _maxRAMPercent->setDefaultValue(50);
    _unreachableRAMPercent->setDefaultValue(5);
    _maxViewerDiskCacheGB->setDefaultValue(5);
    _maxDiskCacheNodeGB->setDefaultValue(10);
    //_diskCachePath
    setCachingLabels();

    // Viewer
    _texturesMode->setDefaultValue(0);
    _powerOf2Tiling->setDefaultValue(8);
    _checkerboardTileSize->setDefaultValue(5);
    _checkerboardColor1->setDefaultValue(0.5);
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(1));
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(2));
    _checkerboardColor1->setDefaultValue(0.5, DimIdx(3));
    _checkerboardColor2->setDefaultValue(0.);
    _checkerboardColor2->setDefaultValue(0., DimIdx(1));
    _checkerboardColor2->setDefaultValue(0., DimIdx(2));
    _checkerboardColor2->setDefaultValue(0., DimIdx(3));
    _autoWipe->setDefaultValue(true);
    _autoProxyWhenScrubbingTimeline->setDefaultValue(true);
    _autoProxyLevel->setDefaultValue(1);
    _maximumNodeViewerUIOpened->setDefaultValue(2);
    _viewerKeys->setDefaultValue(true);

    // Nodegraph
    _autoScroll->setDefaultValue(false);
    _autoTurbo->setDefaultValue(false);
    _snapNodesToConnections->setDefaultValue(true);
    _useBWIcons->setDefaultValue(false);
    _maxUndoRedoNodeGraph->setDefaultValue(20);
    _disconnectedArrowLength->setDefaultValue(30);
    _hideOptionalInputsAutomatically->setDefaultValue(true);
    _useInputAForMergeAutoConnect->setDefaultValue(false);
    _usePluginIconsInNodeGraph->setDefaultValue(true);
    _useAntiAliasing->setDefaultValue(true);

    // Plugins
    _extraPluginPaths->setDefaultValue("");
    _useStdOFXPluginsLocation->setDefaultValue(true);
    //_templatesPluginPaths
    _preferBundledPlugins->setDefaultValue(true);
    _loadBundledPlugins->setDefaultValue(true);

    // Python
    //_onProjectCreated;
    //_defaultOnProjectLoaded;
    //_defaultOnProjectSave;
    //_defaultOnProjectClose;
    //_defaultOnNodeCreated;
    //_defaultOnNodeDelete;
    //_loadPyPlugsFromPythonScript;
    _echoVariableDeclarationToPython->setDefaultValue(false);

    // Appearance
    _systemFontChoice->setDefaultValue(0);
    _fontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);
    //_qssFile
    //_defaultAppearanceVersion

    // Appearance/Main Window
    _sunkenColor->setDefaultValue(0.12, DimIdx(0));
    _sunkenColor->setDefaultValue(0.12, DimIdx(1));
    _sunkenColor->setDefaultValue(0.12, DimIdx(2));
    _baseColor->setDefaultValue(0.19, DimIdx(0));
    _baseColor->setDefaultValue(0.19, DimIdx(1));
    _baseColor->setDefaultValue(0.19, DimIdx(2));
    _raisedColor->setDefaultValue(0.28, DimIdx(0));
    _raisedColor->setDefaultValue(0.28, DimIdx(1));
    _raisedColor->setDefaultValue(0.28, DimIdx(2));
    _selectionColor->setDefaultValue(0.95, DimIdx(0));
    _selectionColor->setDefaultValue(0.54, DimIdx(1));
    _selectionColor->setDefaultValue(0., DimIdx(2));
    _textColor->setDefaultValue(0.78, DimIdx(0));
    _textColor->setDefaultValue(0.78, DimIdx(1));
    _textColor->setDefaultValue(0.78, DimIdx(2));
    _altTextColor->setDefaultValue(0.6, DimIdx(0));
    _altTextColor->setDefaultValue(0.6, DimIdx(1));
    _altTextColor->setDefaultValue(0.6, DimIdx(2));
    _timelinePlayheadColor->setDefaultValue(0.95, DimIdx(0));
    _timelinePlayheadColor->setDefaultValue(0.54, DimIdx(1));
    _timelinePlayheadColor->setDefaultValue(0., DimIdx(2));
    _timelineBGColor->setDefaultValue(0, DimIdx(0));
    _timelineBGColor->setDefaultValue(0, DimIdx(1));
    _timelineBGColor->setDefaultValue(0., DimIdx(2));
    _timelineBoundsColor->setDefaultValue(0.81, DimIdx(0));
    _timelineBoundsColor->setDefaultValue(0.27, DimIdx(1));
    _timelineBoundsColor->setDefaultValue(0.02, DimIdx(2));
    _interpolatedColor->setDefaultValue(0.34, DimIdx(0));
    _interpolatedColor->setDefaultValue(0.46, DimIdx(1));
    _interpolatedColor->setDefaultValue(0.6, DimIdx(2));
    _keyframeColor->setDefaultValue(0.08, DimIdx(0));
    _keyframeColor->setDefaultValue(0.38, DimIdx(1));
    _keyframeColor->setDefaultValue(0.97, DimIdx(2));
    _trackerKeyframeColor->setDefaultValue(0.7, DimIdx(0));
    _trackerKeyframeColor->setDefaultValue(0.78, DimIdx(1));
    _trackerKeyframeColor->setDefaultValue(0.39, DimIdx(2));
    _exprColor->setDefaultValue(0.7, DimIdx(0));
    _exprColor->setDefaultValue(0.78, DimIdx(1));
    _exprColor->setDefaultValue(0.39, DimIdx(2));
    _cachedFrameColor->setDefaultValue(0.56, DimIdx(0));
    _cachedFrameColor->setDefaultValue(0.79, DimIdx(1));
    _cachedFrameColor->setDefaultValue(0.4, DimIdx(2));
    _diskCachedFrameColor->setDefaultValue(0.27, DimIdx(0));
    _diskCachedFrameColor->setDefaultValue(0.38, DimIdx(1));
    _diskCachedFrameColor->setDefaultValue(0.25, DimIdx(2));
    _sliderColor->setDefaultValue(0.33, DimIdx(0));
    _sliderColor->setDefaultValue(0.45, DimIdx(1));
    _sliderColor->setDefaultValue(0.44, DimIdx(2));

    // Appearance/Dope Sheet
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(0));
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(1));
    _animationViewBackgroundColor->setDefaultValue(0.19, DimIdx(2));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(0));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(1));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.204, DimIdx(2));
    _dopesheetRootSectionBackgroundColor->setDefaultValue(0.2, DimIdx(3));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(0));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(1));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.443, DimIdx(2));
    _dopesheetKnobSectionBackgroundColor->setDefaultValue(0.2, DimIdx(2));
    _animationViewScaleColor->setDefaultValue(0.714, DimIdx(0));
    _animationViewScaleColor->setDefaultValue(0.718, DimIdx(1));
    _animationViewScaleColor->setDefaultValue(0.714, DimIdx(2));
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(0));
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(1));
    _animationViewGridColor->setDefaultValue(0.35, DimIdx(2));

    // Appearance/Script Editor
    _curLineColor->setDefaultValue(0.35, DimIdx(0));
    _curLineColor->setDefaultValue(0.35, DimIdx(1));
    _curLineColor->setDefaultValue(0.35, DimIdx(2));
    _keywordColor->setDefaultValue(0.7, DimIdx(0));
    _keywordColor->setDefaultValue(0.7, DimIdx(1));
    _keywordColor->setDefaultValue(0., DimIdx(2));
    _operatorColor->setDefaultValue(0.78, DimIdx(0));
    _operatorColor->setDefaultValue(0.78, DimIdx(1));
    _operatorColor->setDefaultValue(0.78, DimIdx(2));
    _braceColor->setDefaultValue(0.85, DimIdx(0));
    _braceColor->setDefaultValue(0.85, DimIdx(1));
    _braceColor->setDefaultValue(0.85, DimIdx(2));
    _defClassColor->setDefaultValue(0.7, DimIdx(0));
    _defClassColor->setDefaultValue(0.7, DimIdx(1));
    _defClassColor->setDefaultValue(0., DimIdx(2));
    _stringsColor->setDefaultValue(0.8, DimIdx(0));
    _stringsColor->setDefaultValue(0.2, DimIdx(1));
    _stringsColor->setDefaultValue(0., DimIdx(2));
    _commentsColor->setDefaultValue(0.25, DimIdx(0));
    _commentsColor->setDefaultValue(0.6, DimIdx(1));
    _commentsColor->setDefaultValue(0.25, DimIdx(2));
    _selfColor->setDefaultValue(0.7, DimIdx(0));
    _selfColor->setDefaultValue(0.7, DimIdx(1));
    _selfColor->setDefaultValue(0., DimIdx(2));
    _numbersColor->setDefaultValue(0.25, DimIdx(0));
    _numbersColor->setDefaultValue(0.8, DimIdx(1));
    _numbersColor->setDefaultValue(0.9, DimIdx(2));
    _scriptEditorFontChoice->setDefaultValue(0);
    _scriptEditorFontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);


    // Appearance/Node Graph
    _cloneColor->setDefaultValue(0.78, DimIdx(0));
    _cloneColor->setDefaultValue(0.27, DimIdx(1));
    _cloneColor->setDefaultValue(0.39, DimIdx(2));
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(0));
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(1));
    _defaultNodeColor->setDefaultValue(0.7, DimIdx(2));
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(0));
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(1));
    _defaultBackdropColor->setDefaultValue(0.45, DimIdx(2));
    _defaultGeneratorColor->setDefaultValue(0.3, DimIdx(0));
    _defaultGeneratorColor->setDefaultValue(0.5, DimIdx(1));
    _defaultGeneratorColor->setDefaultValue(0.2, DimIdx(2));
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(0));
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(1));
    _defaultReaderColor->setDefaultValue(0.7, DimIdx(2));
    _defaultWriterColor->setDefaultValue(0.75, DimIdx(0));
    _defaultWriterColor->setDefaultValue(0.75, DimIdx(1));
    _defaultWriterColor->setDefaultValue(0., DimIdx(2));
    _defaultColorGroupColor->setDefaultValue(0.48, DimIdx(0));
    _defaultColorGroupColor->setDefaultValue(0.66, DimIdx(1));
    _defaultColorGroupColor->setDefaultValue(1., DimIdx(2));
    _defaultFilterGroupColor->setDefaultValue(0.8, DimIdx(0));
    _defaultFilterGroupColor->setDefaultValue(0.5, DimIdx(1));
    _defaultFilterGroupColor->setDefaultValue(0.3, DimIdx(2));
    _defaultTransformGroupColor->setDefaultValue(0.7, DimIdx(0));
    _defaultTransformGroupColor->setDefaultValue(0.3, DimIdx(1));
    _defaultTransformGroupColor->setDefaultValue(0.1, DimIdx(2));
    _defaultTimeGroupColor->setDefaultValue(0.7, DimIdx(0));
    _defaultTimeGroupColor->setDefaultValue(0.65, DimIdx(1));
    _defaultTimeGroupColor->setDefaultValue(0.35, DimIdx(2));
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(0));
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(1));
    _defaultDrawGroupColor->setDefaultValue(0.75, DimIdx(2));
    _defaultKeyerGroupColor->setDefaultValue(0., DimIdx(0));
    _defaultKeyerGroupColor->setDefaultValue(1, DimIdx(1));
    _defaultKeyerGroupColor->setDefaultValue(0., DimIdx(2));
    _defaultChannelGroupColor->setDefaultValue(0.6, DimIdx(0));
    _defaultChannelGroupColor->setDefaultValue(0.24, DimIdx(1));
    _defaultChannelGroupColor->setDefaultValue(0.39, DimIdx(2));
    _defaultMergeGroupColor->setDefaultValue(0.3, DimIdx(0));
    _defaultMergeGroupColor->setDefaultValue(0.37, DimIdx(1));
    _defaultMergeGroupColor->setDefaultValue(0.776, DimIdx(2));
    _defaultViewsGroupColor->setDefaultValue(0.5, DimIdx(0));
    _defaultViewsGroupColor->setDefaultValue(0.9, DimIdx(1));
    _defaultViewsGroupColor->setDefaultValue(0.7, DimIdx(2));
    _defaultDeepGroupColor->setDefaultValue(0., DimIdx(0));
    _defaultDeepGroupColor->setDefaultValue(0., DimIdx(1));
    _defaultDeepGroupColor->setDefaultValue(0.38, DimIdx(2));


    _publicInterface->endChanges();
} // setDefaultValues


void
Settings::saveAllSettings()
{
    const KnobsVec &knobs = getKnobs();
    KnobsVec k( knobs.size() );

    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i];
    }
    saveSettings(k, true);
}

void
Settings::restorePluginSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        if ( it->first.empty() ) {
            continue;
        }
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            PluginPtr plugin = *itver;
            assert(plugin);

            if ( plugin->getProperty<bool>(kNatronPluginPropIsInternalOnly) ) {
                continue;
            }


            {
                QString pluginIDKey = QString::fromUtf8(plugin->getPluginID().c_str()) + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
                QString enabledKey = pluginIDKey + QString::fromUtf8("_enabled");
                if ( settings.contains(enabledKey) ) {
                    bool enabled = settings.value(enabledKey).toBool();
                    plugin->setEnabled(enabled);
                } else {
                    settings.setValue( enabledKey, plugin->isMultiThreadingEnabled() );
                }

                QString rsKey = pluginIDKey + QString::fromUtf8("_rs");
                if ( settings.contains(rsKey) ) {
                    bool renderScaleEnabled = settings.value(rsKey).toBool();
                    plugin->setRenderScaleEnabled(renderScaleEnabled);
                } else {
                    settings.setValue( rsKey, plugin->isRenderScaleEnabled() );
                }

                QString mtKey = pluginIDKey + QString::fromUtf8("_mt");
                if ( settings.contains(mtKey) ) {
                    bool multiThreadingEnabled = settings.value(mtKey).toBool();
                    plugin->setMultiThreadingEnabled(multiThreadingEnabled);
                } else {
                    settings.setValue( mtKey, plugin->isMultiThreadingEnabled() );
                }

                QString glKey = pluginIDKey + QString::fromUtf8("_gl");
                if (settings.contains(glKey)) {
                    bool openglEnabled = settings.value(glKey).toBool();
                    plugin->setOpenGLEnabled(openglEnabled);
                } else {
                    settings.setValue(glKey, plugin->isOpenGLEnabled());
                }

            }
        }
    }
} // Settings::restorePluginSettings

void
Settings::savePluginsSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            PluginPtr plugin = *itver;
            assert(plugin);

            QString pluginID = QString::fromUtf8(plugin->getPluginID().c_str()) + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
            QString enabledKey = pluginID + QString::fromUtf8("_enabled");
            settings.setValue( enabledKey, plugin->isEnabled() );

            QString rsKey = pluginID + QString::fromUtf8("_rs");
            settings.setValue( rsKey, plugin->isRenderScaleEnabled() );

            QString mtKey = pluginID + QString::fromUtf8("_mt");
            settings.setValue(mtKey, plugin->isMultiThreadingEnabled());

            QString glKey = pluginID + QString::fromUtf8("_gl");
            settings.setValue(glKey, plugin->isOpenGLEnabled());

        }
    }
}

void
Settings::saveSetting(const KnobIPtr& knob)
{
    KnobsVec knobs;

    knobs.push_back(knob);
    saveSettings(knobs, false /*savePluginSettings*/);
}

void
Settings::saveSettings(const KnobsVec& knobs,
                       bool pluginSettings)
{
    if (pluginSettings) {
        savePluginsSettings();
    }
    KnobsVec changedKnobs;
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    settings.setValue(QString::fromUtf8(kQSettingsSoftwareMajorVersionSettingName), NATRON_VERSION_MAJOR);
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBasePtr isString = toKnobStringBase(knobs[i]);
        KnobIntBasePtr isInt = toKnobIntBase(knobs[i]);
        KnobChoicePtr isChoice = toKnobChoice(knobs[i]);
        KnobDoubleBasePtr isDouble = toKnobDoubleBase(knobs[i]);
        KnobBoolBasePtr isBool = toKnobBoolBase(knobs[i]);

        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getNDimensions(); ++j) {
            QString dimensionName;
            if (knobs[i]->getNDimensions() > 1) {
                dimensionName =  QString::fromUtf8( name.c_str() ) + QLatin1Char('.') + QString::fromUtf8( knobs[i]->getDimensionName(DimIdx(j)).c_str() );
            } else {
                dimensionName = QString::fromUtf8( name.c_str() );
            }
            try {
                if (isString) {
                    QString old = settings.value(dimensionName).toString();
                    QString newValue = QString::fromUtf8( isString->getValue(DimIdx(j)).c_str() );
                    if (old != newValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        int newIndex = isChoice->getValue(DimIdx(j));
                        const std::vector<std::string> entries = isChoice->getEntries();
                        if ( newIndex < (int)entries.size() ) {
                            QString oldValue = settings.value(dimensionName).toString();
                            QString newValue = QString::fromUtf8( entries[newIndex].c_str() );
                            if (oldValue != newValue) {
                                changedKnobs.push_back(knobs[i]);
                            }
                            settings.setValue( dimensionName, QVariant(newValue) );
                        }
                    } else {
                        int newValue = isInt->getValue(DimIdx(j));
                        int oldValue = settings.value( dimensionName, QVariant(INT_MIN) ).toInt();
                        if (newValue != oldValue) {
                            changedKnobs.push_back(knobs[i]);
                        }
                        settings.setValue( dimensionName, QVariant(newValue) );
                    }
                } else if (isDouble) {
                    double newValue = isDouble->getValue(DimIdx(j));
                    double oldValue = settings.value( dimensionName, QVariant(INT_MIN) ).toDouble();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isBool) {
                    bool newValue = isBool->getValue(DimIdx(j));
                    bool oldValue = settings.value(dimensionName).toBool();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else {
                    assert(false);
                }
            } catch (std::logic_error) {
                // ignore
            }
        } // for (int j = 0; j < knobs[i]->getNDimensions(); ++j) {
    } // for (U32 i = 0; i < knobs.size(); ++i) {

} // saveSettings

void
Settings::restoreSettings(const KnobsVec& knobs)
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBasePtr isString = toKnobStringBase(knobs[i]);
        KnobIntBasePtr isInt = toKnobIntBase(knobs[i]);
        KnobChoicePtr isChoice = toKnobChoice(knobs[i]);
        KnobDoubleBasePtr isDouble = toKnobDoubleBase(knobs[i]);
        KnobBoolBasePtr isBool = toKnobBoolBase(knobs[i]);

        const std::string& name = knobs[i]->getName();

        // Prevent onKnobValuechanged from being called
        knobs[i]->blockValueChanges();
        for (int j = 0; j < knobs[i]->getNDimensions(); ++j) {
            std::string dimensionName = knobs[i]->getNDimensions() > 1 ? name + '.' + knobs[i]->getDimensionName(DimIdx(j)) : name;
            QString qDimName = QString::fromUtf8( dimensionName.c_str() );

            if ( settings.contains(qDimName) ) {
                if (isString) {
                    isString->setValue(settings.value(qDimName).toString().toStdString(), ViewSetSpec::all(), DimIdx(j));
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        std::string value = settings.value(qDimName).toString().toStdString();
                        const std::vector<std::string> entries = isChoice->getEntries();
                        int found = -1;

                        for (U32 k = 0; k < entries.size(); ++k) {
                            if (entries[k] == value) {
                                found = (int)k;
                                break;
                            }
                        }

                        if (found >= 0) {
                            isChoice->setValue(found, ViewSetSpec::all(), DimIdx(j));
                        }
                    } else {
                        isInt->setValue(settings.value(qDimName).toInt(), ViewSetSpec::all(), DimIdx(j));
                    }
                } else if (isDouble) {
                    isDouble->setValue(settings.value(qDimName).toDouble(), ViewSetSpec::all(), DimIdx(j));
                } else if (isBool) {
                    isBool->setValue(settings.value(qDimName).toBool(), ViewSetSpec::all(), DimIdx(j));
                } else {
                    assert(false);
                }
            }
        }
        knobs[i]->unblockValueChanges();
    }

    _imp->_settingsExisted = false;
    try {
        _imp->_settingsExisted = _imp->_natronSettingsExist->getValue();

        if (!_imp->_settingsExisted) {
            _imp->_natronSettingsExist->setValue(true);
            saveSetting( _imp->_natronSettingsExist );
        }

        int appearanceVersion = _imp->_defaultAppearanceVersion->getValue();
        if ( _imp->_settingsExisted && (appearanceVersion < NATRON_DEFAULT_APPEARANCE_VERSION) ) {
            _imp->_defaultAppearanceOutdated = true;
            _imp->_defaultAppearanceVersion->setValue(NATRON_DEFAULT_APPEARANCE_VERSION);
            saveSetting(_imp->_defaultAppearanceVersion );
        }

        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
        appPTR->setNThreadsToRender( getNumberOfThreads() );
        appPTR->setUseThreadPool( _imp->_useThreadPool->getValue() );
        appPTR->setPluginsUseInputImageCopyToRender( _imp->_pluginUseImageCopyForSource->getValue() );
    } catch (std::logic_error) {
        // ignore
    }


} // restoreSettings

void
Settings::restoreAllSettings()
{
    _imp->_restoringSettings = true;

    const KnobsVec& knobs = getKnobs();
    restoreSettings(knobs);

    if (!_imp->_ocioRestored) {
        ///Load even though there's no settings!
        _imp->tryLoadOpenColorIOConfig();
    }

    // Restore opengl renderer
    {
        std::vector<std::string> availableRenderers = _imp->_availableOpenGLRenderers->getEntries();
        QString missingGLError;
        bool hasGL = appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering, &missingGLError);

        if ( availableRenderers.empty() || !hasGL) {
            if (missingGLError.isEmpty()) {
                _imp->_openglRendererString->setValue( tr("OpenGL rendering disabled: No device meeting %1 requirements could be found.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
            } else {
                _imp->_openglRendererString->setValue(missingGLError.toStdString());
            }
        }
        int curIndex = _imp->_availableOpenGLRenderers->getValue();
        if ( (curIndex >= 0) && ( curIndex < (int)availableRenderers.size() ) ) {
            const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
            int i = 0;
            for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
                if (i == curIndex) {
                    QString maxMemoryString = it->maxMemBytes == 0 ? tr("Unknown") : printAsRAM(it->maxMemBytes);
                    QString curRenderer = (QString::fromUtf8("<p><h2>") +
                                           tr("OpenGL Renderer Infos:") +
                                           QString::fromUtf8("</h2></p><p><b>") +
                                           tr("Vendor:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->vendorName.c_str() ) ) +
                                           tr("Renderer:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->rendererName.c_str() ) ) +
                                           tr("OpenGL Version:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glVersionString.c_str() ) ) +
                                           tr("GLSL Version:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glslVersionString.c_str() ) ) +
                                           tr("Max. Memory:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg(maxMemoryString) +
                                           tr("Max. Texture Size (px):") +
                                           QString::fromUtf8("</b> %5</p<").arg(it->maxTextureSize));
                    _imp->_openglRendererString->setValue( curRenderer.toStdString() );
                    break;
                }
            }
        }
    }

    if (!appPTR->isTextureFloatSupported()) {
        if (_imp->_texturesMode) {
            _imp->_texturesMode->setSecret(true);
        }
    }


    _imp->_restoringSettings = false;
} // restoreSettings

bool
SettingsPrivate::tryLoadOpenColorIOConfig()
{
    QString configFile;


    if ( _customOcioConfigFile->isEnabled() ) {
        ///try to load from the file
        std::string file;
        try {
            file = _customOcioConfigFile->getValue();
        } catch (...) {
            // ignore exceptions
        }

        if ( file.empty() ) {
            return false;
        }
        if ( !QFile::exists( QString::fromUtf8( file.c_str() ) ) ) {
            Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file.").arg( QString::fromUtf8( file.c_str() ) ).toStdString() );

            return false;
        }
        configFile = QString::fromUtf8( file.c_str() );
    } else {
        try {
            ///try to load from the combobox
            QString activeEntryText  = QString::fromUtf8( _ocioConfigKnob->getActiveEntryText().c_str() );
            QString configFileName = QString( activeEntryText + QString::fromUtf8(".ocio") );
            QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
            Q_FOREACH(const QString &defaultConfigsDirStr, defaultConfigsPaths) {
                QDir defaultConfigsDir(defaultConfigsDirStr);

                if ( !defaultConfigsDir.exists() ) {
                    qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory"
                             << defaultConfigsDirStr << "does not exist.";
                    continue;
                }
                ///try to open the .ocio config file first in the defaultConfigsDir
                ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
                if ( !defaultConfigsDir.exists(configFileName) ) {
                    QDir subDir(defaultConfigsDirStr + QDir::separator() + activeEntryText);
                    if ( !subDir.exists() ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file or directory.").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    if ( !subDir.exists( QString::fromUtf8("config.ocio") ) ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: No such file or directory.").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    configFile = subDir.absoluteFilePath( QString::fromUtf8("config.ocio") );
                } else {
                    configFile = defaultConfigsDir.absoluteFilePath(configFileName);
                }
            }
        } catch (...) {
            // ignore exceptions
        }

        if ( configFile.isEmpty() ) {
            return false;
        }
    }
    _ocioRestored = true;
#ifdef DEBUG
    qDebug() << "setting OCIO=" << configFile;
#endif
    std::string stdConfigFile = configFile.toStdString();
#if 0 //def __NATRON_WIN32__
    _wputenv_s(L"OCIO", StrUtils::utf8_to_utf16(stdConfigFile).c_str());
#else
    qputenv( NATRON_OCIO_ENV_VAR_NAME, stdConfigFile.c_str() );
#endif

    std::string configPath = SequenceParsing::removePath(stdConfigFile);
    if ( !configPath.empty() && (configPath[configPath.size() - 1] == '/') ) {
        configPath.erase(configPath.size() - 1, 1);
    }
    appPTR->onOCIOConfigPathChanged(configPath);

    return true;
} // tryLoadOpenColorIOConfig

inline
void
crash_application()
{
    std::cerr << "CRASHING APPLICATION NOW UPON USER REQUEST!" << std::endl;
    volatile int* a = (int*)(NULL);

    // coverity[var_deref_op]
    *a = 1;
}

bool
Settings::onKnobValueChanged(const KnobIPtr& k,
                             ValueChangedReasonEnum reason,
                             double /*time*/,
                             ViewSetSpec /*view*/)
{

    Q_EMIT settingChanged(k, reason);
    bool ret = true;

    if ( k == _imp->_maxViewerDiskCacheGB ) {
        if (!_imp->_restoringSettings) {
            appPTR->setApplicationsCachesMaximumViewerDiskSpace( getMaximumViewerDiskCacheSize() );
        }
    } else if ( k == _imp->_maxDiskCacheNodeGB ) {
        if (!_imp->_restoringSettings) {
            appPTR->setApplicationsCachesMaximumDiskSpace( getMaximumDiskCacheNodeSize() );
        }
    } else if ( k == _imp->_maxRAMPercent ) {
        if (!_imp->_restoringSettings) {
            appPTR->setApplicationsCachesMaximumMemoryPercent( getRamMaximumPercent() );
        }
        _imp->setCachingLabels();
    } else if ( k == _imp->_diskCachePath ) {
        appPTR->setDiskCacheLocation( QString::fromUtf8( _imp->_diskCachePath->getValue().c_str() ) );
    } else if ( k == _imp->_wipeDiskCache ) {
        appPTR->wipeAndCreateDiskCacheStructure();
    } else if ( k == _imp->_numberOfThreads ) {
        int nbThreads = getNumberOfThreads();
        appPTR->setNThreadsToRender(nbThreads);
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            QThreadPool::globalInstance()->setMaxThreadCount( QThread::idealThreadCount() );
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if ( k == _imp->_nThreadsPerEffect ) {
        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
    } else if ( k == _imp->_ocioConfigKnob ) {
        if (_imp->_ocioConfigKnob->getActiveEntryText() == NATRON_CUSTOM_OCIO_CONFIG_NAME) {
            _imp->_customOcioConfigFile->setEnabled(true);
        } else {
            _imp->_customOcioConfigFile->setEnabled(false);
        }
        _imp->tryLoadOpenColorIOConfig();
    } else if ( k == _imp->_useThreadPool ) {
        bool useTP = _imp->_useThreadPool->getValue();
        appPTR->setUseThreadPool(useTP);
    } else if ( k == _imp->_customOcioConfigFile ) {
        if ( _imp->_customOcioConfigFile->isEnabled() ) {
            _imp->tryLoadOpenColorIOConfig();
        }
    } else if ( k == _imp->_maxUndoRedoNodeGraph ) {
        appPTR->setUndoRedoStackLimit( _imp->_maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _imp->_maxPanelsOpened ) {
        appPTR->onMaxPanelsOpenedChanged( _imp->_maxPanelsOpened->getValue() );
    } else if ( k == _imp->_queueRenders ) {
        appPTR->onQueueRendersChanged( _imp->_queueRenders->getValue() );
    } else if ( ( k == _imp->_checkerboardTileSize ) || ( k == _imp->_checkerboardColor1 ) || ( k == _imp->_checkerboardColor2 ) ) {
        appPTR->onCheckerboardSettingsChanged();
    } else if ( k == _imp->_powerOf2Tiling && !_imp->_restoringSettings) {
        appPTR->onViewerTileCacheSizeChanged();
    } else if ( k == _imp->_texturesMode &&  !_imp->_restoringSettings) {
        appPTR->onViewerTileCacheSizeChanged();
    } else if ( ( k == _imp->_hideOptionalInputsAutomatically ) && !_imp->_restoringSettings && (reason == eValueChangedReasonUserEdited) ) {
        appPTR->toggleAutoHideGraphInputs();
    } else if ( k == _imp->_autoProxyWhenScrubbingTimeline ) {
        _imp->_autoProxyLevel->setSecret( !_imp->_autoProxyWhenScrubbingTimeline->getValue() );
    } else if ( !_imp->_restoringSettings &&
               ( ( k == _imp->_sunkenColor ) ||
                ( k == _imp->_baseColor ) ||
                ( k == _imp->_raisedColor ) ||
                ( k == _imp->_selectionColor ) ||
                ( k == _imp->_textColor ) ||
                ( k == _imp->_altTextColor ) ||
                ( k == _imp->_timelinePlayheadColor ) ||
                ( k == _imp->_timelineBoundsColor ) ||
                ( k == _imp->_timelineBGColor ) ||
                ( k == _imp->_interpolatedColor ) ||
                ( k == _imp->_keyframeColor ) ||
                ( k == _imp->_trackerKeyframeColor ) ||
                ( k == _imp->_cachedFrameColor ) ||
                ( k == _imp->_diskCachedFrameColor ) ||
                ( k == _imp->_animationViewBackgroundColor ) ||
                ( k == _imp->_animationViewGridColor ) ||
                ( k == _imp->_animationViewScaleColor ) ||
                ( k == _imp->_dopesheetRootSectionBackgroundColor ) ||
                ( k == _imp->_dopesheetKnobSectionBackgroundColor ) ||
                ( k == _imp->_keywordColor ) ||
                ( k == _imp->_operatorColor ) ||
                ( k == _imp->_curLineColor ) ||
                ( k == _imp->_braceColor ) ||
                ( k == _imp->_defClassColor ) ||
                ( k == _imp->_stringsColor ) ||
                ( k == _imp->_commentsColor ) ||
                ( k == _imp->_selfColor ) ||
                ( k == _imp->_sliderColor ) ||
                ( k == _imp->_numbersColor ) ||
                ( k == _imp->_qssFile) ) ) {
                   if (_imp->_restoringDefaults) {
                       ++_imp->_nRedrawStyleSheetRequests;
                   } else {
                       appPTR->reloadStylesheets();
                   }
               } else if ( k == _imp->_hostName ) {
                   std::string hostName = _imp->_hostName->getActiveEntryText();
                   bool isCustom = hostName == NATRON_CUSTOM_HOST_NAME_ENTRY;
                   _imp->_customHostName->setSecret(!isCustom);
               } else if ( ( k == _imp->_testCrashReportButton ) && (reason == eValueChangedReasonUserEdited) ) {
                   StandardButtonEnum reply = Dialogs::questionDialog( tr("Crash Test").toStdString(),
                                                                      tr("You are about to make %1 crash to test the reporting system.\n"
                                                                         "Do you really want to crash?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false,
                                                                      StandardButtons(eStandardButtonYes | eStandardButtonNo) );
                   if (reply == eStandardButtonYes) {
                       crash_application();
                   }
               } else if ( ( k == _imp->_scriptEditorFontChoice ) || ( k == _imp->_scriptEditorFontSize ) ) {
                   appPTR->reloadScriptEditorFonts();
               } else if ( k == _imp->_pluginUseImageCopyForSource ) {
                   appPTR->setPluginsUseInputImageCopyToRender( _imp->_pluginUseImageCopyForSource->getValue() );
               } else if ( k == _imp->_enableOpenGL ) {
                   appPTR->refreshOpenGLRenderingFlagOnAllInstances();
                   if (!_imp->_restoringSettings) {
                       appPTR->clearPluginsLoadedCache();
                   }
               } else {
                   ret = false;
               }

    return ret;
} // onKnobValueChanged

////////////////////////////////////////////////////////
// "Viewers" pane

ImageBitDepthEnum
Settings::getViewersBitDepth() const
{
    if (!appPTR->isTextureFloatSupported()) {
        return eImageBitDepthByte;
    }
    int v = _imp->_texturesMode->getValue();

    if (v == 0) {
        return eImageBitDepthByte;
    } else if (v == 1) {
        return eImageBitDepthFloat;
    } else {
        return eImageBitDepthByte;
    }
}

int
Settings::getViewerTilesPowerOf2() const
{
    return _imp->_powerOf2Tiling->getValue();
}

int
Settings::getCheckerboardTileSize() const
{
    return _imp->_checkerboardTileSize->getValue();
}

void
Settings::getCheckerboardColor1(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _imp->_checkerboardColor1->getValue(DimIdx(0));
    *g = _imp->_checkerboardColor1->getValue(DimIdx(1));
    *b = _imp->_checkerboardColor1->getValue(DimIdx(2));
    *a = _imp->_checkerboardColor1->getValue(DimIdx(3));
}

void
Settings::getCheckerboardColor2(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _imp->_checkerboardColor2->getValue(DimIdx(0));
    *g = _imp->_checkerboardColor2->getValue(DimIdx(1));
    *b = _imp->_checkerboardColor2->getValue(DimIdx(2));
    *a = _imp->_checkerboardColor2->getValue(DimIdx(3));
}

bool
Settings::isAutoWipeEnabled() const
{
    return _imp->_autoWipe->getValue();
}

bool
Settings::isAutoProxyEnabled() const
{
    return _imp->_autoProxyWhenScrubbingTimeline->getValue();
}

unsigned int
Settings::getAutoProxyMipMapLevel() const
{
    return (unsigned int)_imp->_autoProxyLevel->getValue() + 1;
}

int
Settings::getMaxOpenedNodesViewerContext() const
{
    return _imp->_maximumNodeViewerUIOpened->getValue();
}

bool
Settings::isViewerKeysEnabled() const
{
    return _imp->_viewerKeys->getValue();
}

///////////////////////////////////////////////////////
// "Caching" pane

bool
Settings::isAggressiveCachingEnabled() const
{
    return _imp->_aggressiveCaching->getValue();
}

double
Settings::getRamMaximumPercent() const
{
    return (double)_imp->_maxRAMPercent->getValue() / 100.;
}

U64
Settings::getMaximumViewerDiskCacheSize() const
{
    return (U64)( _imp->_maxViewerDiskCacheGB->getValue() ) * std::pow(1024., 3.);
}

U64
Settings::getMaximumDiskCacheNodeSize() const
{
    return (U64)( _imp->_maxDiskCacheNodeGB->getValue() ) * std::pow(1024., 3.);
}

///////////////////////////////////////////////////

double
Settings::getUnreachableRamPercent() const
{
    return (double)_imp->_unreachableRAMPercent->getValue() / 100.;
}

bool
Settings::getColorPickerLinear() const
{
    return _imp->_linearPickers->getValue();
}

int
Settings::getNumberOfThreadsPerEffect() const
{
    return _imp->_nThreadsPerEffect->getValue();
}

int
Settings::getNumberOfThreads() const
{
    return _imp->_numberOfThreads->getValue();
}

void
Settings::setNumberOfThreads(int threadsNb)
{
    _imp->_numberOfThreads->setValue(threadsNb);
}

bool
Settings::isAutoPreviewOnForNewProjects() const
{
    return _imp->_autoPreviewEnabledForNewProjects->getValue();
}

#ifdef NATRON_DOCUMENTATION_ONLINE
int
Settings::getDocumentationSource() const
{
    return _imp->_documentationSource->getValue();
}
#endif

int
Settings::getServerPort() const
{
    return _imp->_wwwServerPort->getValue();
}

void
Settings::setServerPort(int port) const
{
    _imp->_wwwServerPort->setValue(port);
}

bool
Settings::isAutoScrollEnabled() const
{
    return _imp->_autoScroll->getValue();
}

QString
Settings::makeHTMLDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    ms << tr("Preferences") << "\n==========\n\n";

    const KnobsVec& knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->getIsSecret() ) {
            continue;
        }
        //QString knobScriptName = QString::fromUtf8( (*it)->getName().c_str() );
        QString knobLabel = QString::fromUtf8( (*it)->getLabel().c_str() );
        QString knobHint = QString::fromUtf8( (*it)->getHintToolTip().c_str() );
        KnobPagePtr isPage = toKnobPage(*it);
        KnobSeparatorPtr isSep = toKnobSeparator(*it);
        if (isPage) {
            if (isPage->getParentKnob()) {
                ms << "### " << knobLabel << "\n\n";
            } else {
                ms << knobLabel << "\n----------\n\n";
            }
        } else if (isSep) {
            ms << "**" << knobLabel << "**\n\n";
        } else if ( !knobLabel.isEmpty() && !knobHint.isEmpty() ) {
            if ( ( knobLabel != QString::fromUtf8("Enabled") ) && ( knobLabel != QString::fromUtf8("Zoom support") ) ) {
                ms << "**" << knobLabel << "**\n\n";
                ms << knobHint << "\n\n";
            }
        }
    }

    if (genHTML) {
        ts << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n";
        ts << "<html>\n<head>\n";
        ts << "<title>" << tr("Natron Preferences") << "</title>\n";
        ts << "<link rel=\"stylesheet\" href=\"_static/makdown.css\" type=\"text/css\" />\n<script type=\"text/javascript\" src=\"_static/jquery.js\"></script>\n<script type=\"text/javascript\" src=\"_static/dropdown.js\"></script>\n";
        ts << "</head>\n<body>\n";
        ts << "<div class=\"related\">\n<h3>" << tr("Navigation") << "</h3>\n<ul>\n";
        ts << "<li><a href=\"/index.html\">" << tr("%1 %2 documentation").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_VERSION_STRING) ) << "</a> &raquo;</li>\n";
        ts << "<li><a href=\"/_group.html\">";
        ts << tr("Reference Guide");
        ts << "</a> &raquo;</li>";
        ts << "</ul>\n</div>\n";
        ts << "<div class=\"document\">\n<div class=\"documentwrapper\">\n<div class=\"body\">\n";
        ts << "<div class=\"section\">\n";
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixSettingsHTML(html);
        ts << "</div>\n</div>\n</div>\n<div class=\"clearer\"></div>\n</div>\n<div class=\"footer\"></div>\n</body>\n</html>\n";
    } else {
        ts << markdown;
    }

    return ret;
} // Settings::makeHTMLDocumentation

void
Settings::populateSystemFonts(const QSettings& settings,
                              const std::vector<std::string>& fonts)
{
    _imp->_systemFontChoice->populateChoices(fonts);
    _imp->_scriptEditorFontChoice->populateChoices(fonts);
    for (U32 i = 0; i < fonts.size(); ++i) {
        if (fonts[i] == NATRON_FONT) {
            _imp->_systemFontChoice->setDefaultValue(i);
        }
        if (fonts[i] == NATRON_SCRIPT_FONT) {
            _imp->_scriptEditorFontChoice->setDefaultValue(i);
        }
    }
    ///Now restore properly the system font choice
    {
        QString name = QString::fromUtf8( _imp->_systemFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _imp->_systemFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
    {
        QString name = QString::fromUtf8( _imp->_scriptEditorFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _imp->_scriptEditorFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
}

void
Settings::getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const
{
    assert(paths);
    try {
        _imp->_extraPluginPaths->getPaths(paths);
    } catch (std::logic_error) {
        paths->clear();
    }
}

bool
Settings::getUseStdOFXPluginsLocation() const
{
    return _imp->_useStdOFXPluginsLocation->getValue();
}

void
Settings::restoreDefault()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    if ( !QFile::remove( settings.fileName() ) ) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }

    _imp->_nRedrawStyleSheetRequests = 0;
    _imp->_restoringDefaults = true;

    beginChanges();
    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        knobs[i]->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    }
    _imp->setCachingLabels();
    endChanges();

    if (_imp->_nRedrawStyleSheetRequests > 0) {
        appPTR->reloadStylesheets();
        _imp->_nRedrawStyleSheetRequests = 0;
    }
    _imp->_restoringDefaults = false;
}

bool
Settings::isRenderInSeparatedProcessEnabled() const
{
    return _imp->_renderInSeparateProcess->getValue();
}

int
Settings::getMaximumUndoRedoNodeGraph() const
{
    return _imp->_maxUndoRedoNodeGraph->getValue();
}

int
Settings::getAutoSaveDelayMS() const
{
    return _imp->_autoSaveDelay->getValue() * 1000;
}

bool
Settings::isAutoSaveEnabledForUnsavedProjects() const
{
    return _imp->_autoSaveUnSavedProjects->getValue();
}

bool
Settings::isSnapToNodeEnabled() const
{
    return _imp->_snapNodesToConnections->getValue();
}

bool
Settings::isCheckForUpdatesEnabled() const
{
    return _imp->_checkForUpdates->getValue();
}

void
Settings::setCheckUpdatesEnabled(bool enabled)
{
    _imp->_checkForUpdates->setValue(enabled);
    saveSetting( _imp->_checkForUpdates );
}

bool
Settings::isCrashReportingEnabled() const
{
    return _imp->_enableCrashReports->getValue();
}

int
Settings::getMaxPanelsOpened() const
{
    return _imp->_maxPanelsOpened->getValue();
}

void
Settings::setMaxPanelsOpened(int maxPanels)
{
    _imp->_maxPanelsOpened->setValue(maxPanels);
    saveSetting( _imp->_maxPanelsOpened );
}

bool
Settings::loadBundledPlugins() const
{
    return _imp->_loadBundledPlugins->getValue();
}

bool
Settings::preferBundledPlugins() const
{
    return _imp->_preferBundledPlugins->getValue();
}

void
Settings::getDefaultNodeColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _imp->_defaultNodeColor->getValue(DimIdx(0));
    *g = _imp->_defaultNodeColor->getValue(DimIdx(1));
    *b = _imp->_defaultNodeColor->getValue(DimIdx(2));
}

void
Settings::getDefaultBackdropColor(float *r,
                                  float *g,
                                  float *b) const
{
    *r = _imp->_defaultBackdropColor->getValue(DimIdx(0));
    *g = _imp->_defaultBackdropColor->getValue(DimIdx(1));
    *b = _imp->_defaultBackdropColor->getValue(DimIdx(2));
}

void
Settings::getGeneratorColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultGeneratorColor->getValue(DimIdx(0));
    *g = _imp->_defaultGeneratorColor->getValue(DimIdx(1));
    *b = _imp->_defaultGeneratorColor->getValue(DimIdx(2));
}

void
Settings::getReaderColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _imp->_defaultReaderColor->getValue(DimIdx(0));
    *g = _imp->_defaultReaderColor->getValue(DimIdx(1));
    *b = _imp->_defaultReaderColor->getValue(DimIdx(2));
}

void
Settings::getWriterColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _imp->_defaultWriterColor->getValue(DimIdx(0));
    *g = _imp->_defaultWriterColor->getValue(DimIdx(1));
    *b = _imp->_defaultWriterColor->getValue(DimIdx(2));
}

void
Settings::getColorGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultColorGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultColorGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultColorGroupColor->getValue(DimIdx(2));
}

void
Settings::getFilterGroupColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _imp->_defaultFilterGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultFilterGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultFilterGroupColor->getValue(DimIdx(2));
}

void
Settings::getTransformGroupColor(float *r,
                                 float *g,
                                 float *b) const
{
    *r = _imp->_defaultTransformGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultTransformGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultTransformGroupColor->getValue(DimIdx(2));
}

void
Settings::getTimeGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultTimeGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultTimeGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultTimeGroupColor->getValue(DimIdx(2));
}

void
Settings::getDrawGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultDrawGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultDrawGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultDrawGroupColor->getValue(DimIdx(2));
}

void
Settings::getKeyerGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultKeyerGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultKeyerGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultKeyerGroupColor->getValue(DimIdx(2));
}

void
Settings::getChannelGroupColor(float *r,
                               float *g,
                               float *b) const
{
    *r = _imp->_defaultChannelGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultChannelGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultChannelGroupColor->getValue(DimIdx(2));
}

void
Settings::getMergeGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultMergeGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultMergeGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultMergeGroupColor->getValue(DimIdx(2));
}

void
Settings::getViewsGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _imp->_defaultViewsGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultViewsGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultViewsGroupColor->getValue(DimIdx(2));
}

void
Settings::getDeepGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _imp->_defaultDeepGroupColor->getValue(DimIdx(0));
    *g = _imp->_defaultDeepGroupColor->getValue(DimIdx(1));
    *b = _imp->_defaultDeepGroupColor->getValue(DimIdx(2));
}

int
Settings::getDisconnectedArrowLength() const
{
    return _imp->_disconnectedArrowLength->getValue();
}

std::string
Settings::getHostName() const
{
    int entry_i =  _imp->_hostName->getValue();
    std::vector<std::string> entries = _imp->_hostName->getEntries();

    if ( (entry_i >= 0) && ( entry_i < (int)entries.size() ) && (entries[entry_i] == NATRON_CUSTOM_HOST_NAME_ENTRY) ) {
        return _imp->_customHostName->getValue();
    } else {
        if ( (entry_i >= 0) && ( entry_i < (int)_imp->_knownHostNames.size() ) ) {
            return _imp->_knownHostNames[entry_i];
        }

        return std::string();
    }
}

const std::string&
Settings::getKnownHostName(KnownHostNameEnum e) const
{
    return _imp->_knownHostNames[(int)e];
}

bool
Settings::getRenderOnEditingFinishedOnly() const
{
    return _imp->_renderOnEditingFinished->getValue();
}

void
Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _imp->_renderOnEditingFinished->setValue(render);
}

bool
Settings::getIconsBlackAndWhite() const
{
    return _imp->_useBWIcons->getValue();
}

std::string
Settings::getDefaultLayoutFile() const
{
    return _imp->_defaultLayoutFile->getValue();
}

bool
Settings::getLoadProjectWorkspce() const
{
    return _imp->_loadProjectsWorkspace->getValue();
}

bool
Settings::useCursorPositionIncrements() const
{
    return _imp->_useCursorPositionIncrements->getValue();
}

bool
Settings::isAutoProjectFormatEnabled() const
{
    return _imp->_firstReadSetProjectFormat->getValue();
}

bool
Settings::isAutoFixRelativeFilePathEnabled() const
{
    return _imp->_fixPathsOnProjectPathChanged->getValue();
}

int
Settings::getNumberOfParallelRenders() const
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL

    return _imp->_numberOfParallelRenders->getValue();
#else

    return 1;
#endif
}

void
Settings::setNumberOfParallelRenders(int nb)
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _imp->_numberOfParallelRenders->setValue(nb);
#endif
}

bool
Settings::areRGBPixelComponentsSupported() const
{
    return _imp->_activateRGBSupport->getValue();
}

bool
Settings::isTransformConcatenationEnabled() const
{
    return _imp->_activateTransformConcatenationSupport->getValue();
}

bool
Settings::useGlobalThreadPool() const
{
    return _imp->_useThreadPool->getValue();
}

void
Settings::setUseGlobalThreadPool(bool use)
{
    _imp->_useThreadPool->setValue(use);
}

bool
Settings::isMergeAutoConnectingToAInput() const
{
    return _imp->_useInputAForMergeAutoConnect->getValue();
}

void
Settings::doOCIOStartupCheckIfNeeded()
{
    bool docheck = _imp->_ocioStartupCheck->getValue();
    AppInstancePtr mainInstance = appPTR->getTopLevelInstance();

    if (!mainInstance) {
        qDebug() << "WARNING: doOCIOStartupCheckIfNeeded() called without a AppInstance";

        return;
    }

    if (docheck && mainInstance) {
        int entry_i = _imp->_ocioConfigKnob->getValue();
        std::vector<std::string> entries = _imp->_ocioConfigKnob->getEntries();
        std::string warnText;
        if ( (entry_i < 0) || ( entry_i >= (int)entries.size() ) ) {
            warnText = tr("The current OCIO config selected in the preferences is invalid, would you like to set it to the default config (%1)?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else if (entries[entry_i] != NATRON_DEFAULT_OCIO_CONFIG_NAME) {
            warnText = tr("The current OCIO config selected in the preferences is not the default one (%1), would you like to set it to the default config?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else {
            return;
        }

        bool stopAsking = false;
        StandardButtonEnum reply = mainInstance->questionDialog("OCIO config", warnText, false,
                                                                StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                                eStandardButtonYes,
                                                                &stopAsking);
        if (stopAsking != !docheck) {
            _imp->_ocioStartupCheck->setValue(!stopAsking);
            saveSetting( _imp->_ocioStartupCheck );
        }

        if (reply == eStandardButtonYes) {
            int defaultIndex = -1;
            for (unsigned i = 0; i < entries.size(); ++i) {
                if (entries[i].find(NATRON_DEFAULT_OCIO_CONFIG_NAME) != std::string::npos) {
                    defaultIndex = i;
                    break;
                }
            }
            if (defaultIndex != -1) {
                _imp->_ocioConfigKnob->setValue(defaultIndex);
                saveSetting( _imp->_ocioConfigKnob );
            } else {
                Dialogs::warningDialog( "OCIO config", tr("The %2 OCIO config could not be found.\n"
                                                          "This is probably because you're not using the OpenColorIO-Configs folder that should "
                                                          "be bundled with your %1 installation.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString() );
            }
        }
    }
} // Settings::doOCIOStartupCheckIfNeeded

bool
Settings::didSettingsExistOnStartup() const
{
    return _imp->_settingsExisted;
}

bool
Settings::notifyOnFileChange() const
{
    return _imp->_notifyOnFileChange->getValue();
}

bool
Settings::isAutoTurboEnabled() const
{
    return _imp->_autoTurbo->getValue();
}

void
Settings::setAutoTurboModeEnabled(bool e)
{
    _imp->_autoTurbo->setValue(e);
}

void
Settings::setOptionalInputsAutoHidden(bool hidden)
{
    _imp->_hideOptionalInputsAutomatically->setValue(hidden);
}

bool
Settings::areOptionalInputsAutoHidden() const
{
    return _imp->_hideOptionalInputsAutomatically->getValue();
}

void
Settings::getPythonGroupsSearchPaths(std::list<std::string>* templates) const
{
    _imp->_templatesPluginPaths->getPaths(templates);
}

void
Settings::appendPythonGroupsPath(const std::string& path)
{
    _imp->_templatesPluginPaths->appendPath(path);
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.setValue( QString::fromUtf8( _imp->_templatesPluginPaths->getName().c_str() ), QVariant( QString::fromUtf8( _imp->_templatesPluginPaths->getValue(DimIdx(0)).c_str() ) ) );
}

std::string
Settings::getDefaultOnProjectLoadedCB()
{
    return _imp->_defaultOnProjectLoaded->getValue();
}

std::string
Settings::getDefaultOnProjectSaveCB()
{
    return _imp->_defaultOnProjectSave->getValue();
}

std::string
Settings::getDefaultOnProjectCloseCB()
{
    return _imp->_defaultOnProjectClose->getValue();
}

std::string
Settings::getDefaultOnNodeCreatedCB()
{
    return _imp->_defaultOnNodeCreated->getValue();
}

std::string
Settings::getDefaultOnNodeDeleteCB()
{
    return _imp->_defaultOnNodeDelete->getValue();
}

std::string
Settings::getOnProjectCreatedCB()
{
    return _imp->_onProjectCreated->getValue();
}

bool
Settings::isAutoDeclaredVariablePrintActivated() const
{
    return _imp->_echoVariableDeclarationToPython->getValue();
}

void
Settings::setAutoDeclaredVariablePrintEnabled(bool enabled)
{
    _imp->_echoVariableDeclarationToPython->setValue(enabled);
    saveSetting( _imp->_echoVariableDeclarationToPython );
}

bool
Settings::isPluginIconActivatedOnNodeGraph() const
{
    return _imp->_usePluginIconsInNodeGraph->getValue();
}

bool
Settings::isNodeGraphAntiAliasingEnabled() const
{
    return _imp->_useAntiAliasing->getValue();
}

void
Settings::getSunkenColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_sunkenColor->getValue(DimIdx(0));
    *g = _imp->_sunkenColor->getValue(DimIdx(1));
    *b = _imp->_sunkenColor->getValue(DimIdx(2));
}

void
Settings::getBaseColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_baseColor->getValue(DimIdx(0));
    *g = _imp->_baseColor->getValue(DimIdx(1));
    *b = _imp->_baseColor->getValue(DimIdx(2));
}

void
Settings::getRaisedColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_raisedColor->getValue(DimIdx(0));
    *g = _imp->_raisedColor->getValue(DimIdx(1));
    *b = _imp->_raisedColor->getValue(DimIdx(2));
}

void
Settings::getSelectionColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_selectionColor->getValue(DimIdx(0));
    *g = _imp->_selectionColor->getValue(DimIdx(1));
    *b = _imp->_selectionColor->getValue(DimIdx(2));
}

void
Settings::getInterpolatedColor(double* r,
                               double* g,
                               double* b) const
{
    *r = _imp->_interpolatedColor->getValue(DimIdx(0));
    *g = _imp->_interpolatedColor->getValue(DimIdx(1));
    *b = _imp->_interpolatedColor->getValue(DimIdx(2));
}

void
Settings::getKeyframeColor(double* r,
                           double* g,
                           double* b) const
{
    *r = _imp->_keyframeColor->getValue(DimIdx(0));
    *g = _imp->_keyframeColor->getValue(DimIdx(1));
    *b = _imp->_keyframeColor->getValue(DimIdx(2));
}

void
Settings::getTrackerKeyframeColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _imp->_trackerKeyframeColor->getValue(DimIdx(0));
    *g = _imp->_trackerKeyframeColor->getValue(DimIdx(1));
    *b = _imp->_trackerKeyframeColor->getValue(DimIdx(2));
}

void
Settings::getExprColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_exprColor->getValue(DimIdx(0));
    *g = _imp->_exprColor->getValue(DimIdx(1));
    *b = _imp->_exprColor->getValue(DimIdx(2));
}


void
Settings::getCloneColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_cloneColor->getValue(DimIdx(0));
    *g = _imp->_cloneColor->getValue(DimIdx(1));
    *b = _imp->_cloneColor->getValue(DimIdx(2));
}

void
Settings::getTextColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _imp->_textColor->getValue(DimIdx(0));
    *g = _imp->_textColor->getValue(DimIdx(1));
    *b = _imp->_textColor->getValue(DimIdx(2));
}

void
Settings::getAltTextColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _imp->_altTextColor->getValue(DimIdx(0));
    *g = _imp->_altTextColor->getValue(DimIdx(1));
    *b = _imp->_altTextColor->getValue(DimIdx(2));
}

void
Settings::getTimelinePlayheadColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _imp->_timelinePlayheadColor->getValue(DimIdx(0));
    *g = _imp->_timelinePlayheadColor->getValue(DimIdx(1));
    *b = _imp->_timelinePlayheadColor->getValue(DimIdx(2));
}

void
Settings::getTimelineBoundsColor(double* r,
                                 double* g,
                                 double* b) const
{
    *r = _imp->_timelineBoundsColor->getValue(DimIdx(0));
    *g = _imp->_timelineBoundsColor->getValue(DimIdx(1));
    *b = _imp->_timelineBoundsColor->getValue(DimIdx(2));
}

void
Settings::getTimelineBGColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_timelineBGColor->getValue(DimIdx(0));
    *g = _imp->_timelineBGColor->getValue(DimIdx(1));
    *b = _imp->_timelineBGColor->getValue(DimIdx(2));
}

void
Settings::getCachedFrameColor(double* r,
                              double* g,
                              double* b) const
{
    *r = _imp->_cachedFrameColor->getValue(DimIdx(0));
    *g = _imp->_cachedFrameColor->getValue(DimIdx(1));
    *b = _imp->_cachedFrameColor->getValue(DimIdx(2));
}

void
Settings::getDiskCachedColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_diskCachedFrameColor->getValue(DimIdx(0));
    *g = _imp->_diskCachedFrameColor->getValue(DimIdx(1));
    *b = _imp->_diskCachedFrameColor->getValue(DimIdx(2));
}

void
Settings::getAnimationModuleEditorBackgroundColor(double *r,
                                            double *g,
                                            double *b) const
{
    *r = _imp->_animationViewBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_animationViewBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_animationViewBackgroundColor->getValue(DimIdx(2));
}

void
Settings::getAnimationModuleEditorRootRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(2));
    *a = _imp->_dopesheetRootSectionBackgroundColor->getValue(DimIdx(3));
}

void
Settings::getAnimationModuleEditorKnobRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(0));
    *g = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(1));
    *b = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(2));
    *a = _imp->_dopesheetKnobSectionBackgroundColor->getValue(DimIdx(3));
}

void
Settings::getAnimationModuleEditorScaleColor(double *r,
                                       double *g,
                                       double *b) const
{
    *r = _imp->_animationViewScaleColor->getValue(DimIdx(0));
    *g = _imp->_animationViewScaleColor->getValue(DimIdx(1));
    *b = _imp->_animationViewScaleColor->getValue(DimIdx(2));
}

void
Settings::getAnimationModuleEditorGridColor(double *r,
                                      double *g,
                                      double *b) const
{
    *r = _imp->_animationViewGridColor->getValue(DimIdx(0));
    *g = _imp->_animationViewGridColor->getValue(DimIdx(1));
    *b = _imp->_animationViewGridColor->getValue(DimIdx(2));
}

void
Settings::getSEKeywordColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_keywordColor->getValue(DimIdx(0));
    *g = _imp->_keywordColor->getValue(DimIdx(1));
    *b = _imp->_keywordColor->getValue(DimIdx(2));
}

void
Settings::getSEOperatorColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_operatorColor->getValue(DimIdx(0));
    *g = _imp->_operatorColor->getValue(DimIdx(1));
    *b = _imp->_operatorColor->getValue(DimIdx(2));
}

void
Settings::getSEBraceColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _imp->_braceColor->getValue(DimIdx(0));
    *g = _imp->_braceColor->getValue(DimIdx(1));
    *b = _imp->_braceColor->getValue(DimIdx(2));
}

void
Settings::getSEDefClassColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_defClassColor->getValue(DimIdx(0));
    *g = _imp->_defClassColor->getValue(DimIdx(1));
    *b = _imp->_defClassColor->getValue(DimIdx(2));
}

void
Settings::getSEStringsColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_stringsColor->getValue(DimIdx(0));
    *g = _imp->_stringsColor->getValue(DimIdx(1));
    *b = _imp->_stringsColor->getValue(DimIdx(2));
}

void
Settings::getSECommentsColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _imp->_commentsColor->getValue(DimIdx(0));
    *g = _imp->_commentsColor->getValue(DimIdx(1));
    *b = _imp->_commentsColor->getValue(DimIdx(2));
}

void
Settings::getSESelfColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _imp->_selfColor->getValue(DimIdx(0));
    *g = _imp->_selfColor->getValue(DimIdx(1));
    *b = _imp->_selfColor->getValue(DimIdx(2));
}

void
Settings::getSENumbersColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_numbersColor->getValue(DimIdx(0));
    *g = _imp->_numbersColor->getValue(DimIdx(1));
    *b = _imp->_numbersColor->getValue(DimIdx(2));
}

void
Settings::getSECurLineColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_curLineColor->getValue(DimIdx(0));
    *g = _imp->_curLineColor->getValue(DimIdx(1));
    *b = _imp->_curLineColor->getValue(DimIdx(2));
}

void
Settings::getSliderColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _imp->_sliderColor->getValue(DimIdx(0));
    *g = _imp->_sliderColor->getValue(DimIdx(1));
    *b = _imp->_sliderColor->getValue(DimIdx(2));
}

int
Settings::getSEFontSize() const
{
    return _imp->_scriptEditorFontSize->getValue();
}

std::string
Settings::getSEFontFamily() const
{
    return _imp->_scriptEditorFontChoice->getActiveEntryText();
}

void
Settings::getPluginIconFrameColor(int *r,
                                  int *g,
                                  int *b) const
{
    *r = 50;
    *g = 50;
    *b = 50;
}

bool
Settings::isNaNHandlingEnabled() const
{
    return _imp->_convertNaNValues->getValue();
}

bool
Settings::isCopyInputImageForPluginRenderEnabled() const
{
    return _imp->_pluginUseImageCopyForSource->getValue();
}

void
Settings::setOnProjectCreatedCB(const std::string& func)
{
    _imp->_onProjectCreated->setValue(func);
}

void
Settings::setOnProjectLoadedCB(const std::string& func)
{
    _imp->_defaultOnProjectLoaded->setValue(func);
}

bool
Settings::isDefaultAppearanceOutdated() const
{
    return _imp->_defaultAppearanceOutdated;
}

void
Settings::restoreDefaultAppearance()
{
    std::vector< KnobIPtr > children = _imp->_appearanceTab->getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        KnobColorPtr isColorKnob = toKnobColor(children[i]);
        if ( isColorKnob && isColorKnob->isSimplified() ) {
            isColorKnob->blockValueChanges();
            isColorKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
            isColorKnob->unblockValueChanges();
        }
    }
    _imp->_defaultAppearanceOutdated = false;
    appPTR->reloadStylesheets();
}

std::string
Settings::getUserStyleSheetFilePath() const
{
    return _imp->_qssFile->getValue();
}

void
Settings::setRenderQueuingEnabled(bool enabled)
{
    _imp->_queueRenders->setValue(enabled);
    saveSetting( _imp->_queueRenders );
}

bool
Settings::isRenderQueuingEnabled() const
{
    return _imp->_queueRenders->getValue();
}

bool
Settings::isFileDialogEnabledForNewWriters() const
{
    return _imp->_filedialogForWriters->getValue();
}

bool
Settings::isDriveLetterToUNCPathConversionEnabled() const
{
    return !_imp->_enableMappingFromDriveLettersToUNCShareNames->getValue();
}

bool
Settings::getIsFullRecoverySaveModeEnabled() const
{
    return _imp->_saveSafetyMode->getValue();
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_Settings.cpp"
